#include "jsonlistprocessor.h"
#include "radiolist.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QProgressBar>
#include <QTimer>
#include <QTcpSocket>

JsonListProcessor::JsonListProcessor() {
    connectionTimer = new QTimer(this);
    connectionTimer->setInterval(5000);
    connect(connectionTimer, &QTimer::timeout, this, &JsonListProcessor::checkInternetConnection);
    connectionTimer->start();

    internetConnectionChecker = new QTimer(this);
    internetConnectionChecker->setInterval(5000);
    connect(internetConnectionChecker, &QTimer::timeout, this, &JsonListProcessor::retryInternetConnection);
}

void JsonListProcessor::loadEndpoint(QString endpoint)
{
    this->endpoint = endpoint;

    RadioStations radioStations(endpoint);
    ui->statusbar->showMessage("Connecting...");

    if (checkInternetConnection()) {
        reply = checkAvailability(radioStations.getAddresses());
        setConnection(reply);
    } else {
        internetConnectionChecker->start();
    }
}

void JsonListProcessor::connected()
{
    ui->statusbar->showMessage("Connected");
    ui->statusbar->setStyleSheet("color: green");
}

QVector<QString> &JsonListProcessor::getStreamAddresses()
{
    return this->streamAddresses;
}

QString JsonListProcessor::getStreamAddresses(int index) const
{
    return this->streamAddresses[index];
}

void JsonListProcessor::setConnection(QNetworkReply *connectionReply)
{
    if (connectionReply == nullptr) {
        lostConnection();
    } else {
        connected();
    }
}

bool JsonListProcessor::checkInternetConnection()
{
    bool isConnected = false;
    QTcpSocket checkConnectionSocket;
    checkConnectionSocket.connectToHost("google.com", 443);
    checkConnectionSocket.waitForConnected(2000);

    if (checkConnectionSocket.state() == QTcpSocket::ConnectedState) {
        isConnected = true;
    } else {
        lostConnection();
        internetConnectionChecker->start();
    }

    checkConnectionSocket.close();
    return isConnected;
}

void JsonListProcessor::lostConnection()
{
    ui->statusbar->showMessage("Connection lost");
    ui->statusbar->setStyleSheet("color: red");
}

void JsonListProcessor::retryInternetConnection()
{
    if (!checkInternetConnection()) {
        lostConnection();
    } else {
        ui->statusbar->showMessage("Connected");
        ui->statusbar->setStyleSheet("color: green");
        internetConnectionChecker->stop();
        loadEndpoint(endpoint);
        radioList->setLoadedStationsCount(0);
        processJsonQuery();
        radioList->loadRadioList();
    }
}

void JsonListProcessor::setUi(Ui::MainWindow *ui)
{
    this->ui = ui;
}

void JsonListProcessor::setRadioList(RadioList *radioList)
{
    this->radioList = radioList;
}

JsonListProcessor::~JsonListProcessor()
{
    if (reply) {
        reply->deleteLater();
    }
    if (radioList) {
        delete radioList;
    }
}

void JsonListProcessor::processJsonQuery()
{
    tableRows.clear();
    if (reply)
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

            QString streamUrl = stationObject["url_resolved"].toString();
            this->streamAddresses.push_back(streamUrl);
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
        qDebug() << "status " << status;
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
