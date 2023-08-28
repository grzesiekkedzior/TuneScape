#include "jsonlistprocessor.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QMessageBox>

JsonListProcessor::JsonListProcessor() {}

void JsonListProcessor::loadEndpoint(QString endpoint)
{
    RadioStations radioStations(endpoint);
    for (;;) {
        reply = checkAvailability(radioStations.getAddresses());
        if (reply == nullptr) {
            QMessageBox mb;
            mb.setText("All services are not available.");
            mb.exec();
        } else {
            break;
        }
    }

}

JsonListProcessor::~JsonListProcessor()
{
    if (reply) {
        reply->deleteLater();
    }
}

void JsonListProcessor::processJsonQuery()
{
    tableRows.clear();
    doc = createJasonDocument(reply);
    if (!doc.isEmpty()) {

    }
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

QNetworkReply* JsonListProcessor::checkAvailability(const QStringList &radioAddresses)
{
    QEventLoop loop;
    int status;

    for (const QString &address : radioAddresses) {
        QNetworkRequest request((QUrl(address)));
        reply = manager.get(request);
        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();
        status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        qDebug() << status;
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
