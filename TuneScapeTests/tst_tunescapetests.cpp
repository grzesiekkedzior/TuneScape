#include <QBuffer>
#include <QAudioOutput>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QLockFile>
#include <QMainWindow>
#include <QMenu>
#include <QMessageBox>
#include <QNetworkProxy>
#include <QProgressDialog>
#include <QProcess>
#include <QStandardItemModel>
#include <QTableView>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QTimer>
#include <QtTest>
#include "include/AppConfig.h"
#include "include/Country.h"
#include "include/jsonlistprocessor.h"
#include "include/trayicon.h"
#include "include/music_data/models/Track.h"
#include "include/view/TrackView.h"
#include "include/RadioBrowserApi.h"
#include "include/miniplayer.h"
#include "include/radioexplorer.h"
#include "include/radiolist.h"
#include <filesystem>
#include <functional>
#include <memory>

namespace {

class TestHttpServer
{
public:
    TestHttpServer()
    {
        QObject::connect(&server, &QTcpServer::newConnection, &server, [this] {
            auto *socket = server.nextPendingConnection();
            QObject::connect(socket, &QTcpSocket::readyRead, socket,
                             [this, socket, request = QByteArray()]() mutable {
                request += socket->readAll();
                if (request.contains("\r\n\r\n")) {
                    const auto target = request.split(' ').value(1);
                    requestHeaders.insert(target, request);
                    clients.insert(target, socket);
                    ++counts[target];
                    if (onRequest)
                        onRequest(socket, target);
                    request.clear();
                }
            });
        });
    }

    bool listen() { return server.listen(QHostAddress::LocalHost); }

    QUrl url(const QString &target) const
    {
        return QUrl(QStringLiteral("http://127.0.0.1:%1%2").arg(server.serverPort()).arg(target));
    }

    QTcpSocket *client(const QByteArray &target) const { return clients.value(target); }
    QByteArray request(const QByteArray &target) const { return requestHeaders.value(target); }
    int requestCount(const QByteArray &target) const { return counts.value(target); }
    int requestCount() const
    {
        int total = 0;
        for (int count : counts)
            total += count;
        return total;
    }

    int activeConnections() const
    {
        int count = 0;
        for (auto *socket : server.findChildren<QTcpSocket *>(QString(), Qt::FindDirectChildrenOnly))
            count += socket->state() == QAbstractSocket::ConnectedState;
        return count;
    }

    std::function<void(QTcpSocket *, const QByteArray &)> onRequest;

    static bool respond(QTcpSocket *client, const QByteArray &body, qint64 length = -1,
                        const QByteArray &status = "200 OK", const QByteArray &type = "audio/mpeg")
    {
        QByteArray response = "HTTP/1.1 " + status + "\r\nContent-Type: " + type + "\r\nConnection: close\r\n";
        if (length >= 0)
            response += "Content-Length: " + QByteArray::number(length) + "\r\n";
        response += "\r\n" + body;
        return client->write(response) == response.size();
    }

    bool respondJson(const QByteArray &target, const QByteArray &body, const QByteArray &status = "200 OK")
    {
        auto *socket = client(target);
        if (!socket || !respond(socket, body, body.size(), status, "application/json"))
            return false;
        socket->disconnectFromHost();
        return true;
    }

    bool respondImage(const QByteArray &target, const QByteArray &bytes, const QByteArray &status = "200 OK")
    {
        auto *socket = client(target);
        if (!socket || !respond(socket, bytes, bytes.size(), status, "image/png"))
            return false;
        socket->disconnectFromHost();
        return true;
    }

private:
    QHash<QByteArray, QTcpSocket *> clients;
    QHash<QByteArray, QByteArray> requestHeaders;
    QHash<QByteArray, int> counts;
    QTcpServer server;
};

QByteArray imageFixture(const QSize &size, const QColor &color)
{
    QImage image(size, QImage::Format_RGB32);
    image.fill(color);
    QByteArray bytes;
    QBuffer output(&bytes);
    output.open(QIODevice::WriteOnly);
    return image.save(&output, "PNG") ? bytes : QByteArray();
}

QColor modelIconColor(const RadioStationsModel &model, int row)
{
    const auto icon = model.data(model.index(row, 0), Qt::DecorationRole).value<QIcon>();
    const auto image = icon.pixmap(QSize(20, 20)).toImage();
    return image.isNull() ? QColor() : image.pixelColor(image.width() / 2, image.height() / 2);
}

class LimitedWritesRecorder : public StreamRecorder
{
public:
    explicit LimitedWritesRecorder(qint64 limit) : remaining(limit) {}

protected:
    qint64 writeToFile(const char *data, qint64 size) override
    {
        if (remaining == 0)
            return -1;
        const qint64 written = StreamRecorder::writeToFile(data, qMin(size, qMin(remaining, qint64(3))));
        if (written > 0)
            remaining -= written;
        return written;
    }

private:
    qint64 remaining;
};

class ControlledAudio : public RadioAudioManager
{
public:
    struct Attempt { QUrl url; quint64 id; };

    void start(const QUrl &url, quint64 id) override
    {
        attempts.append({url, id});
        active = id;
    }

    void stop() override
    {
        if (active)
            ++stops;
        active = 0;
    }

    void confirmPlaying()
    {
        emit mediaStatusChanged(active, QMediaPlayer::BufferedMedia);
        emit playbackStateChanged(active, QMediaPlayer::PlayingState);
    }

    QList<Attempt> attempts;
    quint64 active = 0;
    int stops = 0;
};

RadioStation playbackStation(const QString &name, const QUrl &url)
{
    RadioStation station;
    station.station = name;
    station.streamUrl = url.toString();
    station.codec = "MP3";
    return station;
}

QByteArray mpegStream()
{
    // MPEG-1 Layer III, stereo, 44.1 kHz at 128 kbps. Zero spectral coefficients encode silence.
    constexpr int FrameBytes = 417;
    auto frame = QByteArray(FrameBytes, '\0');
    frame.replace(0, 4, QByteArray::fromHex("fffb9000"));
    return frame.repeated(240);
}

QByteArray monoResamplingStream()
{
    // Optional reproducer for the Qt 6.7/FFmpeg crash observed in swr_convert().
    auto frame = QByteArray(417, '\0');
    frame.replace(0, 4, QByteArray::fromHex("fffd80c0"));
    return frame.repeated(240);
}

bool waitForAudio(PlaybackController &playback, RadioAudioManager &backend)
{
    const bool ready = QTest::qWaitFor([&] {
        using State = PlaybackController::State;
        const auto state = playback.state();
        if (state == State::Error || state == State::Stopped)
            return true;
        const auto players = backend.findChildren<QMediaPlayer *>();
        return state == State::Playing && players.size() == 1 && players.first()->position() >= 100;
    }, 17000) && playback.state() == PlaybackController::State::Playing;
    if (!ready) {
        qInfo() << "Audio did not advance:" << playback.state() << playback.lastError();
        for (auto *player : backend.findChildren<QMediaPlayer *>())
            qInfo() << player->mediaStatus() << player->playbackState() << player->position();
    }
    return ready;
}

QString runMediaTestInChild(const QString &name)
{
    const QTemporaryDir directory;
    if (!directory.isValid())
        return QStringLiteral("Cannot create the subprocess log directory.");
    const auto path = directory.filePath("result.txt");
    QProcess child;
    auto environment = QProcessEnvironment::systemEnvironment();
    environment.insert("TUNESCAPE_MEDIA_CHILD", "1");
    child.setProcessEnvironment(environment);
    child.start(QCoreApplication::applicationFilePath(),
                {name, "-nocrashhandler", "-o", path + ",txt"});
    if (!child.waitForStarted(5000))
        return child.errorString();
    if (!child.waitForFinished(55000)) {
        child.kill();
        child.waitForFinished(5000);
        return QStringLiteral("Media test subprocess exceeded 55 seconds.");
    }
    QFile log(path);
    if (!log.open(QIODevice::ReadOnly))
        return QStringLiteral("The media test subprocess produced no log.");
    const auto result = QString::fromUtf8(log.readAll());
    qInfo().noquote() << result;
    return child.exitStatus() == QProcess::NormalExit && child.exitCode() == 0
        ? QString() : QStringLiteral("Media subprocess failed (%1).\n%2")
                          .arg(child.exitCode()).arg(result);
}

bool writeFixture(const QString &path, const QByteArray &bytes)
{
    QFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size() && file.flush();
}

QByteArray fileContents(const QString &path)
{
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}

RadioStation favoriteStation(const QString &url = QStringLiteral("https://radio.example/Live"))
{
    RadioStation station;
    station.station = QStringLiteral("Test Radio");
    station.streamUrl = url;
    return station;
}

} // namespace

class TuneScapeTests : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testRadioListPause();
    void testPlaybackSwitchAndStop_data();
    void testPlaybackSwitchAndStop();
    void testPlaybackErrors_data();
    void testPlaybackErrors();
    void testPlaybackStallAndSystemWake();
    void testPlaybackStopDuringBuffering();
    void testPlaybackModelIdentity();
    void testPlaybackViewsAndTray();
    void testPlaybackShutdownFinalizesRecording();
    void testMediaHttpSwitchAndRelease();
    void testMediaHttpRecordingLifecycle();
    void testMediaHttpErrorsAndRecovery();
    void testRecordingForbidden_data();
    void testRecordingForbidden();
    void testLiveRadioStreams();
    void testJsonListProcessorConnection();
    void testJsonListProcessorStationLoaded();
    void testCatalogResponseErrors_data();
    void testCatalogResponseErrors();
    void testRadioStationsGenerateFullAddress();
    void testCatalogTimeout();
    void testCatalogMirrorFallback();
    void testCatalogCancellation();
    void testCatalogCancellationDuringFailover();
    void testCatalogOutOfOrder();
    void testCatalogReceiverDestruction();
    void testCatalogListsDoNotOverwriteVisibleSelection();
    void testCatalogCountrySearchAndTabChange();
    void testCatalogSearchReplacementAndCancellation();
    void testCatalogStationDetailsIgnorePreviousRequest();
    void testCatalogFailureLeavesPlaybackAndRecordingRunning();
    void testImagesShareDownloadsAndCacheAcrossViews();
    void testImageFailures_data();
    void testImageFailures();
    void testImageTimeoutAndConcurrencyLimit();
    void testImageCacheEviction();
    void testLargeImageAndHighDpi();
    void testTableImagesFollowStationIdentity();
    void testTableImagesLoadVisibleRowsAndCancelOnClose();
    void testAppConfigCheckFileIsExists();
    void testAppConfigCheckBoolState();
    void testAppConfigChangeBoolState();
    void testMiniPlayerIsVisible();
    void testFavoriteAddAndRemove();
    void testFavoriteRoundTrip_data();
    void testFavoriteRoundTrip();
    void testFavoriteDuplicateAndUrlIdentity();
    void testFavoriteUuidIdentity();
    void testFavoriteM3UImportExport();
    void testFavoriteInvalidImport_data();
    void testFavoriteInvalidImport();
    void testFavoriteImportIntoItself();
    void testFavoriteWriteFailure();
    void testFavoriteLegacyReadAndMigration();
    void testFavoriteViewOnlyUpdatesAfterSave();
    void testStopCountryStationFromContextMenu();
    void testRecordingStartAndStop();
    void testRecordingOpenFailure();
    void testRecordingNameCollision();
    void testRecordingSanitizesName();
    void testRecordingRestartsWithoutMixingSessions();
    void testRecordingConnectionEnds_data();
    void testRecordingConnectionEnds();
    void testRecordingShortWrites_data();
    void testRecordingShortWrites();
    void testStationChangeStopsRecording();
    void testRecorderDestructionClosesSession();

private:
    QTemporaryDir workingDirectory;
    QString originalWorkingDirectory;
    QNetworkProxy originalProxy;
};

void TuneScapeTests::initTestCase()
{
    originalWorkingDirectory = QDir::currentPath();
    originalProxy = QNetworkProxy::applicationProxy();
    QVERIFY(workingDirectory.isValid());
    QVERIFY(QDir::setCurrent(workingDirectory.path()));
    QNetworkProxy::setApplicationProxy(QNetworkProxy::NoProxy);
}

void TuneScapeTests::cleanupTestCase()
{
    QNetworkProxy::setApplicationProxy(originalProxy);
    QVERIFY(QDir::setCurrent(originalWorkingDirectory));
    if (workingDirectory.isValid())
        QVERIFY(workingDirectory.remove());
}

void TuneScapeTests::testRadioListPause()
{
    TestHttpServer http;
    QVERIFY(http.listen());
    RadioBrowserApi api({http.url("/")}, 1500);
    QMainWindow window;
    Ui::MainWindow ui;
    ui.setupUi(&window);
    FavoriteManager favorites;
    ControlledAudio audio;
    PlaybackController playback(audio);
    RadioList radioList(&ui, &favorites, &api, &playback);
    using State = PlaybackController::State;
    QSignalSpy states(&playback, &PlaybackController::stateChanged);
    QSignalSpy sessions(&playback, &PlaybackController::sessionChanged);
    const auto station = playbackStation("Station", http.url("/metadata"));

    radioList.radioStationsModel->setStations({station});
    ui.playPause->click();
    QCOMPARE(playback.state(), State::Connecting);
    audio.confirmPlaying();
    QCOMPARE(playback.state(), State::Playing);
    radioList.miniPlayerWindow().getMui()->play->click();
    QCOMPARE(playback.state(), State::Paused);
    QCOMPARE(playback.streamUrl(), QUrl(station.streamUrl));
    QCOMPARE(audio.stops, 1);

    ui.playPause->click();
    QCOMPARE(playback.state(), State::Connecting);
    QCOMPARE(audio.attempts.size(), 2);
    QCOMPARE(audio.attempts.last().url, QUrl(station.streamUrl));
    QCOMPARE(sessions.size(), 1);
    audio.confirmPlaying();
    ui.stop->click();
    QCOMPARE(playback.state(), State::Stopped);
    QCOMPARE(states.size(), 6);
    QCOMPARE(sessions.size(), 2);
    QVERIFY(!playback.station());
}

void TuneScapeTests::testPlaybackSwitchAndStop_data()
{
    QTest::addColumn<bool>("switchStation");
    QTest::addColumn<bool>("confirmNewStation");
    QTest::newRow("play-stop") << false << true;
    QTest::newRow("play-a-play-b") << true << true;
    QTest::newRow("quick-a-b-stop") << true << false;
}

