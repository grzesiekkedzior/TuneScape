#include "jsonlistprocessor.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QMessageBox>
#include <QProgressBar>
#include <QTimer>

JsonListProcessor::JsonListProcessor() {
    connectionTimer = new QTimer(this);
    connectionTimer->setInterval(5000);
    connect(connectionTimer, &QTimer::timeout, this, &JsonListProcessor::checkConnection);
    connectionTimer->start();
}

void JsonListProcessor::loadEndpoint(QString endpoint)
{
    this->endpoint = endpoint;
    RadioStations radioStations(endpoint);

    ui->statusbar->showMessage("Connecting...");

    while (true) {
        reply = checkAvailability(radioStations.getAddresses());

        if (reply) {
            setConnection(reply);
            break;
        }
    }
}

void JsonListProcessor::setConnection(QNetworkReply *connectionReply)
{
    if (connectionReply == nullptr) {
        ui->statusbar->showMessage("Connection lost");
        ui->statusbar->setStyleSheet("color: red");
    } else {
        ui->statusbar->showMessage("Connected");
        ui->statusbar->setStyleSheet("color: green");
    }
}

void JsonListProcessor::checkConnection()
{
    if (endpoint.isEmpty()) {
        qDebug() << "Endpoint not set";
        return;
    }

    RadioStations radioStations(endpoint);
    QNetworkReply *connectionReply = checkAvailability(radioStations.getAddresses());

    setConnection(connectionReply);
}

void JsonListProcessor::setUi(Ui::MainWindow *ui)
{
    this->ui = ui;
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
    int status = -1;
    QNetworkReply* currentReply = nullptr;

    QEventLoop loop;
    for (const QString &address : radioAddresses) {
        QNetworkRequest request((QUrl(address)));
        currentReply = manager.get(request);
        QObject::connect(currentReply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();
        status = currentReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        qDebug() << status;
        if (currentReply->error() == QNetworkReply::NoError) break;
    }

    if (status == 200) {
        return currentReply;
    } else {
        if (currentReply) {
            currentReply->deleteLater();
        }
        return nullptr;
    }
}

QJsonDocument JsonListProcessor::createJasonDocument(QNetworkReply *reply)
{
    QByteArray data = reply->readAll();
    doc = QJsonDocument::fromJson(data);
    return doc;
}
