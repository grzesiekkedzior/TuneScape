#include <QCoreApplication>
#include <QtTest>
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
    void test_case1();
};

TuneScapeTests::TuneScapeTests() {}

TuneScapeTests::~TuneScapeTests() {}

void TuneScapeTests::initTestCase() {}

void TuneScapeTests::cleanupTestCase() {}

void TuneScapeTests::test_case1() {}

QTEST_MAIN(TuneScapeTests)

#include "tst_tunescapetests.moc"