void TuneScapeTests::testPlaybackSwitchAndStop()
{
    QFETCH(bool, switchStation);
    QFETCH(bool, confirmNewStation);
    ControlledAudio audio;
    PlaybackController playback(audio);
    using State = PlaybackController::State;
    QSignalSpy sessions(&playback, &PlaybackController::sessionChanged);
    QSignalSpy states(&playback, &PlaybackController::stateChanged);
    auto first = playbackStation("A", QUrl("http://127.0.0.1/shared"));
    first.stationUuid = QUuid::createUuid();
    auto second = first;
    second.station = "B";
    second.stationUuid = QUuid::createUuid();

    QCOMPARE(playback.state(), State::Idle);
    playback.stop();
    QCOMPARE(sessions.size(), 0);
    playback.play(first);
    QCOMPARE(playback.state(), State::Connecting);
    const auto originalAttempt = audio.active;
    const auto originalGeneration = playback.generation();
    playback.play(first);
    QCOMPARE(audio.attempts.size(), 1);
    QCOMPARE(playback.generation(), originalGeneration);
    if (switchStation) {
        playback.play(second);
        QCOMPARE(playback.state(), State::Connecting);
        QVERIFY(playback.generation() > originalGeneration);
        QCOMPARE(playback.station()->stationUuid, second.stationUuid);
        emit audio.playbackError(originalAttempt, "Obsolete error");
        emit audio.mediaStatusChanged(originalAttempt, QMediaPlayer::EndOfMedia);
        emit audio.playbackStateChanged(originalAttempt, QMediaPlayer::PlayingState);
        QCOMPARE(playback.state(), State::Connecting);
        QVERIFY(playback.lastError().isEmpty());
    }
    const auto currentAttempt = audio.active;
    if (confirmNewStation) {
        emit audio.playbackStateChanged(currentAttempt, QMediaPlayer::PlayingState);
        QCOMPARE(playback.state(), State::Connecting);
        emit audio.mediaStatusChanged(currentAttempt, QMediaPlayer::BufferedMedia);
        QCOMPARE(playback.state(), State::Playing);
    }
    playback.stop();
    playback.stop();
    emit audio.playbackError(currentAttempt, "Error after Stop");
    emit audio.mediaStatusChanged(currentAttempt, QMediaPlayer::BufferedMedia);
    QCOMPARE(playback.state(), State::Stopped);
    QVERIFY(!playback.station());
    QVERIFY(playback.streamUrl().isEmpty());
    QCOMPARE(audio.active, 0);
    QCOMPARE(audio.stops, switchStation ? 2 : 1);
    QCOMPARE(sessions.size(), switchStation ? 3 : 2);
    QCOMPARE(states.last().first().value<State>(), State::Stopped);
}

void TuneScapeTests::testPlaybackErrors_data()
{
    QTest::addColumn<int>("phase");
    QTest::addColumn<QString>("message");
    using State = PlaybackController::State;
    QTest::newRow("connecting-resource-error") << int(State::Connecting) << QString("Stream unavailable");
    QTest::newRow("buffering-decoder-error") << int(State::Buffering) << QString("Unsupported codec");
    QTest::newRow("playing-network-error") << int(State::Playing) << QString("Connection lost");
}

void TuneScapeTests::testPlaybackErrors()
{
    QFETCH(int, phase);
    QFETCH(QString, message);
    ControlledAudio audio;
    PlaybackController playback(audio);
    using State = PlaybackController::State;
    QSignalSpy sessions(&playback, &PlaybackController::sessionChanged);
    QSignalSpy states(&playback, &PlaybackController::stateChanged);
    const auto station = playbackStation("Faulty", QUrl("http://127.0.0.1/faulty"));
    playback.play(station);
    if (phase == int(State::Buffering))
        emit audio.mediaStatusChanged(audio.active, QMediaPlayer::BufferingMedia);
    if (phase == int(State::Playing))
        audio.confirmPlaying();
    QCOMPARE(int(playback.state()), phase);
    const auto attempt = audio.active;
    emit audio.playbackError(attempt, message);
    QCOMPARE(playback.state(), State::Error);
    QCOMPARE(playback.lastError(), message);
    QCOMPARE(playback.streamUrl(), QUrl(station.streamUrl));
    QVERIFY(!playback.hasActiveSession());
    QCOMPARE(audio.active, 0);
    QCOMPARE(sessions.size(), 2);
    QCOMPARE(states.last().first().value<State>(), State::Error);

    playback.toggle();
    QCOMPARE(playback.state(), State::Connecting);
    QVERIFY(playback.lastError().isEmpty());
    QVERIFY(audio.active != attempt);
    emit audio.playbackError(attempt, "Late error during retry");
    audio.confirmPlaying();
    QCOMPARE(playback.state(), State::Playing);
}

void TuneScapeTests::testPlaybackStallAndSystemWake()
{
    ControlledAudio audio;
    PlaybackController playback(audio);
    using State = PlaybackController::State;
    const auto station = playbackStation("Radio", QUrl("http://127.0.0.1/live"));
    playback.play(station);
    audio.confirmPlaying();
    const auto original = audio.active;
    const auto generation = playback.generation();
    emit audio.mediaStatusChanged(original, QMediaPlayer::StalledMedia);
    QCOMPARE(playback.state(), State::Buffering);
    emit audio.mediaStatusChanged(original, QMediaPlayer::BufferingMedia);
    QCOMPARE(playback.state(), State::Playing);
    emit audio.systemSuspended();
    QCOMPARE(playback.state(), State::Connecting);
    QCOMPARE(audio.active, 0);
    emit audio.playbackError(original, "Stale during system sleep");
    emit audio.systemResumed();
    emit audio.systemResumed();
    QCOMPARE(audio.attempts.size(), 2);
    QCOMPARE(playback.generation(), generation);
    audio.confirmPlaying();
    emit audio.systemSuspended();
    playback.pause();
    emit audio.systemResumed();
    QCOMPARE(playback.state(), State::Paused);
    QCOMPARE(audio.attempts.size(), 2);
    playback.resume();
    audio.confirmPlaying();
    emit audio.playbackStateChanged(audio.active, QMediaPlayer::StoppedState);
    QTRY_COMPARE(playback.state(), State::Error);
    QVERIFY(!playback.lastError().isEmpty());
}

void TuneScapeTests::testPlaybackStopDuringBuffering()
{
    ControlledAudio audio;
    PlaybackController playback(audio);
    using State = PlaybackController::State;
    QSignalSpy sessions(&playback, &PlaybackController::sessionChanged);
    QSignalSpy states(&playback, &PlaybackController::stateChanged);
    playback.play(playbackStation("Buffering", QUrl("http://127.0.0.1/buffering")));
    const auto attempt = audio.active;
    emit audio.mediaStatusChanged(attempt, QMediaPlayer::BufferingMedia);
    QCOMPARE(playback.state(), State::Buffering);
    playback.stop();
    QCOMPARE(playback.state(), State::Stopped);
    QCOMPARE(audio.active, 0);
    QCOMPARE(audio.stops, 1);
    QCOMPARE(sessions.size(), 2);
    QCOMPARE(states.size(), 3);
    emit audio.mediaStatusChanged(attempt, QMediaPlayer::BufferedMedia);
    emit audio.playbackStateChanged(attempt, QMediaPlayer::PlayingState);
    emit audio.playbackError(attempt, "Late buffering error");
    QCOMPARE(playback.state(), State::Stopped);
    QCOMPARE(states.size(), 3);
    QVERIFY(playback.lastError().isEmpty());
    QVERIFY(!playback.station());
}

void TuneScapeTests::testPlaybackModelIdentity()
{
    const QTemporaryDir directory;
    QVERIFY(directory.isValid());
    TestHttpServer http;
    QVERIFY(http.listen());
    RadioBrowserApi catalog({http.url("/")}, 1500);
    QMainWindow window;
    Ui::MainWindow ui;
    ui.setupUi(&window);
    RadioExplorer explorer(&ui);
    explorer.createMenu();
    FavoriteManager favorites(directory.filePath("favorites.txt"));
    Theme theme(*qApp);
    ControlledAudio audio;
    PlaybackController playback(audio);
    RadioList list(&ui, &favorites, &catalog, &playback);
    list.setTheme(&theme);
    const auto one = playbackStation("One", http.url("/one"));
    const auto two = playbackStation("Two", http.url("/two"));
    list.setFavorite(one, true);
    list.setFavorite(two, true);
    list.setFavoriteLibrary();

    emit ui.tableView->activated({});
    QStandardItemModel foreign(1, 1);
    emit ui.tableView->doubleClicked(foreign.index(0, 0));
    QVERIFY(audio.attempts.isEmpty());
    const auto index = list.radioStationsModel->index(0, 0);
    emit ui.tableView->activated(index);
    emit ui.tableView->doubleClicked(index);
    audio.confirmPlaying();
    auto *selection = qobject_cast<CustomColorDelegate *>(ui.tableView->itemDelegate());
    QVERIFY(selection);
    QCOMPARE(selection->playingRow(), 0);
    QCOMPARE(audio.attempts.size(), 1);
    const auto generation = playback.generation();

    list.radioStationsModel->setStations({two, one});
    QCOMPARE(selection->playingRow(), 1);
    list.selectPlaylist(Stations::TOP);
    QCOMPARE(selection->playingRow(), -1);
    list.setFavoriteLibrary();
    QCOMPARE(selection->playingRow(), 0);
    list.setFavorite(one, false);
    QCOMPARE(selection->playingRow(), -1);
    QCOMPARE(playback.streamUrl(), QUrl(one.streamUrl));
    QCOMPARE(playback.generation(), generation);
    QCOMPARE(playback.state(), PlaybackController::State::Playing);
    QCOMPARE(audio.attempts.size(), 1);
}

void TuneScapeTests::testPlaybackViewsAndTray()
{
    TestHttpServer http;
    QVERIFY(http.listen());
    RadioBrowserApi catalog({http.url("/")}, 1500);
    QMainWindow window;
    Ui::MainWindow ui;
    ui.setupUi(&window);
    RadioExplorer explorer(&ui);
    explorer.createMenu();
    FavoriteManager favorites;
    ControlledAudio audio;
    PlaybackController playback(audio);
    RadioList list(&ui, &favorites, &catalog, &playback);
    TrayIcon tray(&ui, window);
    tray.setRadioList(&list);
    auto *menu = tray.getSystemTrayIcon()->contextMenu();
    QVERIFY(menu);
    auto *playPause = menu->actions().value(0);
    QVERIFY(playPause);
    QWidget *panel = nullptr;
    for (auto *widget : QApplication::topLevelWidgets()) {
        if (widget->objectName() == "nowPlayingWindow")
            panel = widget;
    }
    QVERIFY(panel);
    auto *status = panel->findChild<QLabel *>("nowPlayingStatus");
    auto *name = panel->findChild<QLabel *>("nowPlayingStation");
    QVERIFY(status && name);

    const auto one = playbackStation("One", http.url("/one"));
    const auto two = playbackStation("Two", http.url("/two"));
    list.allStations.resize(Stations::SEARCH + 1);
    list.allStations[Stations::TOP] = {one, two};
    tray.loadTrayList(Stations::TOP);
    QMenu *top = nullptr;
    for (auto *action : menu->actions()) {
        if (action->menu() && action->text() == TrayIcon::tr("Top"))
            top = action->menu();
    }
    QVERIFY(top);
    auto *oneAction = top->actions().value(0);
    QVERIFY(oneAction);
    list.allStations[Stations::TOP] = {two, one};
    oneAction->trigger();
    QCOMPARE(playback.streamUrl(), QUrl(one.streamUrl));
    QCOMPARE(name->text(), one.station);
    QCOMPARE(status->text(), TrayIcon::tr("Connecting"));
    QCOMPARE(playPause->text(), TrayIcon::tr("Pause"));
    QCOMPARE(ui.playPause->toolTip(), QString("Pause"));
    QCOMPARE(list.miniPlayerWindow().getMui()->play->toolTip(), QString("Pause"));
    QVERIFY(oneAction->isChecked());
    audio.confirmPlaying();
    QCOMPARE(status->text(), TrayIcon::tr("Playing"));
    list.selectPlaylist(Stations::POPULAR);
    QCOMPARE(name->text(), one.station);
    QCOMPARE(audio.attempts.size(), 1);
    playPause->trigger();
    QCOMPARE(status->text(), TrayIcon::tr("Paused"));
    QCOMPARE(ui.playPause->toolTip(), QString("Play"));
    QCOMPARE(list.miniPlayerWindow().getMui()->play->toolTip(), QString("Play"));
    list.miniPlayerWindow().getMui()->play->click();
    QCOMPARE(status->text(), TrayIcon::tr("Connecting"));
    emit audio.playbackError(audio.active, "Device failure");
    QCOMPARE(status->text(), TrayIcon::tr("Playback failed"));
    QCOMPARE(status->toolTip(), QString("Device failure"));
    QVERIFY(ui.statusbar->currentMessage().contains("Device failure"));
    QVERIFY(!oneAction->isChecked());
    ui.stop->click();
    QCOMPARE(name->text(), TrayIcon::tr("Nothing playing"));
    QCOMPARE(playPause->text(), TrayIcon::tr("Play"));
}

void TuneScapeTests::testPlaybackShutdownFinalizesRecording()
{
    const QTemporaryDir directory;
    QVERIFY(directory.isValid());
    TestHttpServer http;
    QVERIFY(http.listen());
    RadioBrowserApi catalog({http.url("/")}, 1500);
    QMainWindow window;
    Ui::MainWindow ui;
    ui.setupUi(&window);
    FavoriteManager favorites;
    ControlledAudio audio;
    PlaybackController playback(audio);
    auto list = std::make_unique<RadioList>(&ui, &favorites, &catalog, &playback);
    QSignalSpy states(&playback, &PlaybackController::stateChanged);
    TrackView trackView(&ui);
    connect(&playback, &PlaybackController::sessionChanged, &trackView, &TrackView::clear);
    playback.play(playbackStation("Station", http.url("/metadata")));
    audio.confirmPlaying();
    auto &recorder = list->getStreamRecorder();
    recorder.startRecording(http.url("/recording"), "Station", "MP3", directory.path());
    music_data::Track track;
    track.artist = "Artist";
    track.title = "Song";
    track.coverUrl = http.url("/pending-cover").toString();
    trackView.setTrack(track);
    QTRY_VERIFY(http.client("/metadata") && http.client("/recording") && http.client("/pending-cover"));
    QVERIFY(http.respond(http.client("/recording"), "audio"));
    QTRY_COMPARE(recorder.bytesWritten(), 5);
    const auto path = recorder.filePath();
    list.reset();
    QCOMPARE(playback.state(), PlaybackController::State::Stopped);
    QCOMPARE(states.last().first().value<PlaybackController::State>(), PlaybackController::State::Stopped);
    QCOMPARE(audio.active, 0);
    QCOMPARE(fileContents(path), QByteArray("audio"));
    QVERIFY(ui.title->text().isEmpty());
    QTRY_COMPARE(http.client("/recording")->state(), QAbstractSocket::UnconnectedState);
    QTRY_COMPARE(http.client("/metadata")->state(), QAbstractSocket::UnconnectedState);
    QTRY_COMPARE(http.client("/pending-cover")->state(), QAbstractSocket::UnconnectedState);
}

