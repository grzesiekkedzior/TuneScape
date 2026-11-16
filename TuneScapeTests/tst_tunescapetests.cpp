#include <QDir>
#include <QFile>
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
#include "include/radiolist.h"

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
    country.setFavoriteManager(&favorites);

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
