#include <QCoreApplication>
#include <QMainWindow>
#include <QMenu>
#include <QTimer>
#include <QtTest>
#include "include/AppConfig.h"
#include "include/miniplayer.h"
#include "include/radiolist.h"
// add necessary includes here

class TuneScapeTests : public QObject
{
    Q_OBJECT

public:
    TuneScapeTests();
    ~TuneScapeTests();

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testRadioListPause();
    void testJsonListProcesorConnection();
    void testJsonListProcesorStationLoaded();
    void testRadioStationsGenerateFullAddress();
    void testAppConfigCheckFileIsExists();
    void testAppConfigCheckBoolState();
    void testAppConfigChangeBoolState();
    void testMiniPlayerIsVisible();
    void testStopCountryStationFromContextMenu();
};

TuneScapeTests::TuneScapeTests() {}

TuneScapeTests::~TuneScapeTests() {}

void TuneScapeTests::initTestCase() {}

void TuneScapeTests::cleanupTestCase() {}

void TuneScapeTests::testRadioListPause()
{
    //TODO
}

void TuneScapeTests::testJsonListProcesorConnection()
{
    JsonListProcessor jsonListProcessor;
    // turn on or off network
    QCOMPARE(jsonListProcessor.checkInternetConnection(), true);
}

void TuneScapeTests::testJsonListProcesorStationLoaded()
{
    JsonListProcessor jsonListProcesor;
    jsonListProcesor.processJsonQuery();
    QCOMPARE(jsonListProcesor.getTableRows().size(), jsonListProcesor.getStreamAddresses().size());
}

void TuneScapeTests::testRadioStationsGenerateFullAddress()
{
    const QString SERVER = "http://de1.api.radio-browser.info/";
    const QString JSON_ENDPOINT_TOP = "json/stations/topvote/20";
    const QString FULL_ADDRESS = SERVER + JSON_ENDPOINT_TOP;
    RadioStations radioStations(JSON_ENDPOINT_TOP);
    QCOMPARE(radioStations.getAddresses().at(0), FULL_ADDRESS);
}

void TuneScapeTests::testAppConfigCheckFileIsExists()
{
    AppConfig appConfig{"C:\\QtProjects\\TuneScape\\TuneScapeTests\\application.properties"};
    bool status = appConfig.checkFile();
    QCOMPARE(status, true);
}

void TuneScapeTests::testAppConfigCheckBoolState()
{
    AppConfig appConfig{"C:\\QtProjects\\TuneScape\\TuneScapeTests\\application.properties"};
    // file is false for now
    appConfig.changeBoolState(false, "notifications");
    bool state = appConfig.checkBoolState("notifications");
    QCOMPARE(state, false);
}

void TuneScapeTests::testAppConfigChangeBoolState()
{
    AppConfig appConfig{"C:\\QtProjects\\TuneScape\\TuneScapeTests\\application.properties"};
    // file is false for now
    bool state = appConfig.changeBoolState(false,"notifications");
    QCOMPARE(state, true);
}

void TuneScapeTests::testMiniPlayerIsVisible()
{
    miniplayer miniPlayer;
    bool state = miniPlayer.getIsVisible();
    QCOMPARE_NE(state, true);
    miniPlayer.setIsVisible(false);
    QCOMPARE(state, false);
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