void TuneScapeTests::testMediaHttpSwitchAndRelease()
{
    if (qEnvironmentVariableIntValue("TUNESCAPE_TEST_MEDIA") != 1)
        QSKIP("Set TUNESCAPE_TEST_MEDIA=1 to exercise the installed multimedia backend.");
    if (qEnvironmentVariableIntValue("TUNESCAPE_MEDIA_CHILD") != 1) {
        const auto error = runMediaTestInChild(QString::fromLatin1(QTest::currentTestFunction()));
        QVERIFY2(error.isEmpty(), qPrintable(error));
        return;
    }

    TestHttpServer http;
    QVERIFY(http.listen());
    const auto audio = qEnvironmentVariableIntValue("TUNESCAPE_TEST_MONO") == 1
        ? monoResamplingStream() : mpegStream();
    http.onRequest = [&](QTcpSocket *socket, const QByteArray &target) {
        if (target != "/pending")
            http.respond(socket, audio, -1, "200 OK", "audio/mpeg");
    };
    RadioAudioManager backend;
    PlaybackController playback(backend);
    playback.setVolume(0);
    using State = PlaybackController::State;
    const auto a = playbackStation("A", http.url("/a.mp3"));
    const auto b = playbackStation("B", http.url("/b.mp3"));
    QSignalSpy errors(&backend, &RadioAudioManager::playbackError);

    for (const auto &station : {a, b, a}) {
        const auto previous = backend.findChild<QMediaPlayer *>();
        const QPointer<QMediaPlayer> retired(previous);
        playback.play(station);
        QVERIFY2(waitForAudio(playback, backend), qPrintable(playback.lastError()));
        QTRY_COMPARE(http.activeConnections(), 1);
        QVERIFY(retired.isNull());
        QCOMPARE(playback.streamUrl(), QUrl(station.streamUrl));
        QCOMPARE(backend.findChildren<QAudioOutput *>().size(), 1);
    }
    QCOMPARE(http.requestCount("/a.mp3"), 2);
    QCOMPARE(http.requestCount("/b.mp3"), 1);
    const auto generation = playback.generation();
    for (int cycle = 0; cycle < 6; ++cycle) {
        const QPointer<QMediaPlayer> retired(backend.findChild<QMediaPlayer *>());
        playback.pause();
        QCOMPARE(playback.state(), State::Paused);
        QTRY_VERIFY(retired.isNull());
        QCOMPARE(backend.findChildren<QAudioOutput *>().size(), 1);
        QTRY_COMPARE(http.activeConnections(), 0);
        playback.resume();
        QVERIFY2(waitForAudio(playback, backend), qPrintable(playback.lastError()));
        QTRY_COMPARE(http.activeConnections(), 1);
        QCOMPARE(playback.generation(), generation);
    }
    QCOMPARE(http.requestCount("/a.mp3"), 8);

    playback.play(playbackStation("Pending", http.url("/pending")));
    QTRY_VERIFY(http.client("/pending"));
    QCOMPARE(playback.state(), State::Connecting);
    const QPointer<QMediaPlayer> pendingPlayer(backend.findChild<QMediaPlayer *>());
    playback.play(b);
    playback.play(a);
    QVERIFY2(waitForAudio(playback, backend), qPrintable(playback.lastError()));
    QTRY_VERIFY(pendingPlayer.isNull());
    QTRY_COMPARE(http.client("/pending")->state(), QAbstractSocket::UnconnectedState);
    QCOMPARE(playback.streamUrl(), QUrl(a.streamUrl));
    playback.stop();
    playback.stop();
    QTRY_COMPARE(backend.findChildren<QMediaPlayer *>().size(), 0);
    QTRY_COMPARE(http.activeConnections(), 0);
    QCOMPARE(backend.findChildren<QAudioOutput *>().size(), 1);
    QCOMPARE(errors.size(), 0);

    auto shutdownBackend = std::make_unique<RadioAudioManager>();
    PlaybackController shutdownPlayback(*shutdownBackend);
    shutdownPlayback.setVolume(0);
    shutdownPlayback.play(a);
    QVERIFY2(waitForAudio(shutdownPlayback, *shutdownBackend), qPrintable(shutdownPlayback.lastError()));
    QStringList destruction;
    const QPointer<QMediaPlayer> activePlayer(shutdownBackend->findChild<QMediaPlayer *>());
    const QPointer<QAudioOutput> sharedOutput(shutdownBackend->findChild<QAudioOutput *>());
    QVERIFY(activePlayer && sharedOutput);
    connect(activePlayer, &QObject::destroyed, this, [&] { destruction.append("player"); });
    connect(sharedOutput, &QObject::destroyed, this, [&] { destruction.append("output"); });
    shutdownBackend.reset();
    QVERIFY(activePlayer.isNull());
    QVERIFY(sharedOutput.isNull());
    QCOMPARE(destruction, (QStringList{"player", "output"}));
    QTRY_COMPARE(http.activeConnections(), 0);
    shutdownPlayback.stop();
}

void TuneScapeTests::testMediaHttpRecordingLifecycle()
{
    if (qEnvironmentVariableIntValue("TUNESCAPE_TEST_MEDIA") != 1)
        QSKIP("Set TUNESCAPE_TEST_MEDIA=1 to exercise the installed multimedia backend.");
    if (qEnvironmentVariableIntValue("TUNESCAPE_MEDIA_CHILD") != 1) {
        const auto error = runMediaTestInChild(QString::fromLatin1(QTest::currentTestFunction()));
        QVERIFY2(error.isEmpty(), qPrintable(error));
        return;
    }

    const QTemporaryDir recordings;
    QVERIFY(recordings.isValid());
    TestHttpServer http;
    QVERIFY(http.listen());
    const auto audio = mpegStream();
    http.onRequest = [&](QTcpSocket *socket, const QByteArray &target) {
        if (target.startsWith("/json/")) {
            http.respond(socket, "[]", 2, "200 OK", "application/json");
            socket->disconnectFromHost();
        } else {
            http.respond(socket, audio, -1, "200 OK", "audio/mpeg");
        }
    };
    RadioBrowserApi catalog({http.url("/")}, 1500);
    RadioAudioManager backend;
    PlaybackController playback(backend);
    QMainWindow window;
    Ui::MainWindow ui;
    ui.setupUi(&window);
    FavoriteManager favorites(recordings.filePath("favorites.txt"));
    RadioList list(&ui, &favorites, &catalog, &playback);
    playback.setVolume(0);
    using State = PlaybackController::State;
    const auto a = playbackStation("A", http.url("/a.mp3"));
    const auto b = playbackStation("B", http.url("/b.mp3"));
    playback.play(a);
    QVERIFY2(waitForAudio(playback, backend), qPrintable(playback.lastError()));
    QTRY_COMPARE(http.activeConnections(), 2); // Audio and ICY.

    auto &recorder = list.getStreamRecorder();
    recorder.startRecording(playback.streamUrl(), a.station, "mp3", recordings.path());
    QTRY_COMPARE(recorder.bytesWritten(), audio.size());
    QTRY_COMPARE(http.activeConnections(), 3);
    auto *recordingSocket = http.client("/a.mp3");
    ui.playPause->click();
    QCOMPARE(playback.state(), State::Paused);
    QTRY_COMPARE(http.activeConnections(), 2); // ICY and recording survive Pause.
    const QByteArray silence = audio.left(417 * 10);
    QCOMPARE(recordingSocket->write(silence), silence.size());
    QTRY_COMPARE(recorder.bytesWritten(), audio.size() + silence.size());
    const auto pausedFile = recorder.filePath();
    ui.stop->click();
    QTRY_COMPARE(http.activeConnections(), 0);
    QCOMPARE(fileContents(pausedFile), audio + silence);

    playback.play(a);
    QVERIFY2(waitForAudio(playback, backend), qPrintable(playback.lastError()));
    recorder.startRecording(playback.streamUrl(), a.station, "mp3", recordings.path());
    QTRY_COMPARE(recorder.bytesWritten(), audio.size());
    const auto switchedFile = recorder.filePath();
    QVERIFY(switchedFile != pausedFile);
    playback.play(b);
    QCOMPARE(recorder.state(), StreamRecorder::State::Idle);
    QCOMPARE(fileContents(switchedFile), audio);
    QCOMPARE(fileContents(pausedFile), audio + silence);
    QVERIFY2(waitForAudio(playback, backend), qPrintable(playback.lastError()));
    QTRY_COMPARE(http.activeConnections(), 2);
    playback.stop();
    QTRY_COMPARE(http.activeConnections(), 0);
    QTRY_COMPARE(backend.findChildren<QMediaPlayer *>().size(), 0);

    RadioAudioManager fileBackend;
    PlaybackController savedAudio(fileBackend);
    savedAudio.setVolume(0);
    for (const auto &path : {pausedFile, switchedFile}) {
        savedAudio.play(playbackStation("Recorded", QUrl::fromLocalFile(path)));
        QVERIFY2(waitForAudio(savedAudio, fileBackend), qPrintable(savedAudio.lastError()));
        savedAudio.stop();
    }
}

void TuneScapeTests::testMediaHttpErrorsAndRecovery()
{
    if (qEnvironmentVariableIntValue("TUNESCAPE_TEST_MEDIA") != 1)
        QSKIP("Set TUNESCAPE_TEST_MEDIA=1 to exercise the installed multimedia backend.");
    if (qEnvironmentVariableIntValue("TUNESCAPE_MEDIA_CHILD") != 1) {
        const auto error = runMediaTestInChild(QString::fromLatin1(QTest::currentTestFunction()));
        QVERIFY2(error.isEmpty(), qPrintable(error));
        return;
    }

    TestHttpServer http;
    QVERIFY(http.listen());
    const auto audio = mpegStream();
    http.onRequest = [&](QTcpSocket *socket, const QByteArray &target) {
        if (target == "/404" || target == "/503") {
            http.respond(socket, {}, 0, target == "/404" ? "404 Not Found" : "503 Service Unavailable");
            socket->disconnectFromHost();
        } else if (target != "/timeout") {
            http.respond(socket, audio, audio.size() * 10, "200 OK", "audio/mpeg");
        }
    };
    RadioAudioManager backend;
    PlaybackController playback(backend);
    playback.setVolume(0);
    using State = PlaybackController::State;
    auto invalid = playbackStation("Invalid", {});
    invalid.streamUrl = "http://[invalid";
    playback.play(invalid);
    QCOMPARE(playback.state(), State::Error);
    QVERIFY(backend.findChildren<QMediaPlayer *>().isEmpty());

    for (const auto &path : {"/404", "/503", "/timeout"}) {
        playback.play(playbackStation("Failing", http.url(path)));
        QTRY_VERIFY(http.client(path));
        QTRY_COMPARE_WITH_TIMEOUT(playback.state(), State::Error, 18000);
        QVERIFY(!playback.lastError().isEmpty());
        if (QByteArray(path) == "/timeout")
            qInfo() << "Timeout result:" << playback.lastError();
        QTRY_COMPARE(http.activeConnections(), 0);
        QTRY_VERIFY(backend.findChildren<QMediaPlayer *>().isEmpty());
    }

    const auto station = playbackStation("Recoverable", http.url("/live.mp3"));
    playback.play(station);
    QVERIFY2(waitForAudio(playback, backend), qPrintable(playback.lastError()));
    http.client("/live.mp3")->abort();
    // Some backends treat premature EOF as EndOfMedia, others report a network error.
    QTRY_VERIFY_WITH_TIMEOUT(playback.state() == State::Error || playback.state() == State::Stopped, 12000);
    qInfo() << "Backend state after truncated HTTP response:" << playback.state() << playback.lastError();
    QTRY_VERIFY(backend.findChildren<QMediaPlayer *>().isEmpty());
    playback.play(station);
    QVERIFY2(waitForAudio(playback, backend), qPrintable(playback.lastError()));
    playback.stop();
    QTRY_COMPARE(http.activeConnections(), 0);
}

void TuneScapeTests::testRecordingForbidden_data()
{
    QTest::addColumn<bool>("redirect");
    QTest::newRow("direct-403") << false;
    QTest::newRow("redirect-to-403") << true;
}

void TuneScapeTests::testRecordingForbidden()
{
    QFETCH(bool, redirect);
    const QTemporaryDir directory;
    QVERIFY(directory.isValid());
    TestHttpServer server;
    QVERIFY(server.listen());
    server.onRequest = [&](QTcpSocket *socket, const QByteArray &target) {
        if (target == "/redirect") {
            socket->write("HTTP/1.1 302 Found\r\nLocation: /denied\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");
        } else {
            const QByteArray body = "<html>Recording denied</html>";
            server.respond(socket, body, body.size(), "403 Forbidden", "text/html");
        }
        socket->disconnectFromHost();
    };
    const auto existing = directory.filePath("existing.mp3");
    QVERIFY(writeFixture(existing, "previous recording"));
    RadioBrowserApi catalog({}, 1500);
    QMainWindow window;
    Ui::MainWindow ui;
    ui.setupUi(&window);
    FavoriteManager favorites(directory.filePath("favorites.txt"));
    RadioList list(&ui, &favorites, &catalog);
    auto &recorder = list.getStreamRecorder();
    QSignalSpy failed(&recorder, &StreamRecorder::recordingFailed);
    QSignalSpy finished(&recorder, &StreamRecorder::recordingFinished);
    recorder.startRecording(server.url(redirect ? "/redirect" : "/denied"),
                            "Denied", "mp3", directory.path());
    QTRY_COMPARE(failed.size(), 1);
    QCOMPARE(recorder.state(), StreamRecorder::State::Idle);
    QCOMPARE(finished.size(), 0);
    QCOMPARE(recorder.bytesWritten(), 0);
    QVERIFY(failed.first().first().toString().contains("Forbidden"));
    QVERIFY(failed.first().first().toString().contains("HTTP 403"));
    QCOMPARE(QFileInfo(recorder.filePath()).size(), 0);
    QVERIFY(fileContents(recorder.filePath()).isEmpty());
    QCOMPARE(fileContents(existing), QByteArray("previous recording"));
    QVERIFY(ui.statusbar->currentMessage().contains("Forbidden"));
    QCOMPARE(ui.record->toolTip(), RadioList::tr("Start recording"));
    QTRY_COMPARE(server.activeConnections(), 0);
    QCOMPARE(server.requestCount("/denied"), 1);
    QCOMPARE(server.requestCount("/redirect"), redirect ? 1 : 0);
    QVERIFY(server.request("/denied").contains("User-Agent: TuneScape/6.0\r\n"));
    recorder.stopRecording();
    QCOMPARE(failed.size(), 1);
    QCOMPARE(finished.size(), 0);
}

