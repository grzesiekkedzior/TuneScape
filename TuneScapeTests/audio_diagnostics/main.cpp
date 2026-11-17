#include <QAudioOutput>
#include <QAudioDevice>
#include <QAudioDecoder>
#include <QAudioBuffer>
#include <QBuffer>
#include <QElapsedTimer>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QLibraryInfo>
#include <QLibrary>
#include <QMediaDevices>
#include <QMediaMetaData>
#include <QMediaPlayer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QNetworkProxy>
#include <QNetworkProxyFactory>
#include <QNetworkProxyQuery>
#include <QPointer>
#include <QProcess>
#include <QTemporaryDir>
#include <QFile>
#include <QSslConfiguration>
#include <QSslSocket>
#include <QSslCipher>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <QtTest>
#include <memory>
#include <algorithm>
#ifdef Q_OS_WIN
#include <windows.h>
#include <psapi.h>
#endif

namespace {

void log(QJsonObject record)
{
    qInfo().noquote() << QJsonDocument(record).toJson(QJsonDocument::Compact);
}

qint64 privateBytes()
{
#ifdef Q_OS_WIN
    PROCESS_MEMORY_COUNTERS_EX counters{};
    if (GetProcessMemoryInfo(GetCurrentProcess(),
            reinterpret_cast<PROCESS_MEMORY_COUNTERS *>(&counters), sizeof(counters)))
        return qint64(counters.PrivateUsage);
#endif
    return -1;
}

QByteArray audioBytes(bool mono)
{
    auto frame = QByteArray(417, '\0');
    frame.replace(0, 4, QByteArray::fromHex(mono ? "fffd80c0" : "fffb9000"));
    return frame.repeated(240);
}

class HttpAudio : public QTcpServer
{
public:
    HttpAudio()
    {
        connect(this, &QTcpServer::newConnection, this, [this] {
            auto *socket = nextPendingConnection();
            connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
            connect(socket, &QTcpSocket::readyRead, socket, [this, socket, bytes = QByteArray()]() mutable {
                bytes += socket->readAll();
                if (!bytes.contains("\r\n\r\n"))
                    return;
                log({{"request", QString::fromLatin1(bytes)}});
                const bool pending = bytes.startsWith("GET /pending");
                ++requests;
                bytes.clear();
                if (pending)
                    return;
                socket->write("HTTP/1.1 200 OK\r\nContent-Type: audio/mpeg\r\nConnection: close\r\n\r\n");
                socket->write(audio);
                auto *feed = new QTimer(socket);
                connect(feed, &QTimer::timeout, socket, [this, socket] {
                    if (socket->state() == QAbstractSocket::ConnectedState)
                        socket->write(audio.left(417 * 10));
                });
                feed->start(250);
            });
        });
    }

    QUrl url(const QString &path) const
    {
        return QUrl(QStringLiteral("http://127.0.0.1:%1/%2").arg(serverPort()).arg(path));
    }

    int active() const
    {
        int count = 0;
        for (auto *socket : findChildren<QTcpSocket *>())
            count += socket->state() == QAbstractSocket::ConnectedState;
        return count;
    }

