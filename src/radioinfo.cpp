#include "include/radioinfo.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QProgressBar>
#include <QTableWidgetItem>
#include <QTcpSocket>
#include <QTimer>

RadioInfo::RadioInfo() {}

RadioInfo::RadioInfo(Ui::MainWindow *ui)
    : ui(ui)
{}

void RadioInfo::setInfo()
{
    // load endpoint
    // process query
    // set data
}

void RadioInfo::loadEndpoint(const QString station)
{
    // ENDPOIN + station here is an adres of station "advance search" check dockumentation
    qDebug() << ENDPOINT + station;
    RadioStations radioStations(ENDPOINT + station);
    if (jsonListProcessor.checkInternetConnection()) {
        // important load first available server!!!
        if (reply)
            reply->deleteLater();
        reply = jsonListProcessor.checkAvailability(radioStations.getAddresses());
    }
}

void RadioInfo::processInfoJsonQuery()
{
    if (reply)
        doc = jsonListProcessor.createJasonDocument(reply);

    if (doc.isArray()) {
        QJsonArray stationsArray = doc.array();
        for (const QJsonValue &value : stationsArray) {
            // I can do this in loop. This is more readible for long maintainance
            QJsonObject stationObject = value.toObject();
            infoData.favicon = stationObject[FAVICON].toString();
            infoData.station = stationObject[STATION].toString().trimmed();
            infoData.country = stationObject[COUNTRY].toString().trimmed();
            infoData.state = stationObject[STATE].toString().trimmed();
            infoData.language = stationObject[LANGUAGE].toString().trimmed();
            infoData.votes = QString::number(stationObject[VOTES].toInt());
            infoData.codec = stationObject[CODEC].toString().trimmed();
            infoData.bitrate = QString::number(stationObject[BITRATE].toInt());
            infoData.clickcount = QString::number(stationObject[CLICKCOUNT].toInt());
            infoData.clicktrend = QString::number(stationObject[CLICKTREND].toInt());
        }
    }
}

void RadioInfo::setDataOnTable()
{
    ui->tableWidget->setItem(0, 1, new QTableWidgetItem(infoData.station));
    ui->tableWidget->setItem(1, 1, new QTableWidgetItem(infoData.country));
    ui->tableWidget->setItem(2, 1, new QTableWidgetItem(infoData.state));
    ui->tableWidget->setItem(3, 1, new QTableWidgetItem(infoData.language));
    ui->tableWidget->setItem(4, 1, new QTableWidgetItem(infoData.votes));
    ui->tableWidget->setItem(5, 1, new QTableWidgetItem(infoData.codec));
    ui->tableWidget->setItem(6, 1, new QTableWidgetItem(infoData.bitrate));
    ui->tableWidget->setItem(7, 1, new QTableWidgetItem(infoData.clickcount));
    ui->tableWidget->setItem(8, 1, new QTableWidgetItem(infoData.clicktrend));
    QPixmap pixmapFromInfoLabel = ui->infoLabel->pixmap();
    ui->radioIcon->setPixmap(pixmapFromInfoLabel);
}

RadioInfo::~RadioInfo()
{
    if (reply) {
        reply->deleteLater();
    }
}