void TuneScapeTests::testLiveRadioStreams()
{
    const QUrl first(qEnvironmentVariable("TUNESCAPE_LIVE_STREAM_A"));
    const QUrl second(qEnvironmentVariable("TUNESCAPE_LIVE_STREAM_B"));
    if (first.isEmpty() || second.isEmpty())
        QSKIP("Supply TUNESCAPE_LIVE_STREAM_A and TUNESCAPE_LIVE_STREAM_B to test live radio.");
    for (const auto &url : {first, second}) {
        QVERIFY(url.isValid() && !url.host().isEmpty());
        QVERIFY(url.scheme() == "http" || url.scheme() == "https");
    }
    QVERIFY(first != second);
    const QTemporaryDir directory;
    QVERIFY(directory.isValid());
    RadioBrowserApi catalog({}, 1500);
    RadioAudioManager backend;
    PlaybackController playback(backend);
    QMainWindow window;
    Ui::MainWindow ui;
    ui.setupUi(&window);
    FavoriteManager favorites(directory.filePath("favorites.txt"));
    RadioList list(&ui, &favorites, &catalog, &playback);
    playback.setVolume(0);
    using State = PlaybackController::State;
    const auto a = playbackStation("A", first);
    const auto b = playbackStation("B", second);
    for (const auto &station : {a, b, a}) {
        const QPointer<QMediaPlayer> retired(backend.findChild<QMediaPlayer *>());
        playback.play(station);
        QVERIFY2(waitForAudio(playback, backend), qPrintable(playback.lastError()));
        QVERIFY(retired.isNull());
        QCOMPARE(playback.streamUrl(), QUrl(station.streamUrl));
        qInfo() << "Decoded live audio:" << station.station << QUrl(station.streamUrl).scheme();
    }
    auto &recording = list.getStreamRecorder();
    QSignalSpy failures(&recording, &StreamRecorder::recordingFailed);
    const auto recordingError = [&] {
        return failures.isEmpty() ? QByteArray("No bytes received before test timeout")
                                 : failures.last().first().toString().toUtf8();
    };
    recording.startRecording(playback.streamUrl(), a.station, "mp3", directory.path());
    QVERIFY2(QTest::qWaitFor([&] { return recording.bytesWritten() > 0 || !failures.isEmpty(); }, 10000)
             && recording.bytesWritten() > 0, recordingError().constData());
    const auto firstRecording = recording.filePath();
    for (int cycle = 0; cycle < 2; ++cycle) {
        playback.pause();
        QCOMPARE(playback.state(), State::Paused);
        QTRY_VERIFY(backend.findChildren<QMediaPlayer *>().isEmpty());
        const auto pausedBytes = recording.bytesWritten();
        QTRY_VERIFY_WITH_TIMEOUT(recording.bytesWritten() > pausedBytes, 10000);
        playback.resume();
        QVERIFY2(waitForAudio(playback, backend), qPrintable(playback.lastError()));
    }
    playback.play(b);
    QCOMPARE(recording.state(), StreamRecorder::State::Idle);
    const auto firstBytes = recording.bytesWritten();
    QCOMPARE(QFileInfo(firstRecording).size(), firstBytes);
    QVERIFY2(waitForAudio(playback, backend), qPrintable(playback.lastError()));
    recording.startRecording(playback.streamUrl(), b.station, "mp3", directory.path());
    QVERIFY2(QTest::qWaitFor([&] { return recording.bytesWritten() > 0 || !failures.isEmpty(); }, 10000)
             && recording.bytesWritten() > 0, recordingError().constData());
    const auto secondRecording = recording.filePath();
    playback.stop();
    QCOMPARE(playback.state(), State::Stopped);
    QCOMPARE(recording.state(), StreamRecorder::State::Idle);
    QVERIFY(recording.bytesWritten() > 0);
    QCOMPARE(QFileInfo(secondRecording).size(), recording.bytesWritten());
    QCOMPARE(QFileInfo(firstRecording).size(), firstBytes);
    QTRY_VERIFY(backend.findChildren<QMediaPlayer *>().isEmpty());
    QCOMPARE(backend.findChildren<QAudioOutput *>().size(), 1);
}

void TuneScapeTests::testJsonListProcessorConnection()
{
    TestHttpServer http;
    QVERIFY(http.listen());
    RadioBrowserApi api({http.url("/")}, 1500);
    QList<RadioBrowserApi::Result> results;
    api.request(QUrl("stations"), this, [&](auto, const auto &result) { results.append(result); });
    QVERIFY(results.isEmpty());
    QTRY_VERIFY(http.client("/stations"));
    QVERIFY(http.respondJson("/stations", "[]"));
    QTRY_COMPARE(results.size(), 1);
    QCOMPARE(results.first().status, RadioBrowserApi::Status::EmptyResult);
    QVERIFY(results.first().data.isEmpty());
}

void TuneScapeTests::testJsonListProcessorStationLoaded()
{
    TestHttpServer http;
    QVERIFY(http.listen());
    RadioBrowserApi api({http.url("/")}, 1500);
    QList<RadioBrowserApi::Result> results;
    api.request(QUrl("stations"), this, [&](auto, const auto &result) { results.append(result); });
    QTRY_VERIFY(http.client("/stations"));
    const QByteArray payload = R"json([{
        "name": "  - Test Radio  ",
        "tags": " jazz,rock",
        "country": " Poland ",
        "homepage": "https://station.example/",
        "url": "http://stream.example/playlist",
        "url_resolved": "https://stream.example/live",
        "stationuuid": "87654321-4321-4321-4321-123456789abc",
        "favicon": "https://station.example/icon.png",
        "codec": "MP3",
        "language": "polish",
        "state": "Mazovia",
        "countrycode": "PL",
        "bitrate": 128,
        "votes": 7,
        "clickcount": 13
    }])json";
    QVERIFY(http.respondJson("/stations", payload));
    QTRY_COMPARE(results.size(), 1);
    QCOMPARE(results.first().status, RadioBrowserApi::Status::Success);
    const auto stations = JsonListProcessor::stationsFromJson(results.first().data);
    QCOMPARE(stations.size(), 1);

    const auto &station = stations.first();
    QCOMPARE(station.station, QStringLiteral("Test Radio"));
    QCOMPARE(station.genre, QStringLiteral("jazz"));
    QCOMPARE(station.country, QStringLiteral("Poland"));
    QCOMPARE(station.homepage, QStringLiteral("https://station.example/"));
    QCOMPARE(station.streamUrl, QStringLiteral("https://stream.example/live"));
    QCOMPARE(station.stationUuid, QUuid(QStringLiteral("87654321-4321-4321-4321-123456789abc")));
    QCOMPARE(station.iconUrl, QStringLiteral("https://station.example/icon.png"));
    QCOMPARE(station.codec, QStringLiteral("MP3"));
    QCOMPARE(station.language, QStringLiteral("polish"));
    QCOMPARE(station.state, QStringLiteral("Mazovia"));
    QCOMPARE(station.countryCode, QStringLiteral("PL"));
    QCOMPARE(station.bitrate, 128);
    QCOMPARE(station.votes, 7);
    QCOMPARE(station.clickCount, 13);
}

void TuneScapeTests::testCatalogResponseErrors_data()
{
    QTest::addColumn<QByteArray>("body");
    QTest::addColumn<QByteArray>("status");
    QTest::addColumn<int>("expected");
    using Status = RadioBrowserApi::Status;
    QTest::newRow("empty-body") << QByteArray() << QByteArray("200 OK") << int(Status::InvalidResponse);
    QTest::newRow("invalid-json") << QByteArray("[invalid]") << QByteArray("200 OK") << int(Status::InvalidResponse);
    QTest::newRow("wrong-shape") << QByteArray("[1]") << QByteArray("200 OK") << int(Status::InvalidResponse);
    QTest::newRow("http-404") << QByteArray("[]") << QByteArray("404 Not Found") << int(Status::NetworkError);
    QTest::newRow("http-503") << QByteArray("[]") << QByteArray("503 Service Unavailable") << int(Status::NetworkError);
}

void TuneScapeTests::testCatalogResponseErrors()
{
    QFETCH(QByteArray, body);
    QFETCH(QByteArray, status);
    QFETCH(int, expected);
    TestHttpServer http;
    QVERIFY(http.listen());
    RadioBrowserApi api({http.url("/")}, 1500);
    QList<RadioBrowserApi::Result> results;
    api.request(QUrl("errors"), this, [&](auto, const auto &result) { results.append(result); });
    QTRY_VERIFY(http.client("/errors"));
    QVERIFY(http.respondJson("/errors", body, status));
    QTRY_COMPARE(results.size(), 1);
    QCOMPARE(int(results.first().status), expected);
    QVERIFY(results.first().data.isEmpty());
    QVERIFY(!results.first().detail.isEmpty());
}

void TuneScapeTests::testRadioStationsGenerateFullAddress()
{
    const auto endpoint = RadioBrowserApi::endpoint("json/stations/search", "name", QStringLiteral("Łódź + & /"));
    const auto first = RadioBrowserApi::baseUrlForTarget("first.example.").resolved(endpoint);
    const auto second = RadioBrowserApi::baseUrlForTarget("second.example", 8443).resolved(endpoint);
    QCOMPARE(first.toEncoded(), QByteArray("https://first.example/json/stations/search?name=%C5%81%C3%B3d%C5%BA%20%2B%20%26%20%2F"));
    QCOMPARE(second.port(), 8443);
    QCOMPARE(second.host(), QString("second.example"));
}

void TuneScapeTests::testCatalogTimeout()
{
    TestHttpServer http;
    QVERIFY(http.listen());
    RadioBrowserApi api({http.url("/")}, 200);
    QList<RadioBrowserApi::Result> results;
    bool timerRan = false;
    api.request(QUrl("stall"), this, [&](auto, const auto &result) { results.append(result); });
    QTimer::singleShot(10, &api, [&] { timerRan = true; });
    QTRY_VERIFY(http.client("/stall"));
    QTRY_COMPARE(results.size(), 1);
    QVERIFY(timerRan);
    QCOMPARE(results.first().status, RadioBrowserApi::Status::Timeout);
    QTRY_COMPARE(http.client("/stall")->state(), QAbstractSocket::UnconnectedState);
}

void TuneScapeTests::testCatalogMirrorFallback()
{
    TestHttpServer broken;
    TestHttpServer working;
    QVERIFY(broken.listen());
    QVERIFY(working.listen());
    QTcpServer unused;
    QVERIFY(unused.listen(QHostAddress::LocalHost));
    const QUrl refused(QStringLiteral("http://127.0.0.1:%1/").arg(unused.serverPort()));
    unused.close();

    RadioBrowserApi api({refused, broken.url("/"), working.url("/")}, 1500);
    QList<RadioBrowserApi::Result> results;
    api.request(QUrl("stations"), this, [&](auto, const auto &result) { results.append(result); });
    QTRY_VERIFY(broken.client("/stations"));
    QVERIFY(broken.respondJson("/stations", "malformed"));
    QTRY_VERIFY(working.client("/stations"));
    QVERIFY(working.respondJson("/stations", "[{\"name\":\"Available\"}]"));
    QTRY_COMPARE(results.size(), 1);
    QCOMPARE(results.first().status, RadioBrowserApi::Status::Success);
    QCOMPARE(JsonListProcessor::stationsFromJson(results.first().data).first().station, QString("Available"));
}

void TuneScapeTests::testCatalogCancellation()
{
    TestHttpServer http;
    QVERIFY(http.listen());
    RadioBrowserApi api({http.url("/")}, 1500);
    QList<RadioBrowserApi::Result> results;
    const auto pending = api.request(QUrl("pending"), this, [&](auto, const auto &result) { results.append(result); });
    const auto active = api.request(QUrl("active"), this, [&](auto, const auto &result) { results.append(result); });
    api.cancel(pending);
    QTRY_VERIFY(http.client("/active"));
    api.cancel(active);
    api.cancel(active);
    QCOMPARE(results.size(), 2);
    QCOMPARE(results.at(0).status, RadioBrowserApi::Status::Cancelled);
    QCOMPARE(results.at(1).status, RadioBrowserApi::Status::Cancelled);
    QVERIFY(!http.client("/pending"));
    QTRY_COMPARE(http.client("/active")->state(), QAbstractSocket::UnconnectedState);
    QCOMPARE(results.size(), 2);
}

void TuneScapeTests::testCatalogCancellationDuringFailover()
{
    TestHttpServer first;
    TestHttpServer second;
    TestHttpServer third;
    QVERIFY(first.listen());
    QVERIFY(second.listen());
    QVERIFY(third.listen());
    RadioBrowserApi api({first.url("/"), second.url("/"), third.url("/")}, 1500);
    QList<RadioBrowserApi::Result> results;
    const auto id = api.request(QUrl("failover"), this, [&](auto, const auto &result) { results.append(result); });
    QTRY_VERIFY(first.client("/failover"));
    QVERIFY(first.respondJson("/failover", "[]", "503 Service Unavailable"));
    QTRY_VERIFY(second.client("/failover"));
    api.cancel(id);
    QTRY_COMPARE(second.client("/failover")->state(), QAbstractSocket::UnconnectedState);
    QCOMPARE(results.size(), 1);
    QCOMPARE(results.first().status, RadioBrowserApi::Status::Cancelled);
    QVERIFY(!third.client("/failover"));
}

void TuneScapeTests::testCatalogOutOfOrder()
{
    TestHttpServer http;
    QVERIFY(http.listen());
    RadioBrowserApi api({http.url("/")}, 1500);
    QList<RadioBrowserApi::RequestId> order;
    QStringList names;
    const auto completion = [&](auto id, const auto &result) {
        order.append(id);
        const auto stations = JsonListProcessor::stationsFromJson(result.data);
        names.append(stations.isEmpty() ? QString() : stations.first().station);
    };
    const auto first = api.request(QUrl("first"), this, completion);
    const auto second = api.request(QUrl("second"), this, completion);
    QTRY_VERIFY(http.client("/first"));
    QTRY_VERIFY(http.client("/second"));
    QVERIFY(http.respondJson("/second", "[{\"name\":\"Second\"}]"));
    QTRY_COMPARE(order.size(), 1);
    QCOMPARE(order.first(), second);
    QVERIFY(http.respondJson("/first", "[{\"name\":\"First\"}]"));
    QTRY_COMPARE(order.size(), 2);
    QCOMPARE(order.last(), first);
    QCOMPARE(names, (QStringList{"Second", "First"}));
}

void TuneScapeTests::testCatalogReceiverDestruction()
{
    TestHttpServer http;
    QVERIFY(http.listen());
    RadioBrowserApi api({http.url("/")}, 1500);
    auto receiver = std::make_unique<QObject>();
    int calls = 0;
    api.request(QUrl("close"), receiver.get(), [&](auto, const auto &) { ++calls; });
    QTRY_VERIFY(http.client("/close"));
    receiver.reset();
    QTRY_COMPARE(http.client("/close")->state(), QAbstractSocket::UnconnectedState);
    QCOMPARE(calls, 0);
}