    QByteArray audio;
    int requests = 0;
};

void versions()
{
    QJsonObject info{{"qt", qVersion()}, {"build", QLibraryInfo::build()},
                     {"outputs", QMediaDevices::audioOutputs().size()},
                     {"sslBuild", QSslSocket::sslLibraryBuildVersionString()},
                     {"sslRuntime", QSslSocket::sslLibraryVersionString()}};
    QLibrary avutil(QStringLiteral("avutil-58"));
    const auto ffmpegVersion = reinterpret_cast<const char *(*)()>(avutil.resolve("av_version_info"));
    if (ffmpegVersion)
        info.insert("ffmpeg", QString::fromLatin1(ffmpegVersion()));
    const QList<QPair<QString, QString>> libraries = {
        {"avcodec-60", "avcodec_version"}, {"avformat-60", "avformat_version"},
        {"avutil-58", "avutil_version"}, {"swresample-4", "swresample_version"}
    };
    for (const auto &entry : libraries) {
        QLibrary library(entry.first);
        const auto version = reinterpret_cast<unsigned (*)()>(library.resolve(entry.second.toLatin1()));
        if (version) {
            const auto value = version();
            info.insert(entry.first, QStringLiteral("%1.%2.%3")
                .arg(value >> 16).arg((value >> 8) & 255).arg(value & 255));
        }
    }
    log(info);
}

int resample()
{
    // FFmpeg 6 ABI. Layout masks MONO=4/STEREO=3, formats FLT=3/FLTP=8.
    QLibrary library(QStringLiteral("swresample-4"));
    using Context = void;
    const auto allocate = reinterpret_cast<Context *(*)(Context *, qint64, int, int, qint64, int, int, int, void *)>(
        library.resolve("swr_alloc_set_opts"));
    const auto initialize = reinterpret_cast<int (*)(Context *)>(library.resolve("swr_init"));
    const auto convert = reinterpret_cast<int (*)(Context *, unsigned char **, int,
                                                 const unsigned char **, int)>(library.resolve("swr_convert"));
    const auto release = reinterpret_cast<void (*)(Context **)>(library.resolve("swr_free"));
    if (!allocate || !initialize || !convert || !release)
        return 2;
    versions();
    for (int rate : {44100, 48000}) {
        for (int stereo = 0; stereo < 2; ++stereo) {
            auto *context = allocate(nullptr, stereo ? 3 : 4, 3, rate, 4, 8, 44100, 0, nullptr);
            if (!context)
                return 3;
            if (initialize(context) < 0) {
                release(&context);
                return 3;
            }
            alignas(64) unsigned char input[1152 * sizeof(float)]{};
            alignas(64) unsigned char output[2048 * 2 * sizeof(float)]{};
            const unsigned char *in[] = {input};
            unsigned char *out[] = {output};
            for (int i = 0; i < 10000; ++i) {
                const int converted = convert(context, out, 2048, in, 1152);
                if (converted <= 0 || converted > 2048) {
                    release(&context);
                    return 4;
                }
            }
            release(&context);
            log({{"directResample", stereo ? "mono-to-stereo" : "mono-to-mono"},
                 {"rate", rate}, {"iterations", 10000}});
        }
    }
    return 0;
}

int worker(const QStringList &args)
{
    const bool mono = args.contains("--mono");
    const bool stopOnly = args.contains("--stop-only");
    const bool destroyFirst = args.contains("--destroy-first");
    const bool buffering = args.contains("--buffering");
    const bool connecting = args.contains("--connecting");
    const int index = args.indexOf("--cycles");
    const int cycles = index >= 0 ? args.value(index + 1).toInt() : 12;
    if (cycles < 1 || cycles > 200)
        return 2;
    versions();
    HttpAudio server;
    server.audio = audioBytes(mono);
    if (!server.listen(QHostAddress::LocalHost))
        return 3;
    QAudioOutput output;
    QObject owner;
    if (args.contains("--stereo-output")) {
        const auto devices = QMediaDevices::audioOutputs();
        const auto stereo = std::find_if(devices.cbegin(), devices.cend(), [](const auto &device) {
            return device.preferredFormat().channelCount() == 2;
        });
        if (stereo == devices.cend())
            return 16;
        output.setDevice(*stereo);
    }
    output.setVolume(0);
    QObject::connect(&output, &QObject::destroyed, qApp, [] {
        log({{"destroyed", "audioOutput"}});
    });
    log({{"output", output.device().description()},
         {"outputChannels", output.device().preferredFormat().channelCount()},
         {"outputRate", output.device().preferredFormat().sampleRate()}});
    QPointer<QMediaPlayer> current;
    QList<QPointer<QMediaPlayer>> retired;
    quint64 attempt = 0;
    int obsoleteCallbacks = 0;
    int updates = 0;
    QElapsedTimer clock;
    clock.start();
    const qint64 initialMemory = privateBytes();

    const auto stop = [&] {
        ++attempt;
        auto *player = current.data();
        current.clear();
        if (!player)
            return;
        QObject::disconnect(player, nullptr, &owner, nullptr);
        player->stop();
        if (!stopOnly) {
            player->setAudioOutput(nullptr);
            player->setSource({});
        }
        retired.append(player);
        player->deleteLater();
    };
    const auto start = [&](int cycle, const QString &path) {
        stop();
        if (destroyFirst && !QTest::qWaitFor([&] {
                return std::all_of(retired.cbegin(), retired.cend(), [](auto p) { return p.isNull(); });
            }, 5000))
            return false;
        auto *player = new QMediaPlayer(&owner);
        current = player;
        const auto id = ++attempt;
        QObject::connect(player, &QObject::destroyed, qApp, [id] {
            log({{"destroyed", "mediaPlayer"}, {"attempt", qint64(id)}});
        });
        QObject::connect(player, &QMediaPlayer::mediaStatusChanged, &owner, [&, id, player, cycle](auto status) {
            if (id != attempt || current != player)
                ++obsoleteCallbacks;
            else
                ++updates;
            log({{"cycle", cycle}, {"attempt", qint64(id)}, {"status", int(status)}});
        });
        QObject::connect(player, &QMediaPlayer::errorOccurred, &owner, [&, id](auto error, const QString &detail) {
            log({{"attemptError", qint64(id)}, {"error", int(error)}, {"detail", detail}});
        });
        player->setAudioOutput(&output);
        player->setSource(server.url(path));
        player->play();
        return true;
    };

    for (int cycle = 0; cycle < cycles; ++cycle) {
        log({{"beginCycle", cycle}, {"mono", mono}, {"privateBytes", privateBytes()}});
        if (!start(cycle, connecting ? "pending" : (cycle % 2 ? "B" : "A")))
            return 4;
        if (connecting) {
            if (!QTest::qWaitFor([&] { return server.active() >= 1; }, 5000))
                return 5;
        } else if (buffering) {
            if (!QTest::qWaitFor([&] {
                    return current->mediaStatus() == QMediaPlayer::BufferingMedia
                        || current->mediaStatus() == QMediaPlayer::BufferedMedia
                        || current->error() != QMediaPlayer::NoError;
                }, 12000))
                return 6;
        } else {
            if (!QTest::qWaitFor([&] {
                    return current->position() >= 100 || current->error() != QMediaPlayer::NoError;
                }, 12000) || current->error() != QMediaPlayer::NoError)
                return 7;
        }

        if (cycle % 4 == 3) {
            if (!start(cycle, "B") || !start(cycle, "A"))
                return 8;
        }
        stop();
        if (!QTest::qWaitFor([&] {
                return server.active() == 0 && owner.findChildren<QMediaPlayer *>().isEmpty();
            }, 5000))
            return 9;
        for (const auto &pointer : retired) {
            if (pointer)
                return 10;
        }
        retired.clear();
    }

    // Destruction with a live decoder and its shared output follows parent-child ownership.
    if (!start(cycles, "shutdown"))
        return 11;
    if (!QTest::qWaitFor([&] { return current->position() >= 100; }, 12000))
        return 12;
    const QPointer<QMediaPlayer> finalPlayer(current);
    delete current.data();
    if (finalPlayer)
        return 13;
    if (!QTest::qWaitFor([&] { return server.active() == 0; }, 5000))
        return 14;

    log({{"result", "success"}, {"cycles", cycles}, {"mono", mono},
         {"updates", updates}, {"obsoleteCallbacks", obsoleteCallbacks},
         {"players", owner.findChildren<QMediaPlayer *>().size()}, {"outputs", 1},
         {"httpActive", server.active()}, {"httpRequests", server.requests},
         {"elapsedMs", clock.elapsed()}, {"privateBytesBefore", initialMemory},
         {"privateBytesAfter", privateBytes()}});
    return obsoleteCallbacks == 0 ? 0 : 15;
}

int httpProbe(const QStringList &args)
{
    const QUrl url(args.value(args.indexOf("--http") + 1));
    if (!url.isValid() || (url.scheme() != "http" && url.scheme() != "https"))
        return 2;
    versions();
    QNetworkAccessManager network;
    if (args.contains("--no-proxy"))
        network.setProxy(QNetworkProxy::NoProxy);
    for (const auto &proxy : QNetworkProxyFactory::proxyForQuery(QNetworkProxyQuery(url)))
        log({{"systemProxyType", int(proxy.type())}, {"proxyHost", proxy.hostName()}});
    const auto variantIndex = args.indexOf("--variant");
    const int first = variantIndex >= 0 ? args.value(variantIndex + 1).toInt() : 0;
    const int last = variantIndex >= 0 ? first + 1 : 8;
    if (first < 0 || last > 8)
        return 2;
    for (int variant = first; variant < last; ++variant) {
        QNetworkRequest request(url);
        request.setTransferTimeout(6000);
        if ((variant >= 1 && variant <= 4) || variant == 7)
            request.setRawHeader("User-Agent", "TuneScape/6.0");
        if (variant >= 2 && variant <= 4)
            request.setRawHeader("Accept", "audio/mpeg");
        if (variant == 5 || variant == 7)
            request.setRawHeader("Accept", "*/*");
        if (variant == 3 || variant == 4 || variant == 6)
            request.setRawHeader("Icy-MetaData", "1");
        if (variant == 4)
            request.setAttribute(QNetworkRequest::Http2AllowedAttribute, false);
        auto *reply = network.get(request);
        QJsonArray redirects;
        bool encrypted = false;
        QJsonArray sslErrors;
        QObject::connect(reply, &QNetworkReply::encrypted, reply, [&] { encrypted = true; });
        QObject::connect(reply, &QNetworkReply::sslErrors, reply, [&](const auto &errors) {
            for (const auto &error : errors)
                sslErrors.append(error.errorString());
        });
        QObject::connect(reply, &QNetworkReply::redirected, reply, [&](const QUrl &redirect) {
            redirects.append(redirect.toString());
        });
        const bool ready = QTest::qWaitFor([&] { return reply->isFinished() || reply->bytesAvailable() >= 4096; }, 8000);
        QJsonObject headers;
        for (const auto &header : reply->rawHeaderPairs()) {
            const auto name = header.first.toLower();
            if (name != "set-cookie")
                headers.insert(QString::fromLatin1(header.first), QString::fromLatin1(header.second));
        }
        const auto ssl = reply->sslConfiguration();
        log({{"variant", variant}, {"url", reply->url().toString()},
             {"redirects", redirects}, {"status", reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()},
             {"networkError", int(reply->error())}, {"message", reply->errorString()},
             {"bytes", qint64(reply->bytesAvailable())}, {"completedOrReceivedData", ready},
             {"http2", reply->attribute(QNetworkRequest::Http2WasUsedAttribute).toBool()},
             {"tlsProtocol", int(ssl.sessionProtocol())},
             {"encrypted", encrypted}, {"sslErrors", sslErrors},
             {"cipher", ssl.sessionCipher().name()}, {"headers", headers}});
        reply->abort();
        delete reply;
    }
    return 0;
}

int decode(const QStringList &args)
{
    versions();
    const QTemporaryDir directory;
    if (!directory.isValid())
        return 2;
    QFile file(directory.filePath("fixture.mpeg"));
    if (!file.open(QIODevice::WriteOnly))
        return 3;
    const auto bytes = audioBytes(args.contains("--mono"));
    if (file.write(bytes) != bytes.size())
        return 4;
    file.close();
    QAudioDecoder decoder;
    int frames = 0;
    bool finished = false;
    bool valid = true;
    QObject::connect(&decoder, &QAudioDecoder::bufferReady, &decoder, [&] {
        const auto buffer = decoder.read();
        if (!frames)
            log({{"channels", buffer.format().channelCount()},
                 {"sampleRate", buffer.format().sampleRate()}, {"format", int(buffer.format().sampleFormat())}});
        frames += buffer.frameCount();
        valid &= buffer.isValid();
    });
    QObject::connect(&decoder, &QAudioDecoder::finished, &decoder, [&] { finished = true; });
    decoder.setSource(QUrl::fromLocalFile(file.fileName()));
    decoder.start();
    const bool ready = QTest::qWaitFor([&] { return finished || decoder.error() != QAudioDecoder::NoError; }, 10000);
    log({{"finished", finished}, {"frames", frames}, {"error", decoder.errorString()}});
    return ready && finished && valid && frames > 0 ? 0 : 5;
}

class AudioDiagnostics : public QObject
{
    Q_OBJECT
private slots:
    void isolatedAudio_data()
    {
        QTest::addColumn<QStringList>("arguments");
        QTest::newRow("mono-playing") << (QStringList{"--worker", "--mono"});
        QTest::newRow("stereo-playing") << (QStringList{"--worker"});
        QTest::newRow("mono-buffering") << (QStringList{"--worker", "--mono", "--buffering"});
        QTest::newRow("mono-connecting") << (QStringList{"--worker", "--mono", "--connecting"});
        QTest::newRow("mono-decoder") << (QStringList{"--decode", "--mono"});
        QTest::newRow("stereo-decoder") << (QStringList{"--decode"});
    }

