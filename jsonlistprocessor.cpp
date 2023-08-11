#include "jsonlistprocessor.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkRequest>

//JsonListProcessor::JsonListProcessor(QObject *parent)
//    : QObject{parent}
//{

//}

JsonListProcessor::JsonListProcessor()
{
    serverAddress sw;
    reply = checkAvailability(sw);
    QEventLoop loop;
    //This loop wait until request finished, then close it
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
}

JsonListProcessor::~JsonListProcessor()
{
    if (reply) {
        reply->deleteLater();
    }
}

void JsonListProcessor::processJsonQuery()
{
    doc = createJasonDocument(reply);
    if (doc.isArray())
    {
        QJsonArray stationsArray = doc.array();

        for (const QJsonValue &value : stationsArray)
        {
            QJsonObject stationObject = value.toObject();

            QString stationName = stationObject[NAME].toString();
            QString genre = stationObject[GENRE].toString();
            QString country = stationObject[COUNTRY].toString();
            QString stationUrl = stationObject[URL].toString();

            stationName = stationName.trimmed().replace(QRegularExpression("^[\\s?_.-]+"), "");
            genre       = genre.left(genre.indexOf(',')).trimmed();
            country     = country.trimmed();
            stationUrl  = stationUrl.trimmed();


            TableRow row;
            row.station = stationName;
            row.genre = genre;
            row.country = country;
            row.stationUrl = stationUrl;

            tableRows.append(row);
        }
    }
}

QVector<TableRow> &JsonListProcessor::getTableRows()
{
    return tableRows;
}

QNetworkReply* JsonListProcessor::checkAvailability(serverAddress sw)
{
    for (const QString &address : sw.addresses) {
        QNetworkRequest request((QUrl(address)));
        reply = manager.get(request);

        if (reply->error() == QNetworkReply::NoError) return reply;
    }

    return nullptr;
}

QJsonDocument JsonListProcessor::createJasonDocument(QNetworkReply *reply)
{
    QByteArray data = reply->readAll();
    doc = QJsonDocument::fromJson(data);
    return doc;
}
