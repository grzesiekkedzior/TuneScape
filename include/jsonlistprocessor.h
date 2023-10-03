#ifndef JSONLISTPROCESSOR_H
#define JSONLISTPROCESSOR_H


#include "include/radiostations.h"
#include "../ui_mainwindow.h"

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QEventLoop>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QTimer>

class RadioList;

struct TableRow {
    QString station;
    QString genre;
    QString country;
    QString stationUrl;
};

class JsonListProcessor : public QObject
{
    Q_OBJECT
public:
    JsonListProcessor();
    ~JsonListProcessor();

    void processJsonQuery();
    QVector<TableRow>& getTableRows();
    void loadEndpoint(QString endpoint);
    void setUi(Ui::MainWindow *ui);
    void setRadioList(RadioList *radioList);
    bool checkInternetConnection();
    void lostConnection();
    void connected();
    QVector<QString>& getStreamAddresses();
    QVector<QString>& getIconAddresses();
    QString getStreamAddresses(int index) const;
    QString getIconAddresses(int index) const;

    void setTableRows(const QVector<TableRow> &rows);
    void setStreamAddresses(const QVector<QString> &addresses);
    void setIconAddresses(const QVector<QString> &icons);


private:
    const QString NAME          = "name";
    const QString GENRE         = "tags";
    const QString COUNTRY       = "country";
    const QString URL           = "homepage";
    const QString URL_RESOLVED  = "url_resolved";
    const QString FAVICON       = "favicon";
    QString endpoint            = "";

    QVector<TableRow>       tableRows;
    QVector<QString>        streamAddresses;
    QVector<QString>        iconAddresses;

    QNetworkAccessManager   manager;
    QNetworkReply           *reply;
    QJsonDocument           doc;
    RadioStations           severAddress;
    Ui::MainWindow          *ui;
    QTimer                  *connectionTimer;
    QTimer                  *internetConnectionChecker;
    RadioList               *radioList = nullptr;

    QNetworkReply* checkAvailability(const QStringList &radioAddresses);
    QJsonDocument createJasonDocument(QNetworkReply *reply);
    void retryInternetConnection();
    void setConnection(QNetworkReply *connectionReply);

};

#endif // JSONLISTPROCESSOR_H