    void isolatedAudio()
    {
        if (qEnvironmentVariableIntValue("TUNESCAPE_TEST_MEDIA") != 1)
            QSKIP("Set TUNESCAPE_TEST_MEDIA=1 to test the installed multimedia backend.");
        QFETCH(QStringList, arguments);
        QProcess child;
        child.start(QCoreApplication::applicationFilePath(), arguments);
        QVERIFY(child.waitForStarted());
        if (!child.waitForFinished(90000)) {
            child.kill();
            child.waitForFinished(5000);
            QFAIL("Diagnostic subprocess exceeded 90 seconds.");
        }
        const auto output = child.readAllStandardError() + child.readAllStandardOutput();
        qInfo().noquote() << output;
        QCOMPARE(child.exitStatus(), QProcess::NormalExit);
        QCOMPARE(child.exitCode(), 0);
    }
};

} // namespace

int main(int argc, char **argv)
{
    qputenv("QT_FORCE_STDERR_LOGGING", "1");
    qSetMessagePattern(QStringLiteral("%{message}"));
    QGuiApplication app(argc, argv);
    const auto args = app.arguments();
    if (args.contains("--worker"))
        return worker(args);
    if (args.contains("--http"))
        return httpProbe(args);
    if (args.contains("--decode"))
        return decode(args);
    if (args.contains("--resample"))
        return resample();
    if (args.contains("--http-local")) {
        HttpAudio server;
        server.audio = audioBytes(false);
        if (!server.listen(QHostAddress::LocalHost))
            return 3;
        return httpProbe({"--http", server.url("compare").toString()});
    }
    AudioDiagnostics tests;
    return QTest::qExec(&tests, argc, argv);
}

#include "main.moc"