void TuneScapeTests::testCatalogListsDoNotOverwriteVisibleSelection()
{
    TestHttpServer http;
    QVERIFY(http.listen());
    RadioBrowserApi api({http.url("/")}, 2000);
    QMainWindow window;
    Ui::MainWindow ui;
    ui.setupUi(&window);
    RadioExplorer explorer(&ui);
    explorer.createMenu();
    FavoriteManager favorites;
    RadioList radioList(&ui, &favorites, &api);
    radioList.setMainWindow(&window);
    QSignalSpy lists(&radioList, &RadioList::catalogPlaylistLoaded);
    radioList.loadAllData();
    window.show();

    const QByteArray top = "/json/stations/topvote/20";
    const QByteArray popular = "/json/stations/topclick/20";
    const QByteArray newest = "/json/stations/lastchange/20";
    QTRY_VERIFY(http.client(top) && http.client(popular) && http.client(newest));
    QVERIFY(window.isVisible());
    QCOMPARE(radioList.radioStationsModel->size(), 0);
    auto *tree = ui.treeView->model();
    const auto library = tree->index(0, 0);
    const auto popularIndex = tree->index(1, 0, library);
    QVERIFY(popularIndex.isValid());
    emit ui.treeView->clicked(popularIndex);

    QVERIFY(http.respondJson(popular, "[{\"name\":\"Popular station\"}]"));
    QTRY_COMPARE(radioList.radioStationsModel->size(), 1);
    QCOMPARE(radioList.radioStationsModel->station(0).station, QString("Popular station"));
    QVERIFY(http.respondJson(top, "[{\"name\":\"Top station\"}]"));
    QVERIFY(http.respondJson(newest, "[{\"name\":\"New station\"}]"));
    QTRY_COMPARE(lists.size(), 3);
    QCOMPARE(radioList.radioStationsModel->station(0).station, QString("Popular station"));
    QCOMPARE(radioList.stations(Stations::NEW).first().station, QString("New station"));
    emit ui.treeView->clicked(tree->index(0, 0, library));
    QCOMPARE(radioList.radioStationsModel->station(0).station, QString("Top station"));
}

void TuneScapeTests::testCatalogCountrySearchAndTabChange()
{
    TestHttpServer http;
    QVERIFY(http.listen());
    RadioBrowserApi api({http.url("/")}, 2000);
    QMainWindow window;
    Ui::MainWindow ui;
    ui.setupUi(&window);
    FavoriteManager favorites;
    ControlledAudio audio;
    PlaybackController playback(audio);
    RadioList radioList(&ui, &favorites, &api, &playback);
    Country country;
    country.setData(&ui, &radioList);
    ui.tabRadioListWidget->setCurrentWidget(ui.listCountries);
    QTRY_VERIFY(http.client("/json/countries"));
    QVERIFY(http.respondJson("/json/countries",
        "[{\"name\":\"Poland\",\"iso_3166_1\":\"PL\"},{\"name\":\"United Kingdom\",\"iso_3166_1\":\"GB\"}]"));
    QTRY_COMPARE(ui.comboBox->count(), 2);
    const auto selectCountry = [&ui](int index) {
        ui.comboBox->setCurrentIndex(index);
        emit ui.comboBox->textActivated(ui.comboBox->currentText());
    };

    selectCountry(0);
    const QByteArray first = "/json/stations/search?countrycode=PL";
    QTRY_VERIFY(http.client(first));
    selectCountry(1);
    const QByteArray second = "/json/stations/search?countrycode=GB";
    QTRY_VERIFY(http.client(second));
    QTRY_COMPARE(http.client(first)->state(), QAbstractSocket::UnconnectedState);
    QVERIFY(http.respondJson(second, "[{\"name\":\"Current country\"}]"));
    auto *model = qobject_cast<RadioStationsModel *>(ui.tableOfCoutries->model());
    QVERIFY(model);
    QTRY_COMPARE(model->size(), 1);
    QCOMPARE(model->station(0).station, QString("Current country"));

    const auto station = playbackStation("Playing from country", http.url("/country-live"));
    model->setStations({station});
    emit ui.tableOfCoutries->doubleClicked(model->index(0, 0));
    audio.confirmPlaying();
    QCOMPARE(playback.state(), PlaybackController::State::Playing);
    selectCountry(0);
    auto *old = http.client(first);
    QTRY_VERIFY(http.client(first) != old);
    ui.tabRadioListWidget->setCurrentIndex(0);
    QTRY_COMPARE(http.client(first)->state(), QAbstractSocket::UnconnectedState);
    QCOMPARE(model->size(), 0);
    QCOMPARE(playback.state(), PlaybackController::State::Playing);
    QCOMPARE(playback.streamUrl(), QUrl(station.streamUrl));
    QCOMPARE(audio.attempts.size(), 1);
}

void TuneScapeTests::testCatalogSearchReplacementAndCancellation()
{
    TestHttpServer http;
    QVERIFY(http.listen());
    RadioBrowserApi api({http.url("/")}, 4000);
    QMainWindow window;
    Ui::MainWindow ui;
    ui.setupUi(&window);
    FavoriteManager favorites;
    RadioList radioList(&ui, &favorites, &api);
    window.show();

    const auto search = [&ui](const QString &name) {
        ui.serachInput->setText(name);
        emit ui.serachInput->returnPressed();
    };
    auto *progress = window.findChild<QProgressDialog *>();
    QVERIFY(progress);
    auto *cancel = progress->findChild<QPushButton *>();
    QVERIFY(cancel);

    search("old");
    const QByteArray old = "/json/stations/search?name=old";
    QTRY_VERIFY(http.client(old));
    QVERIFY(http.respond(http.client(old), "[", 15, "200 OK", "application/json"));
    search("fresh + one");
    const QByteArray fresh = "/json/stations/search?name=fresh%20%2B%20one";
    QTRY_VERIFY(http.client(fresh));
    QTRY_COMPARE(http.client(old)->state(), QAbstractSocket::UnconnectedState);
    QVERIFY(http.respondJson(fresh, "[{\"name\":\"Fresh result\"}]"));
    QTRY_COMPARE(radioList.radioStationsModel->size(), 1);
    QCOMPARE(radioList.radioStationsModel->station(0).station, QString("Fresh result"));

    for (int attempt = 0; attempt < 3; ++attempt) {
        const QString name = QStringLiteral("cancel-%1").arg(attempt);
        const QByteArray target = "/json/stations/search?name=" + name.toLatin1();
        search(name);
        QTRY_VERIFY(http.client(target));
        QTRY_VERIFY(progress->isVisible());
        cancel->click();
        QTRY_COMPARE(http.client(target)->state(), QAbstractSocket::UnconnectedState);
        QVERIFY(!progress->isVisible());
        QCOMPARE(radioList.radioStationsModel->size(), 0);
    }
}

void TuneScapeTests::testCatalogStationDetailsIgnorePreviousRequest()
{
    TestHttpServer http;
    QVERIFY(http.listen());
    RadioBrowserApi api({http.url("/")}, 2000);
    QMainWindow window;
    Ui::MainWindow ui;
    ui.setupUi(&window);
    FavoriteManager favorites;
    ControlledAudio audio;
    PlaybackController playback(audio);
    RadioList radioList(&ui, &favorites, &api, &playback);

    auto previous = playbackStation("Previous", http.url("/previous-live"));
    previous.iconUrl = http.url("/old-icon").toString();
    playback.play(previous);
    const QByteArray first = "/json/stations/search?name=Previous&nameExact=true";
    QTRY_VERIFY(http.client(first) && http.client("/old-icon"));

    auto current = playbackStation("Cached", http.url("/current-live"));
    current.stationUuid = QUuid("87654321-4321-4321-4321-123456789abc");
    playback.play(current);
    const QByteArray second = "/json/stations/byuuid/87654321-4321-4321-4321-123456789abc";
    QCOMPARE(ui.tableWidget->item(0, 1)->text(), current.station);
    QTRY_VERIFY(http.client(second));
    QTRY_COMPARE(http.client(first)->state(), QAbstractSocket::UnconnectedState);
    QTRY_COMPARE(http.client("/old-icon")->state(), QAbstractSocket::UnconnectedState);
    QVERIFY(http.respondJson(second, "[{\"name\":\"Current\",\"codec\":\"AAC\",\"clicktrend\":11}]"));
    QTRY_COMPARE(ui.tableWidget->item(0, 1)->text(), QString("Current"));
    QCOMPARE(ui.tableWidget->item(5, 1)->text(), QString("AAC"));
    QCOMPARE(ui.tableWidget->item(8, 1)->text(), QString("11"));

    auto legacy = playbackStation("Shared", http.url("/selected"));
    playback.play(legacy);
    const QByteArray third = "/json/stations/search?name=Shared&nameExact=true";
    QTRY_VERIFY(http.client(third));
    const QByteArray matches = QByteArray(R"([
        {"name":"Shared","url_resolved":"https://stream.example/other","codec":"FLAC"},
        {"name":"Shared","url_resolved":"%selected%","codec":"MP3"}
    ])").replace("%selected%", legacy.streamUrl.toUtf8());
    QVERIFY(http.respondJson(third, matches));
    QTRY_COMPARE(ui.tableWidget->item(5, 1)->text(), QString("MP3"));
}

void TuneScapeTests::testCatalogFailureLeavesPlaybackAndRecordingRunning()
{
    const QTemporaryDir directory;
    QVERIFY(directory.isValid());
    TestHttpServer http;
    QVERIFY(http.listen());
    RadioBrowserApi api({http.url("/")}, 1500);
    QMainWindow window;
    Ui::MainWindow ui;
    ui.setupUi(&window);
    FavoriteManager favorites;
    ControlledAudio audio;
    PlaybackController playback(audio);
    RadioList radioList(&ui, &favorites, &api, &playback);
    playback.play(playbackStation("Station", http.url("/metadata")));
    audio.confirmPlaying();
    auto &recorder = radioList.getStreamRecorder();
    recorder.startRecording(http.url("/live"), "Station", "MP3", directory.path());
    QTRY_VERIFY(http.client("/live"));
    QVERIFY(http.respond(http.client("/live"), "first"));
    QTRY_COMPARE(recorder.bytesWritten(), 5);

    ui.serachInput->setText("broken");
    emit ui.serachInput->returnPressed();
    const QByteArray target = "/json/stations/search?name=broken";
    QTRY_VERIFY(http.client(target));
    QVERIFY(http.respondJson(target, "", "503 Service Unavailable"));
    QTRY_VERIFY(ui.statusbar->currentMessage().contains("503"));
    QCOMPARE(playback.state(), PlaybackController::State::Playing);
    QCOMPARE(recorder.state(), StreamRecorder::State::Recording);
    auto *failedRequest = http.client(target);
    emit ui.serachInput->returnPressed();
    QTRY_VERIFY(http.client(target) != failedRequest);
    QVERIFY(http.respondJson(target, "[{\"name\":\"Recovered\"}]"));
    QTRY_COMPARE(radioList.radioStationsModel->size(), 1);
    QCOMPARE(radioList.radioStationsModel->station(0).station, QString("Recovered"));
    QCOMPARE(http.client("/live")->write("more"), 4);
    QTRY_COMPARE(recorder.bytesWritten(), 9);
    ui.playPause->click();
    QCOMPARE(playback.state(), PlaybackController::State::Paused);
    ui.stop->click();
    QCOMPARE(recorder.state(), StreamRecorder::State::Idle);
    QCOMPARE(fileContents(recorder.filePath()), QByteArray("firstmore"));
}

void TuneScapeTests::testImagesShareDownloadsAndCacheAcrossViews()
{
    TestHttpServer http;
    QVERIFY(http.listen());
    IconLoader loader;
    QObject table;
    QObject tray;
    QObject details;
    const auto url = http.url("/shared");
    QPixmap inTable;
    QPixmap inTray;
    QPixmap inDetails;
    loader.request(url, {20, 20}, 1, &table, [&](auto, const auto &pixmap) { inTable = pixmap; });
    loader.request(url, {32, 32}, 1, &tray, [&](auto, const auto &pixmap) { inTray = pixmap; });
    loader.request(url, {120, 120}, 2, &details, [&](auto, const auto &pixmap) { inDetails = pixmap; });
    QTRY_VERIFY(http.client("/shared"));
    QVERIFY(http.respondImage("/shared", imageFixture({48, 24}, Qt::blue)));
    QTRY_VERIFY(!inTable.isNull() && !inTray.isNull() && !inDetails.isNull());
    QCOMPARE(inTable.size(), QSize(20, 10));
    QCOMPARE(inTray.size(), QSize(32, 16));
    QCOMPARE(inDetails.size(), QSize(240, 120));
    QCOMPARE(inDetails.devicePixelRatio(), 2.0);

    for (int index = 0; index < 4; ++index) {
        loader.request(http.url(QStringLiteral("/slow-%1").arg(index)), {20, 20}, 1, &tray,
                       [](auto, const auto &) {}, IconLoader::Priority::Background);
    }
    QTRY_VERIFY(http.client("/slow-3"));
    inDetails = {};
    loader.request(url, {120, 120}, 2, &details, [&](auto, const auto &pixmap) { inDetails = pixmap; });
    QTRY_VERIFY_WITH_TIMEOUT(!inDetails.isNull(), 1000);
    QCOMPARE(http.requestCount("/shared"), 1);
    QCOMPARE(http.client("/slow-0")->state(), QAbstractSocket::ConnectedState);
}

void TuneScapeTests::testImageFailures_data()
{
    QTest::addColumn<QString>("target");
    QTest::addColumn<QByteArray>("body");
    QTest::addColumn<QByteArray>("status");
    QTest::newRow("empty-url") << QString() << QByteArray() << QByteArray();
    QTest::newRow("invalid-url") << QString("http://[bad") << QByteArray() << QByteArray();
    QTest::newRow("http-404") << QString("/not-found") << QByteArray() << QByteArray("404 Not Found");
    QTest::newRow("not-an-image") << QString("/bad-image") << QByteArray("bad") << QByteArray("200 OK");
}

void TuneScapeTests::testImageFailures()
{
    QFETCH(QString, target);
    QFETCH(QByteArray, body);
    QFETCH(QByteArray, status);
    TestHttpServer http;
    QVERIFY(http.listen());
    IconLoader loader;
    const QUrl url = status.isEmpty() ? QUrl(target) : http.url(target);
    QList<QPixmap> results;
    const auto complete = [&](auto, const auto &pixmap) { results.append(pixmap); };
    loader.request(url, {20, 20}, 1, this, complete);
    if (!status.isEmpty()) {
        QTRY_VERIFY(http.client(target.toUtf8()));
        QVERIFY(http.respondImage(target.toUtf8(), body, status));
    }
    QTRY_COMPARE(results.size(), 1);
    QVERIFY(results.first().isNull());
    loader.request(url, {32, 32}, 1, this, complete);
    QTRY_COMPARE(results.size(), 2);
    QVERIFY(results.last().isNull());
    QCOMPARE(http.requestCount(), status.isEmpty() ? 0 : 1);
}

