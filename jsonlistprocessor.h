#ifndef JSONLISTPROCESSOR_H
#define JSONLISTPROCESSOR_H

#include "radiostations.h"

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QEventLoop>
#include <QNetworkReply>
#include <QJsonDocument>

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

signals:


private:
    const QString NAME      = "name";
    const QString GENRE     = "tags";
    const QString COUNTRY   = "country";
    const QString URL       = "homepage";
    QString                 endpoint;
    QVector<TableRow>       tableRows;
    QNetworkAccessManager   manager;
    QNetworkReply           *reply;
    QJsonDocument           doc;
    RadioStations           severAddress;

    QNetworkReply* checkAvailability(const QStringList &radioAddresses);
    QJsonDocument createJasonDocument(QNetworkReply *reply);
};

#endif // JSONLISTPROCESSOR_H
