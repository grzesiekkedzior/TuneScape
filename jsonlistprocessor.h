#ifndef JSONLISTPROCESSOR_H
#define JSONLISTPROCESSOR_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QEventLoop>
#include <QNetworkReply>
#include <QJsonDocument>

struct serverAddress {
    QStringList addresses = {
        "http://de1.api.radio-browser.info/json/stations",
        "https://fr1.api.radio-browser.info/json/stations",
        "https://at1.api.radio-browser.info/json/stations"
    };
};

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
    //explicit JsonListProcessor(QObject *parent = nullptr);
    JsonListProcessor();
    ~JsonListProcessor();

    void processJsonQuery();
    QVector<TableRow>& getTableRows();

signals:


private:
    const QString NAME      = "name";
    const QString GENRE     = "tags";
    const QString COUNTRY   = "country";
    const QString URL       = "homepage";
    QVector<TableRow>       tableRows;
    QNetworkAccessManager   manager;
    QNetworkReply           *reply;
    QJsonDocument           doc;

    QNetworkReply* checkAvailability(serverAddress);
    QJsonDocument createJasonDocument(QNetworkReply *reply);
};

#endif // JSONLISTPROCESSOR_H