void TuneScapeTests::testImageTimeoutAndConcurrencyLimit()
{
    TestHttpServer http;
    QVERIFY(http.listen());
    IconLoader loader(nullptr, 4096, 700);
    int failed = 0;
    const auto completed = [&](auto, const auto &pixmap) { failed += pixmap.isNull(); };
    for (int index = 0; index < 5; ++index) {
        loader.request(http.url(QStringLiteral("/%1").arg(index)), {20, 20}, 1, this,
                       completed, IconLoader::Priority::Background);
    }
    loader.request(http.url("/visible"), {20, 20}, 1, this, completed);
    QTRY_COMPARE(http.requestCount(), 4);
    QVERIFY(http.client("/visible"));
    QVERIFY(!http.client("/3"));
    QVERIFY(!http.client("/4"));
    QTRY_COMPARE(http.requestCount(), 6);
    QTRY_COMPARE(failed, 6);
    for (int index = 0; index < 5; ++index)
        QTRY_COMPARE(http.client("/" + QByteArray::number(index))->state(), QAbstractSocket::UnconnectedState);
}

void TuneScapeTests::testImageCacheEviction()
{
    TestHttpServer http;
    QVERIFY(http.listen());
    IconLoader loader(nullptr, 2400);
    const auto png = imageFixture({24, 24}, Qt::red);
    QVERIFY(!png.isEmpty());
    QPixmap result;
    const auto load = [&](const QString &path) {
        result = {};
        loader.request(http.url(path), {20, 20}, 1, this,
                       [&](auto, const auto &pixmap) { result = pixmap; });
    };
    load("/first");
    QTRY_VERIFY(http.client("/first"));
    QVERIFY(http.respondImage("/first", png));
    QTRY_VERIFY(!result.isNull());
    load("/second");
    QTRY_VERIFY(http.client("/second"));
    QVERIFY(http.respondImage("/second", png));
    QTRY_VERIFY(!result.isNull());
    load("/second");
    QTRY_VERIFY(!result.isNull());
    QCOMPARE(http.requestCount("/second"), 1);
    load("/first");
    QTRY_COMPARE(http.requestCount("/first"), 2);
    QVERIFY(http.respondImage("/first", png));
    QTRY_VERIFY(!result.isNull());
}

void TuneScapeTests::testLargeImageAndHighDpi()
{
    TestHttpServer http;
    QVERIFY(http.listen());
    IconLoader loader(nullptr, 700000);
    QPixmap result;
    const auto complete = [&](auto, const auto &pixmap) { result = pixmap; };
    const auto url = http.url("/large");
    loader.request(url, {120, 120}, 2.5, this, complete);
    QTRY_VERIFY(http.client("/large"));
    const auto png = imageFixture({3200, 1600}, Qt::green);
    QVERIFY(!png.isEmpty());
    QVERIFY(http.respondImage("/large", png));
    QTRY_VERIFY(!result.isNull());
    QCOMPARE(result.size(), QSize(300, 150));
    QCOMPARE(result.deviceIndependentSize(), QSizeF(120, 60));
    QCOMPARE(result.toImage().pixelColor(15, 15), QColor(Qt::green));

    result = {};
    loader.request(url, {20, 20}, 1.5, this, complete);
    QTRY_VERIFY(!result.isNull());
    QCOMPARE(result.size(), QSize(30, 15));
    QCOMPARE(result.devicePixelRatio(), 1.5);
    QCOMPARE(http.requestCount("/large"), 1);
}

void TuneScapeTests::testTableImagesFollowStationIdentity()
{
    TestHttpServer http;
    QVERIFY(http.listen());
    IconLoader loader;
    RadioStationsModel model;
    QTableView view;
    view.setIconSize({20, 20});
    view.resize(380, 210);
    view.setModel(&model);
    model.bindIcons(&view, &loader);
    view.show();
    const auto station = [&](const QString &name, const QString &path) {
        RadioStation item;
        item.station = name;
        item.stationUuid = QUuid::createUuid();
        item.streamUrl = http.url("/stream-" + name).toString();
        item.iconUrl = http.url(path).toString();
        return item;
    };
    model.setStations({station("Old", "/old")});
    QTRY_VERIFY(http.client("/old"));
    model.setStations({station("First", "/first"), station("Second", "/second")});
    QTRY_COMPARE(http.client("/old")->state(), QAbstractSocket::UnconnectedState);
    QTRY_VERIFY(http.client("/first") && http.client("/second"));
    QSignalSpy updates(&model, &RadioStationsModel::dataChanged);
    QVERIFY(http.respondImage("/second", imageFixture({24, 24}, Qt::blue)));
    QTRY_COMPARE(modelIconColor(model, 1), QColor(Qt::blue));
    QVERIFY(http.respondImage("/first", imageFixture({24, 24}, Qt::red)));
    QTRY_COMPARE(modelIconColor(model, 0), QColor(Qt::red));
    QCOMPARE(modelIconColor(model, 1), QColor(Qt::blue));
    QCOMPARE(updates.size(), 2);
}

void TuneScapeTests::testTableImagesLoadVisibleRowsAndCancelOnClose()
{
    TestHttpServer http;
    QVERIFY(http.listen());
    IconLoader loader;
    RadioStationsModel model;
    auto view = std::make_unique<QTableView>();
    view->setIconSize({20, 20});
    view->resize(320, 150);
    view->setModel(&model);
    model.bindIcons(view.get(), &loader);
    QVector<RadioStation> stations(80);
    for (int row = 0; row < stations.size(); ++row)
        stations[row].iconUrl = http.url(QStringLiteral("/row-%1").arg(row)).toString();
    model.setStations(stations);
    view->show();
    QTRY_VERIFY(http.client("/row-0"));
    const int last = view->rowAt(view->viewport()->height() - 1);
    QVERIFY(last >= 0 && last < 79);
    QSignalSpy initialUpdates(&model, &RadioStationsModel::dataChanged);
    const auto png = imageFixture({24, 24}, Qt::blue);
    for (int row = 0; row <= last; ++row) {
        const QByteArray target = "/row-" + QByteArray::number(row);
        QTRY_VERIFY(http.client(target));
        QVERIFY(http.respondImage(target, png));
    }
    QTRY_COMPARE(initialUpdates.size(), last + 1);
    QCOMPARE(http.requestCount(), last + 1);
    qInfo() << "Initial icon GETs for 80 rows:" << http.requestCount();

    view->scrollTo(model.index(79, 0), QAbstractItemView::PositionAtBottom);
    QTRY_VERIFY(http.client("/row-79"));
    QSignalSpy updates(&model, &RadioStationsModel::dataChanged);
    view.reset();
    QTRY_COMPARE(http.client("/row-79")->state(), QAbstractSocket::UnconnectedState);
    QCOMPARE(updates.size(), 0);
}

void TuneScapeTests::testAppConfigCheckFileIsExists()
{
    const QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("application.properties"));
    QVERIFY(!QFile::exists(path));
    AppConfig appConfig(path);
    QVERIFY(appConfig.checkFile());
    QVERIFY(QFile::exists(path));
    QVERIFY(appConfig.checkBoolState(QStringLiteral("notifications")));
    QVERIFY(!appConfig.checkBoolState(QStringLiteral("darktheme")));
}

void TuneScapeTests::testAppConfigCheckBoolState()
{
    const QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("application.properties"));
    QFile fixture(path);
    const QByteArray contents = "notifications=false\ndarktheme=true\n";
    QVERIFY(fixture.open(QIODevice::WriteOnly | QIODevice::Text));
    QCOMPARE(fixture.write(contents), contents.size());
    fixture.close();

    AppConfig appConfig(path);
    QVERIFY(appConfig.checkFile());
    QVERIFY(!appConfig.checkBoolState(QStringLiteral("notifications")));
    QVERIFY(appConfig.checkBoolState(QStringLiteral("darktheme")));
}

void TuneScapeTests::testAppConfigChangeBoolState()
{
    const QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("application.properties"));
    AppConfig appConfig(path);
    const QString notifications = QStringLiteral("notifications");
    QVERIFY(appConfig.checkBoolState(notifications));
    QVERIFY(appConfig.changeBoolState(false, notifications));
    AppConfig reloaded(path);
    QVERIFY(!reloaded.checkBoolState(notifications));
    QVERIFY(appConfig.changeBoolState(true, notifications));
    QVERIFY(reloaded.checkBoolState(notifications));
    QVERIFY(!appConfig.changeBoolState(true, QStringLiteral("unknown")));
    QVERIFY(!reloaded.checkBoolState(QStringLiteral("darktheme")));
}

void TuneScapeTests::testMiniPlayerIsVisible()
{
    miniplayer miniPlayer;
    QVERIFY(!miniPlayer.isVisible());
    miniPlayer.show();
    QVERIFY(miniPlayer.isVisible());
    miniPlayer.hide();
    QVERIFY(!miniPlayer.isVisible());
}

void TuneScapeTests::testFavoriteAddAndRemove()
{
    const QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath("radiobrowser.txt");
    const FavoriteManager favorites(path);
    const auto first = favoriteStation();
    const auto second = favoriteStation("https://radio.example/Second");
    QVector<RadioStation> saved;

    QVERIFY(favorites.load(saved).isEmpty());
    QVERIFY(saved.isEmpty());
    QVERIFY(!QFile::exists(path));
    const QString error = favorites.setFavorite(first, true, saved);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QCOMPARE(saved.size(), 1);
    QVERIFY(favorites.setFavorite(second, true, saved).isEmpty());
    QVERIFY(favorites.setFavorite(first, false, saved).isEmpty());
    QCOMPARE(saved.size(), 1);
    QCOMPARE(saved.first().streamUrl, second.streamUrl);

    const FavoriteManager reloaded(path);
    QVERIFY(reloaded.load(saved).isEmpty());
    QCOMPARE(saved.size(), 1);
    QCOMPARE(saved.first().streamUrl, second.streamUrl);
    QVERIFY(favorites.setFavorite(second, false, saved).isEmpty());
    QVERIFY(saved.isEmpty());
    QVERIFY(reloaded.load(saved).isEmpty());
    QVERIFY(saved.isEmpty());
}

void TuneScapeTests::testFavoriteRoundTrip_data()
{
    QTest::addColumn<QString>("name");
    QTest::newRow("comma") << QStringLiteral("Radio, Jazz");
    QTest::newRow("unicode and raw quotes") << QStringLiteral("\"Łódź\", Rádio 日本 🎵");
}

void TuneScapeTests::testFavoriteRoundTrip()
{
    QFETCH(QString, name);
    const QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const FavoriteManager favorites(directory.filePath(QStringLiteral("ulubione Łódź.txt")));
    auto station = favoriteStation("https://radio.example/Live?tags=jazz,rock");
    station.station = name;
    station.country = QStringLiteral("Bonaire, Sint Eustatius and Saba");
    station.homepage = QStringLiteral("https://radio.example/?show=1,2");
    station.iconUrl = QStringLiteral("https://radio.example/logo.png?v=2,3");
    station.genre = QStringLiteral("\"Jazz\"");

    QVector<RadioStation> saved;
    QVERIFY(favorites.setFavorite(station, true, saved).isEmpty());
    saved.clear();
    QVERIFY(favorites.load(saved).isEmpty());
    QCOMPARE(saved.size(), 1);
    const auto &actual = saved.first();
    QCOMPARE(actual.station, name);
    QCOMPARE(actual.streamUrl, station.streamUrl);
    QCOMPARE(actual.country, station.country);
    QCOMPARE(actual.homepage, station.homepage);
    QCOMPARE(actual.iconUrl, station.iconUrl);
    QCOMPARE(actual.genre, station.genre);
}

void TuneScapeTests::testFavoriteDuplicateAndUrlIdentity()
{
    const QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath("radiobrowser.txt");
    const FavoriteManager favorites(path);
    const auto first = favoriteStation("https://radio.example/Live?Mode=One");
    const auto duplicate = favoriteStation("HTTPS://RADIO.EXAMPLE:443/music/../Live?Mode=One#title");
    const auto prefix = favoriteStation("https://radio.example/Live?Mode=OneMore");
    const auto pathCase = favoriteStation("https://radio.example/live?Mode=One");
    const auto queryCase = favoriteStation("https://radio.example/Live?Mode=one");
    QVector<RadioStation> saved;

    QVERIFY(favorites.setFavorite(first, true, saved).isEmpty());
    const QByteArray original = fileContents(path);
    QVERIFY(favorites.setFavorite(duplicate, true, saved).isEmpty());
    QCOMPARE(saved.size(), 1);
    QCOMPARE(fileContents(path), original);
    QVERIFY(favorites.setFavorite(prefix, true, saved).isEmpty());
    QVERIFY(favorites.setFavorite(pathCase, true, saved).isEmpty());
    QVERIFY(favorites.setFavorite(queryCase, true, saved).isEmpty());
    QCOMPARE(saved.size(), 4);
    QVERIFY(favorites.setFavorite(duplicate, false, saved).isEmpty());
    QCOMPARE(saved.size(), 3);
    QCOMPARE(saved[0].streamUrl, prefix.streamUrl);
    QCOMPARE(saved[1].streamUrl, pathCase.streamUrl);
    QCOMPARE(saved[2].streamUrl, queryCase.streamUrl);
}

void TuneScapeTests::testFavoriteUuidIdentity()
{
    const QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath("radiobrowser.txt");
    const FavoriteManager favorites(path);
    auto first = favoriteStation();
    first.stationUuid = QUuid::createUuid();
    auto second = first;
    second.stationUuid = QUuid::createUuid();
    auto moved = first;
    moved.streamUrl = QStringLiteral("https://new.example/Live");
    QVector<RadioStation> saved;

    QVERIFY(favorites.setFavorite(first, true, saved).isEmpty());
    QVERIFY(favorites.setFavorite(moved, true, saved).isEmpty());
    QCOMPARE(saved.size(), 1);
    QVERIFY(favorites.setFavorite(second, true, saved).isEmpty());
    QVERIFY(favorites.load(saved).isEmpty());
    QCOMPARE(saved.size(), 2);
    QCOMPARE(saved.first().stationUuid, first.stationUuid);
    QCOMPARE(saved.last().stationUuid, second.stationUuid);

    const QByteArray original = fileContents(path);
    QVERIFY(!favorites.setFavorite(favoriteStation(), false, saved).isEmpty());
    QCOMPARE(fileContents(path), original);
    QVERIFY(favorites.setFavorite(moved, false, saved).isEmpty());
    QCOMPARE(saved.size(), 1);
    QCOMPARE(saved.first().stationUuid, second.stationUuid);
}

