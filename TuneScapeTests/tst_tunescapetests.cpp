#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLockFile>
#include <QMainWindow>
#include <QMenu>
#include <QNetworkProxy>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QTimer>
#include <QtTest>
#include "include/AppConfig.h"
#include "include/RadioBrowserApi.h"
#include "include/miniplayer.h"
#include "include/radioexplorer.h"
#include "include/radiolist.h"
#include <filesystem>

namespace {

class LocalHttpResponse
{
public:
    explicit LocalHttpResponse(const QByteArray &body)
    {
        QObject::connect(&server, &QTcpServer::newConnection, &server, [this, body] {
            auto *socket = server.nextPendingConnection();
            QObject::connect(socket, &QTcpSocket::readyRead, socket, [this, socket, body] {
                request += socket->readAll();
                if (!request.contains("\r\n\r\n"))
                    return;

                socket->write("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n"
                              "Connection: close\r\nContent-Length: "
                              + QByteArray::number(body.size()) + "\r\n\r\n" + body);
                socket->disconnectFromHost();
            });
        });
    }

    bool listen() { return server.listen(QHostAddress::LocalHost); }

    QString url() const
    {
        return QStringLiteral("http://127.0.0.1:%1/stations").arg(server.serverPort());
    }

    QByteArray request;

private:
    QTcpServer server;
};

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
    void testJsonListProcessorConnection();
    void testJsonListProcessorStationLoaded();
    void testJsonListProcessorInvalidJson();
    void testRadioStationsGenerateFullAddress();
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
    QMainWindow window;
    Ui::MainWindow ui;
    ui.setupUi(&window);
    FavoriteManager favorites;
    RadioList radioList(&ui, &favorites);
    radioList.setIsTreeClicked(true);
    radioList.getJsonListProcessor()->isConnected = true;

    auto &playback = SingletonContainer::getSingleton().getInstance<PlaybackController>();
    playback.stop();
    playback.setPlaying(true);
    ui.playPause->click();

    QVERIFY(playback.isPaused());
    QVERIFY(!playback.isPlaying());
    playback.stop();
}

void TuneScapeTests::testJsonListProcessorConnection()
{
    LocalHttpResponse http("[]");
    QVERIFY(http.listen());

    JsonListProcessor jsonListProcessor;
    const auto *reply = jsonListProcessor.checkAvailability({http.url()});
    QVERIFY(reply);
    QVERIFY(reply->isFinished());
    QCOMPARE(reply->error(), QNetworkReply::NoError);
    QCOMPARE(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 200);
    QVERIFY(http.request.startsWith("GET /stations HTTP/1.1\r\n"));
}

void TuneScapeTests::testJsonListProcessorStationLoaded()
{
    LocalHttpResponse http(R"json([{
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
    }])json");
    QVERIFY(http.listen());

    JsonListProcessor processor;
    QVERIFY(processor.checkAvailability({http.url()}));
    processor.processJsonQuery();
    const auto stations = processor.getStations();
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
    QCOMPARE(processor.getTableRows().size(), 1);
    QCOMPARE(processor.getTableRows().first().station, station.station);
    QCOMPARE(processor.getStreamAddresses(), QVector<QString>{station.streamUrl});
    QCOMPARE(processor.getIconAddresses(), QVector<QString>{station.iconUrl});
}

void TuneScapeTests::testJsonListProcessorInvalidJson()
{
    LocalHttpResponse http("[invalid]");
    QVERIFY(http.listen());

    JsonListProcessor processor;
    QVERIFY(processor.checkAvailability({http.url()}));
    processor.processJsonQuery();
    QVERIFY(processor.getStations().isEmpty());
}

void TuneScapeTests::testRadioStationsGenerateFullAddress()
{
    const QString endpoint = QStringLiteral("json/stations/topvote/20");
    RadioStations radioStations(endpoint,
                                {RadioBrowserApi::baseUrlForTarget(QStringLiteral("first.example.")),
                                 RadioBrowserApi::baseUrlForTarget(QStringLiteral("second.example"))});
    const QStringList expected = {QStringLiteral("https://first.example/json/stations/topvote/20"),
                                  QStringLiteral("https://second.example/json/stations/topvote/20")};
    QCOMPARE(radioStations.getAddresses(), expected);
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
    const QWidget &window = miniPlayer;
    QVERIFY(miniPlayer.getIsVisible());
    QVERIFY(!window.isVisible());
    miniPlayer.showMiniPlayer();
    QVERIFY(window.isVisible());
    miniPlayer.setIsVisible(false);
    QVERIFY(!miniPlayer.getIsVisible());
    miniPlayer.showMiniPlayer();
    QVERIFY(!window.isVisible());
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
    QMainWindow window;
    Ui::MainWindow ui;
    ui.setupUi(&window);

    FavoriteManager favorites;
    RadioList radioList(&ui, &favorites);
    radioList.setMainWindow(&window);
    auto &container = SingletonContainer::getSingleton();
    auto &country = container.getInstance<Country>();
    auto &playback = container.getInstance<PlaybackController>();
    country.setData(&ui, &radioList);

    // Seed playback state so the Stop action can be exercised without a live station.
    auto *model = qobject_cast<RadioStationsModel *>(ui.tableOfCoutries->model());
    QVERIFY(model);
    RadioStation station = country.getCurrentStation();
    station.station = QStringLiteral("Test station");
    model->setStations({station});
    country.setCurrentIndexPlaying(0);
    country.setIsPlaying(true);
    playback.setPlaying(true);
    radioList.setIsBrowseStationLoaded(true);
    radioList.setIsStopClicked(false);
    const auto recorder = radioList.getStreamRecorder();
    recorder->setIsRecording(true);
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
    QVERIFY(!playback.isPlaying());
    QVERIFY(!country.getIsPlaying());
    QCOMPARE(country.getCurrentIndexPlaying(), -1);
    QVERIFY(radioList.getIsStopClicked());
    QVERIFY(!radioList.getIsBrowseStationLoaded());
    QVERIFY(!recorder->getIsRecording());
    QVERIFY(ui.infoData->text().isEmpty());
}

QTEST_MAIN(TuneScapeTests)

#include "tst_tunescapetests.moc"
