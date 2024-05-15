#include <QCoreApplication>
#include <QtTest>
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
};

TuneScapeTests::TuneScapeTests() {}

TuneScapeTests::~TuneScapeTests() {}

void TuneScapeTests::initTestCase() {}

void TuneScapeTests::cleanupTestCase() {}

void TuneScapeTests::testRadioListPause()
{
    RadioList radioList;
    QCOMPARE_EQ(radioList.getIsPause(), false);
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

QTEST_MAIN(TuneScapeTests)

#include "tst_tunescapetests.moc"