void TuneScapeTests::testFavoriteM3UImportExport()
{
    const QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath("radiobrowser.txt");
    const FavoriteManager favorites(path);
    const QString source = directory.filePath(QStringLiteral("Źródło.M3U8"));
    const QString output = directory.filePath("export.m3u");
    const QString name = QStringLiteral("Radio, \"Łódź\"");
    const QString address = QStringLiteral("https://station.example/live?genre=jazz,rock");
    const QByteArray m3u = QByteArray::fromHex("efbbbf")
                           + (QStringLiteral("#EXTM3U\r\n#EXTINF:-1 tvg-name=\"Łódź, Jazz\",")
                              + name + "\r\n" + address
                              + "\r\n#EXTINF:-1,Other\r\nhttp://station.example/other\r\n").toUtf8();
    QVERIFY(writeFixture(source, m3u));

    QVector<RadioStation> saved;
    QVERIFY(favorites.setFavorite(favoriteStation(), true, saved).isEmpty());
    const QString importError = favorites.importPlaylist(source, saved);
    QVERIFY2(importError.isEmpty(), qPrintable(importError));
    QCOMPARE(saved.size(), 2);
    QCOMPARE(saved.first().station, name);
    QCOMPARE(saved.first().streamUrl, address);
    QCOMPARE(saved.last().station, QStringLiteral("Other"));
    QCOMPARE(fileContents(source), m3u);

    const QString exportError = favorites.exportPlaylist(output, FavoriteManager::Format::M3U);
    QVERIFY2(exportError.isEmpty(), qPrintable(exportError));
    QCOMPARE(fileContents(output),
             (QStringLiteral("#EXTM3U\n#EXTINF:-1,") + name + '\n' + address
              + "\n#EXTINF:-1,Other\nhttp://station.example/other\n").toUtf8());
    const FavoriteManager reimported(directory.filePath("reimported.txt"));
    QVector<RadioStation> roundTrip;
    QVERIFY(reimported.importPlaylist(output, roundTrip).isEmpty());
    QCOMPARE(roundTrip.size(), 2);
    QCOMPARE(roundTrip.first().station, name);
    QCOMPARE(roundTrip.first().streamUrl, address);
}

void TuneScapeTests::testFavoriteInvalidImport_data()
{
    QTest::addColumn<QString>("name");
    QTest::addColumn<QByteArray>("bytes");
    QTest::newRow("empty") << QStringLiteral("bad.m3u") << QByteArray();
    QTest::newRow("header only") << QStringLiteral("bad.m3u") << QByteArray("#EXTM3U\n");
    QTest::newRow("bad URL") << QStringLiteral("bad.m3u") << QByteArray(
        "#EXTM3U\n#EXTINF:-1,Unsafe\nfile:///radio.mp3\n");
    QTest::newRow("partly valid M3U") << QStringLiteral("bad.m3u") << QByteArray(
        "#EXTM3U\n#EXTINF:-1,Valid\nhttps://radio.example/live\n#EXTINF:-1,Missing URL\n");
    QTest::newRow("malformed EXTINF") << QStringLiteral("bad.m3u") << QByteArray(
        "#EXTM3U\n#EXTINF:-1,Valid\nhttps://radio.example/live\n#EXTINF missing separator\n");
    QTest::newRow("bad UTF-8") << QStringLiteral("bad.m3u") << QByteArray(
        "#EXTM3U\n#EXTINF:-1,\xff\nhttps://radio.example/live\n");
    QTest::newRow("partly valid TXT") << QStringLiteral("bad.txt") << QByteArray(
        ",https://radio.example/live,Valid,,,\n,https://radio.example/other,Radio, Jazz,,,\n");
}

void TuneScapeTests::testFavoriteInvalidImport()
{
    QFETCH(QString, name);
    QFETCH(QByteArray, bytes);
    const QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath("radiobrowser.txt");
    const FavoriteManager favorites(path);
    QVector<RadioStation> saved;
    const auto originalStation = favoriteStation();
    QVERIFY(favorites.setFavorite(originalStation, true, saved).isEmpty());
    const QByteArray original = fileContents(path);
    const QString source = directory.filePath(name);
    QVERIFY(writeFixture(source, bytes));

    QVERIFY(!favorites.importPlaylist(source, saved).isEmpty());
    QCOMPARE(fileContents(path), original);
    QVERIFY(QFile::exists(source));
    QCOMPARE(fileContents(source), bytes);
    QCOMPARE(saved.size(), 1);
    QCOMPARE(saved.first().streamUrl, originalStation.streamUrl);
}

void TuneScapeTests::testFavoriteImportIntoItself()
{
    const QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath("radiobrowser.txt");
    const FavoriteManager favorites(path);
    const QByteArray original = ",https://radio.example/live,Original,,,\n";
    QVERIFY(writeFixture(path, original));

    QVector<RadioStation> saved;
    QVERIFY(!favorites.importPlaylist(directory.filePath("./radiobrowser.txt"), saved).isEmpty());
    QVERIFY(!favorites.exportPlaylist(path, FavoriteManager::Format::M3U).isEmpty());
    QCOMPARE(fileContents(path), original);
    QVERIFY(saved.isEmpty());

    const QString alias = directory.filePath("alias.txt");
    std::error_code linkError;
    std::filesystem::create_hard_link(QFileInfo(path).filesystemAbsoluteFilePath(),
                                     QFileInfo(alias).filesystemAbsoluteFilePath(), linkError);
    if (!linkError) {
        QVERIFY(!favorites.importPlaylist(alias, saved).isEmpty());
        QCOMPARE(fileContents(alias), original);
        QCOMPARE(fileContents(path), original);
        QVERIFY(saved.isEmpty());
    }
}

void TuneScapeTests::testFavoriteWriteFailure()
{
    const QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath("radiobrowser.txt");
    const FavoriteManager favorites(path);
    const QByteArray original = ",https://radio.example/Live,Original,,,\n";
    QVERIFY(writeFixture(path, original));
    QVector<RadioStation> saved;
    QVERIFY(favorites.load(saved).isEmpty());
    QCOMPARE(saved.size(), 1);

    {
        QLockFile lock(path + ".lock");
        QVERIFY(lock.tryLock(0));
        QVERIFY(!favorites.setFavorite(favoriteStation(), false, saved).isEmpty());
        QCOMPARE(fileContents(path), original);
        QCOMPARE(saved.size(), 1);
    }

    const QString output = directory.filePath("export.m3u");
    QVERIFY(QDir().mkpath(output));
    QVERIFY(!favorites.exportPlaylist(output, FavoriteManager::Format::M3U).isEmpty());
    QVERIFY(QFileInfo(output).isDir());
    QCOMPARE(fileContents(path), original);

#ifdef Q_OS_WIN
    // An open Windows file handle prevents the atomic replacement at commit.
    QFile originalHandle(path);
    QVERIFY(originalHandle.open(QIODevice::ReadOnly));
    const QString error = favorites.setFavorite(favoriteStation("https://radio.example/Other"),
                                                true, saved);
    QVERIFY2(error.startsWith(QStringLiteral("Cannot replace")), qPrintable(error));
    QCOMPARE(originalHandle.readAll(), original);
    originalHandle.close();
    QCOMPARE(fileContents(path), original);
    QCOMPARE(saved.size(), 1);
#endif
}

void TuneScapeTests::testFavoriteLegacyReadAndMigration()
{
    const QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath("radiobrowser.txt");
    const FavoriteManager favorites(path);
    const auto fixture = QStringLiteral("https://radio.example/r.png,https://radio.example/Live,"
                                        "\"Łódź\" Radio,Poland,Jazz,https://radio.example/\r\n"
                                        ",http://radio.example/other,From old M3U,,,\r\n").toUtf8();
    QVERIFY(writeFixture(path, fixture));
    QVector<RadioStation> loaded;
    QVERIFY(favorites.load(loaded).isEmpty());
    QCOMPARE(loaded.size(), 2);
    QCOMPARE(loaded.first().station, QStringLiteral("\"Łódź\" Radio"));
    QCOMPARE(loaded.first().country, QStringLiteral("Poland"));
    QCOMPARE(loaded.first().streamUrl, QStringLiteral("https://radio.example/Live"));
    QVERIFY(loaded.first().stationUuid.isNull());
    QCOMPARE(loaded.last().station, QStringLiteral("From old M3U"));
    QCOMPARE(loaded.last().streamUrl, QStringLiteral("http://radio.example/other"));
    QCOMPARE(fileContents(path), fixture);

    const auto newStation = favoriteStation("https://radio.example/New");
    QVERIFY(favorites.setFavorite(newStation, true, loaded).isEmpty());
    const FavoriteManager reloaded(path);
    QVERIFY(reloaded.load(loaded).isEmpty());
    QCOMPARE(loaded.size(), 3);
    QCOMPARE(loaded.first().station, QStringLiteral("\"Łódź\" Radio"));
    QCOMPARE(loaded.last().streamUrl, newStation.streamUrl);

    const QString exported = directory.filePath("backup.txt");
    QVERIFY(favorites.exportPlaylist(exported, FavoriteManager::Format::Native).isEmpty());
    const FavoriteManager restored(directory.filePath("restored.txt"));
    QVERIFY(restored.importPlaylist(exported, loaded).isEmpty());
    QCOMPARE(loaded.size(), 3);
    QCOMPARE(loaded.first().station, QStringLiteral("\"Łódź\" Radio"));

    const QString ambiguousPath = directory.filePath("ambiguous.txt");
    const QByteArray ambiguous = ",https://radio.example/old,Radio, Jazz,Poland,Jazz,\n";
    QVERIFY(writeFixture(ambiguousPath, ambiguous));
    const FavoriteManager ambiguousLegacy(ambiguousPath);
    QVERIFY(!ambiguousLegacy.setFavorite(newStation, true, loaded).isEmpty());
    QCOMPARE(fileContents(ambiguousPath), ambiguous);
    QCOMPARE(loaded.size(), 3);
}

void TuneScapeTests::testFavoriteViewOnlyUpdatesAfterSave()
{
    const QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath("radiobrowser.txt");
    FavoriteManager favorites(path);
    QMainWindow window;
    Ui::MainWindow ui;
    ui.setupUi(&window);
    RadioExplorer explorer(&ui);
    explorer.createMenu();
    RadioList radioList(&ui, &favorites);
    radioList.setMainWindow(&window);

    const auto first = favoriteStation();
    const auto second = favoriteStation("https://radio.example/Second");
    radioList.setFavorite(first, true);
    radioList.setFavoriteLibrary();
    QCOMPARE(ui.tableView->model()->rowCount(), 1);

    QLockFile lock(path + ".lock");
    QVERIFY(lock.tryLock(0));
    bool showedError = false;
    QTimer::singleShot(0, &window, [&] {
        auto *message = window.findChild<QMessageBox *>();
        if (message) {
            showedError = message->icon() == QMessageBox::Warning;
            message->accept();
        }
    });
    radioList.setFavorite(second, true);
    QVERIFY(showedError);
    QCOMPARE(ui.tableView->model()->rowCount(), 1);
    QVERIFY(!radioList.isFavorite(second));

    lock.unlock();
    radioList.setFavorite(second, true);
    QCOMPARE(ui.tableView->model()->rowCount(), 2);
    QVERIFY(radioList.isFavorite(second));
}

void TuneScapeTests::testStopCountryStationFromContextMenu()
{
    const QTemporaryDir directory;
    QVERIFY(directory.isValid());
    TestHttpServer server;
    QVERIFY(server.listen());
    RadioBrowserApi catalog({server.url("/")}, 1500);
    QMainWindow window;
    Ui::MainWindow ui;
    ui.setupUi(&window);

    FavoriteManager favorites;
    ControlledAudio audio;
    PlaybackController playback(audio);
    RadioList radioList(&ui, &favorites, &catalog, &playback);
    radioList.setMainWindow(&window);
    Country country;
    country.setData(&ui, &radioList);

    auto *model = qobject_cast<RadioStationsModel *>(ui.tableOfCoutries->model());
    QVERIFY(model);
    const auto station = playbackStation("Test station", server.url("/country-metadata"));
    model->setStations({station});
    emit ui.tableOfCoutries->doubleClicked(model->index(0, 0));
    audio.confirmPlaying();
    QCOMPARE(playback.state(), PlaybackController::State::Playing);
    auto &recorder = radioList.getStreamRecorder();
    recorder.startRecording(server.url("/country"), station.station, "MP3", directory.path());
    QTRY_VERIFY(server.client("/country"));
    QVERIFY(server.respond(server.client("/country"), "country audio"));
    QTRY_COMPARE(recorder.state(), StreamRecorder::State::Recording);
    ui.infoData->setText(QStringLiteral("Test track"));

    ui.tabRadioListWidget->setCurrentWidget(ui.listCountries);
    ui.worldImage->hide();
    ui.tableOfCoutries->show();
    window.show();

    const QRect stationRect = ui.tableOfCoutries->visualRect(model->index(0, 0));
    QVERIFY(stationRect.isValid());

    bool foundStopAction = false;
    QTimer::singleShot(0, &window, [&] {
        auto *menu = ui.tableOfCoutries->findChild<QMenu *>();
        if (!menu)
            return;

        QAction *action = menu->actions().value(0);
        foundStopAction = action && action->text() == Country::tr("Stop");
        if (foundStopAction) {
            menu->setActiveAction(action);
            QTest::keyClick(menu, Qt::Key_Return);
        }
        if (menu->isVisible())
            menu->close();
    });
    emit ui.tableOfCoutries->customContextMenuRequested(stationRect.center());

    QVERIFY(foundStopAction);
    QCOMPARE(playback.state(), PlaybackController::State::Stopped);
    QVERIFY(!playback.station());
    QCOMPARE(audio.stops, 1);
    QCOMPARE(recorder.state(), StreamRecorder::State::Idle);
    QCOMPARE(fileContents(recorder.filePath()), QByteArray("country audio"));
    QCOMPARE(ui.record->toolTip(), RadioList::tr("Start recording"));
    QTRY_COMPARE(server.client("/country")->state(), QAbstractSocket::UnconnectedState);
    QVERIFY(ui.infoData->text().isEmpty());
}

void TuneScapeTests::testRecordingStartAndStop()
{
    const QTemporaryDir directory;
    QVERIFY(directory.isValid());
    TestHttpServer server;
    QVERIFY(server.listen());
    StreamRecorder recorder;
    using State = StreamRecorder::State;
    QSignalSpy states(&recorder, &StreamRecorder::stateChanged);
    QSignalSpy progress(&recorder, &StreamRecorder::bytesWrittenChanged);
    QSignalSpy finished(&recorder, &StreamRecorder::recordingFinished);
    QSignalSpy failed(&recorder, &StreamRecorder::recordingFailed);

    recorder.startRecording(server.url("/live"), "Station", "MP3", directory.path());
    QCOMPARE(recorder.state(), State::Starting);
    QTRY_VERIFY(server.client("/live"));
    const QByteArray first("a\0b", 3);
    QVERIFY(server.respond(server.client("/live"), first));
    QTRY_COMPARE(recorder.bytesWritten(), first.size());
    QCOMPARE(recorder.state(), State::Recording);
    QCOMPARE(progress.last().at(0).toLongLong(), first.size());

    const QByteArray next = "defg";
    QCOMPARE(server.client("/live")->write(next), next.size());
    QTRY_COMPARE(recorder.bytesWritten(), first.size() + next.size());
    QCOMPARE(progress.last().at(0).toLongLong(), first.size() + next.size());
    recorder.stopRecording();
    recorder.stopRecording();

    QCOMPARE(failed.size(), 0);
    QCOMPARE(finished.size(), 1);
    QCOMPARE(finished.first().at(0).toString(), recorder.filePath());
    QCOMPARE(finished.first().at(1).toLongLong(), first.size() + next.size());
    QCOMPARE(fileContents(recorder.filePath()), first + next);
    QCOMPARE(states.size(), 4);
    QCOMPARE(states.at(0).at(0).value<State>(), State::Starting);
    QCOMPARE(states.at(1).at(0).value<State>(), State::Recording);
    QCOMPARE(states.at(2).at(0).value<State>(), State::Stopping);
    QCOMPARE(states.at(3).at(0).value<State>(), State::Idle);
    QTRY_COMPARE(server.client("/live")->state(), QAbstractSocket::UnconnectedState);
}

void TuneScapeTests::testRecordingOpenFailure()
{
    const QTemporaryDir directory;
    QVERIFY(directory.isValid());
    TestHttpServer server;
    QVERIFY(server.listen());
    QMainWindow window;
    Ui::MainWindow ui;
    ui.setupUi(&window);
    FavoriteManager favorites;
    RadioList radioList(&ui, &favorites);
    auto &recorder = radioList.getStreamRecorder();
    QSignalSpy failed(&recorder, &StreamRecorder::recordingFailed);
    QSignalSpy finished(&recorder, &StreamRecorder::recordingFinished);

    recorder.startRecording(server.url("/unopened"), "Station", "MP3", directory.filePath("missing"));
    QCOMPARE(recorder.state(), StreamRecorder::State::Idle);
    QCOMPARE(finished.size(), 0);
    QCOMPARE(failed.size(), 1);
    QVERIFY(!failed.first().at(0).toString().isEmpty());
    QVERIFY(recorder.filePath().isEmpty());
    QCOMPARE(ui.record->toolTip(), RadioList::tr("Start recording"));
    QCOMPARE(ui.recordLabel->text(), RadioList::tr("Recording could not start"));
    QCOMPARE(ui.statusbar->currentMessage(), failed.first().at(0).toString());
    QVERIFY(QDir(directory.path()).isEmpty());
    QTest::qWait(50);
    QVERIFY(!server.client("/unopened"));
}

void TuneScapeTests::testRecordingNameCollision()
{
    const QTemporaryDir directory;
    QVERIFY(directory.isValid());
    TestHttpServer server;
    QVERIFY(server.listen());
    const QDateTime start(QDate(2026, 11, 16), QTime(12, 34, 56, 789));
    const QString original = directory.filePath("2026-11-16_12-34-56-789 Station.mp3");
    QVERIFY(writeFixture(original, "existing recording"));
    StreamRecorder recorder;
    recorder.startRecording(server.url("/collision"), "Station", "MP3", directory.path(), start);
    QCOMPARE(recorder.state(), StreamRecorder::State::Starting);
    QCOMPARE(QFileInfo(recorder.filePath()).fileName(), QString("2026-11-16_12-34-56-789 Station-2.mp3"));
    QTRY_VERIFY(server.client("/collision"));
    QVERIFY(server.respond(server.client("/collision"), "new recording", 13));
    QTRY_COMPARE(recorder.state(), StreamRecorder::State::Idle);
    QCOMPARE(fileContents(original), QByteArray("existing recording"));
    QCOMPARE(fileContents(recorder.filePath()), QByteArray("new recording"));
}

void TuneScapeTests::testRecordingSanitizesName()
{
    const QTemporaryDir directory;
    QVERIFY(directory.isValid());
    TestHttpServer server;
    QVERIFY(server.listen());
    StreamRecorder recorder;
    const QString station = QStringLiteral("Łódź/x\\a:b*?\"<>|\n. ");
    recorder.startRecording(server.url("/name"), station, "../mp3", directory.path());
    QCOMPARE(recorder.state(), StreamRecorder::State::Starting);
    const QFileInfo opened(recorder.filePath());
    QCOMPARE(opened.dir().canonicalPath(), QDir(directory.path()).canonicalPath());
    QVERIFY(opened.completeBaseName().endsWith(QStringLiteral(" Łódź_x_a_b_______")));
    QCOMPARE(opened.suffix(), QString("stream"));
    recorder.stopRecording();
    QVERIFY(QFileInfo::exists(opened.absoluteFilePath()));
}

void TuneScapeTests::testRecordingRestartsWithoutMixingSessions()
{
    const QTemporaryDir directory;
    QVERIFY(directory.isValid());
    TestHttpServer server;
    QVERIFY(server.listen());
    StreamRecorder recorder;
    QSignalSpy failed(&recorder, &StreamRecorder::recordingFailed);
    QSignalSpy finished(&recorder, &StreamRecorder::recordingFinished);
    recorder.startRecording(server.url("/first"), "First", "MP3", directory.path());
    QTRY_VERIFY(server.client("/first"));
    auto *first = server.client("/first");
    QVERIFY(server.respond(first, "old"));
    QTRY_COMPARE(recorder.bytesWritten(), 3);
    const QString firstPath = recorder.filePath();

    QCOMPARE(first->write("late"), 4);
    recorder.stopRecording();
    recorder.startRecording(server.url("/second"), "Second", "MP3", directory.path());
    QTRY_VERIFY(server.client("/second"));
    QVERIFY(server.respond(server.client("/second"), "new audio", 9));
    QTRY_COMPARE(recorder.state(), StreamRecorder::State::Idle);
    QTRY_COMPARE(first->state(), QAbstractSocket::UnconnectedState);
    QCOMPARE(finished.size(), 2);
    QCOMPARE(failed.size(), 0);
    QCOMPARE(fileContents(firstPath), QByteArray("old"));
    QCOMPARE(fileContents(recorder.filePath()), QByteArray("new audio"));
}

void TuneScapeTests::testRecordingConnectionEnds_data()
{
    QTest::addColumn<QByteArray>("body");
    QTest::addColumn<int>("contentLength");
    QTest::addColumn<QByteArray>("status");
    QTest::addColumn<QByteArray>("expected");
    QTest::addColumn<bool>("succeeded");
    QTest::newRow("complete-http-response") << QByteArray("audio") << 5 << QByteArray("200 OK")
                                            << QByteArray("audio") << true;
    QTest::newRow("interrupted-transfer") << QByteArray("partial audio") << 200 << QByteArray("200 OK")
                                          << QByteArray("partial audio") << false;
    QTest::newRow("http-error") << QByteArray("bad URL") << 7 << QByteArray("404 Not Found")
                               << QByteArray() << false;
    QTest::newRow("empty-response") << QByteArray() << 0 << QByteArray("200 OK") << QByteArray() << false;
}

void TuneScapeTests::testRecordingConnectionEnds()
{
    QFETCH(QByteArray, body);
    QFETCH(int, contentLength);
    QFETCH(QByteArray, status);
    QFETCH(QByteArray, expected);
    QFETCH(bool, succeeded);
    const QTemporaryDir directory;
    QVERIFY(directory.isValid());
    TestHttpServer server;
    QVERIFY(server.listen());
    StreamRecorder recorder;
    QSignalSpy finished(&recorder, &StreamRecorder::recordingFinished);
    QSignalSpy failed(&recorder, &StreamRecorder::recordingFailed);
    recorder.startRecording(server.url("/end"), "Station", "MP3", directory.path());
    QTRY_VERIFY(server.client("/end"));
    auto *client = server.client("/end");
    QVERIFY(server.respond(client, body, contentLength, status));
    if (contentLength > body.size())
        QTRY_COMPARE(recorder.bytesWritten(), expected.size());
    client->disconnectFromHost();

    QTRY_COMPARE(recorder.state(), StreamRecorder::State::Idle);
    QCOMPARE(finished.size(), succeeded ? 1 : 0);
    QCOMPARE(failed.size(), succeeded ? 0 : 1);
    if (!succeeded) {
        QVERIFY(!failed.first().at(0).toString().isEmpty());
        QCOMPARE(failed.first().at(1).toString(), recorder.filePath());
        QCOMPARE(failed.first().at(2).toLongLong(), expected.size());
    }
    QCOMPARE(recorder.bytesWritten(), expected.size());
    QVERIFY(QFileInfo::exists(recorder.filePath()));
    QCOMPARE(fileContents(recorder.filePath()), expected);
}

void TuneScapeTests::testRecordingShortWrites_data()
{
    QTest::addColumn<qint64>("writableBytes");
    QTest::addColumn<QByteArray>("expected");
    QTest::addColumn<bool>("succeeded");
    QTest::newRow("short-writes-complete") << qint64(100) << QByteArray("audio bytes") << true;
    QTest::newRow("disk-error-after-partial-write") << qint64(5) << QByteArray("audio") << false;
    QTest::newRow("disk-error-on-first-write") << qint64(0) << QByteArray() << false;
}

void TuneScapeTests::testRecordingShortWrites()
{
    QFETCH(qint64, writableBytes);
    QFETCH(QByteArray, expected);
    QFETCH(bool, succeeded);
    const QTemporaryDir directory;
    QVERIFY(directory.isValid());
    TestHttpServer server;
    QVERIFY(server.listen());
    LimitedWritesRecorder recorder(writableBytes);
    QSignalSpy finished(&recorder, &StreamRecorder::recordingFinished);
    QSignalSpy failed(&recorder, &StreamRecorder::recordingFailed);
    recorder.startRecording(server.url("/disk"), "Station", "MP3", directory.path());
    QTRY_VERIFY(server.client("/disk"));
    const QByteArray body = "audio bytes";
    const qint64 announcedLength = succeeded ? body.size() : -1;
    QVERIFY(server.respond(server.client("/disk"), body, announcedLength));
    QTRY_COMPARE(recorder.state(), StreamRecorder::State::Idle);
    QTRY_COMPARE(server.client("/disk")->state(), QAbstractSocket::UnconnectedState);

    QCOMPARE(finished.size(), succeeded ? 1 : 0);
    QCOMPARE(failed.size(), succeeded ? 0 : 1);
    if (!succeeded)
        QCOMPARE(failed.first().at(2).toLongLong(), expected.size());
    QCOMPARE(recorder.bytesWritten(), expected.size());
    QVERIFY(QFileInfo::exists(recorder.filePath()));
    QCOMPARE(fileContents(recorder.filePath()), expected);
}

void TuneScapeTests::testStationChangeStopsRecording()
{
    const QTemporaryDir directory;
    QVERIFY(directory.isValid());
    TestHttpServer server;
    QVERIFY(server.listen());
    QMainWindow window;
    Ui::MainWindow ui;
    ui.setupUi(&window);
    FavoriteManager favorites;
    RadioBrowserApi api({server.url("/")}, 1500);
    ControlledAudio audio;
    PlaybackController playback(audio);
    RadioList radioList(&ui, &favorites, &api, &playback);
    QSignalSpy titles(&radioList, &RadioList::trackTitleReceived);
    playback.play(playbackStation("Old station", server.url("/old-metadata")));
    audio.confirmPlaying();
    QTRY_VERIFY(server.client("/old-metadata"));
    auto *oldMetadata = server.client("/old-metadata");
    QVERIFY(server.respond(oldMetadata, "StreamTitle='Old';"));
    QTRY_COMPARE(titles.size(), 1);
    QCOMPARE(titles.first().at(0).toString(), QString("Old"));

    auto &recorder = radioList.getStreamRecorder();
    recorder.startRecording(server.url("/recording"), "Station", "MP3", directory.path());
    QTRY_VERIFY(server.client("/recording"));
    QVERIFY(server.respond(server.client("/recording"), "old audio"));
    QTRY_COMPARE(recorder.state(), StreamRecorder::State::Recording);
    QCOMPARE(ui.record->toolTip(), RadioList::tr("Stop recording"));
    QCOMPARE(ui.recordLabel->text(), RadioList::tr("Recording: %L1 bytes").arg(9));

    ui.playPause->click();
    QCOMPARE(playback.state(), PlaybackController::State::Paused);
    QCOMPARE(recorder.state(), StreamRecorder::State::Recording);
    QCOMPARE(server.client("/recording")->write(" plus"), 5);
    QTRY_COMPARE(recorder.bytesWritten(), 14);

    QCOMPARE(oldMetadata->write("StreamTitle='Late';"), 19);
    playback.play(playbackStation("New station", server.url("/new-metadata")));
    QCOMPARE(recorder.state(), StreamRecorder::State::Idle);
    QCOMPARE(ui.record->toolTip(), RadioList::tr("Start recording"));
    QCOMPARE(ui.recordLabel->text(), RadioList::tr("Recording saved (%L1 bytes)").arg(14));
    QCOMPARE(fileContents(recorder.filePath()), QByteArray("old audio plus"));
    QTRY_COMPARE(server.client("/recording")->state(), QAbstractSocket::UnconnectedState);
    QTRY_COMPARE(oldMetadata->state(), QAbstractSocket::UnconnectedState);
    QTRY_VERIFY(server.client("/new-metadata"));
    QVERIFY(server.respond(server.client("/new-metadata"), "StreamTitle='New';"));
    QTRY_COMPARE(titles.size(), 2);
    QCOMPARE(titles.last().at(0).toString(), QString("New"));
}

void TuneScapeTests::testRecorderDestructionClosesSession()
{
    const QTemporaryDir directory;
    QVERIFY(directory.isValid());
    TestHttpServer server;
    QVERIFY(server.listen());
    auto recorder = std::make_unique<StreamRecorder>();
    recorder->startRecording(server.url("/shutdown"), "Station", "MP3", directory.path());
    QTRY_VERIFY(server.client("/shutdown"));
    QVERIFY(server.respond(server.client("/shutdown"), "audio at shutdown"));
    QTRY_COMPARE(recorder->bytesWritten(), 17);
    const QString path = recorder->filePath();
    recorder.reset();

    QTRY_COMPARE(server.client("/shutdown")->state(), QAbstractSocket::UnconnectedState);
    QCOMPARE(fileContents(path), QByteArray("audio at shutdown"));
}

QTEST_MAIN(TuneScapeTests)

#include "tst_tunescapetests.moc"
