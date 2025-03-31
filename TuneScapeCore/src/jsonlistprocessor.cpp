#include "include/jsonlistprocessor.h"
#include <QHostInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QProgressBar>
#include <QTcpSocket>
#include <QTimer>
#include "include/radiolist.h"
#include <QMessageBox>
#include <QProgressDialog>

JsonListProcessor::JsonListProcessor()
{
    connectionTimer = new QTimer(this);
    connectionTimer->setInterval(12000);
    connect(connectionTimer, &QTimer::timeout, this, &JsonListProcessor::checkInternetConnection);

    internetConnectionChecker = new QTimer(this);
    internetConnectionChecker->setInterval(5000);
    connect(internetConnectionChecker,
            &QTimer::timeout,
            this,
            &JsonListProcessor::retryInternetConnection);
}

void JsonListProcessor::loadEndpoint(QString endpoint)
{
    this->endpoint = endpoint;

    RadioStations radioStations(endpoint);
    ui->statusbar->showMessage("Connecting...");
    if (!connectionTimer->isActive())
        connectionTimer->start();
    if (checkInternetConnection()) {
        reply = checkAvailability(radioStations.getAddresses());
        setConnection(reply);
    } else {
        qDebug() << "Connection checker starts";
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

QString JsonListProcessor::getIconAddresses(int index) const
{
    return this->iconAddresses[index];
}

QVector<QString> &JsonListProcessor::getIconAddresses()
{
    return this->iconAddresses;
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
    isConnected = false;
    QTimer timer;

    // Connect to the DNS server and check if the name "google.com" resolves to an IP address.
    QHostInfo hostInfo = QHostInfo::fromName("google.com");
    if (!hostInfo.error()) {
        isConnected = true;
    } else {
        lostConnection();
        qDebug() << "connection failed: " << hostInfo.errorString();
        internetConnectionChecker->start();
    }

    return isConnected;
}

void JsonListProcessor::lostConnection()
{
    if (radioList->getMainWindow()->isHidden())
        radioList->getMainWindow()->show();
    ui->statusbar->showMessage("Connection lost");
    ui->statusbar->setStyleSheet("color: red");
    messagebox.setText("Connection is lost!!!");
    messagebox.setIcon(QMessageBox::Information);
    messagebox.setWindowIcon(QIcon(":/images/img/radio30.png"));
    messagebox.show();
    if (radioList->getStreamRecorder()->getIsRecording()) {
        radioList->getStreamRecorder()->stopRecording();
        radioList->getStreamRecorder()->setIsRecording(false);
    }
}

void JsonListProcessor::retryInternetConnection()
{
    if (!checkInternetConnection()) {
        lostConnection();
    } else {
        ui->statusbar->showMessage("Connected");
        ui->statusbar->setStyleSheet("color: green");
        internetConnectionChecker->stop();
        connectionTimer->start();
        //loadEndpoint(endpoint);
        //radioList->setLoadedStationsCount(0);
        //processJsonQuery();
        radioList->onInternetConnectionRestored();
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
    //    if (radioList) {
    //        delete radioList;
    //    }
}

void JsonListProcessor::processJsonQuery()
{
    tableRows.clear();
    streamAddresses.clear();
    iconAddresses.clear();
    if (reply)
        doc = createJasonDocument(reply);

    if (doc.isArray()) {
        QJsonArray stationsArray = doc.array();

        for (const QJsonValue &value : stationsArray) {
            QJsonObject stationObject = value.toObject();
            QString stationName = stationObject[NAME].toString();
            QString genre = stationObject[GENRE].toString();
            QString country = stationObject[COUNTRY].toString();
            QString stationUrl = stationObject[URL].toString();

            stationName = stationName.trimmed().replace(QRegularExpression("^[\\s?_.-]+"), "");
            genre = genre.left(genre.indexOf(',')).trimmed();
            country = country.trimmed();
            stationUrl = stationUrl.trimmed();

            TableRow row;
            row.station = stationName;
            row.genre = genre;
            row.country = country;
            row.stationUrl = stationUrl;

            tableRows.append(row);

            QString streamUrl = stationObject[URL_RESOLVED].toString();
            QString iconUrl = stationObject[FAVICON].toString();
            this->streamAddresses.push_back(streamUrl);
            this->iconAddresses.push_back(iconUrl);
        }
    }
}

QVector<TableRow> &JsonListProcessor::getTableRows()
{
    return tableRows;
}

QNetworkReply* JsonListProcessor::checkAvailability(const QStringList& radioAddresses)
{
    QEventLoop loop;
    int status;

    // Creating "Connecting..." dialog
    QProgressDialog progressDialog("Connecting to servers...", "Cancel", 0, radioAddresses.size());
    progressDialog.setWindowModality(Qt::WindowModal);
    progressDialog.setMinimumDuration(500); // Shows only if the operation lasts >500ms
    progressDialog.setWindowIcon(QIcon(":/images/img/radio30.png"));
    progressDialog.setAutoClose(true);

    for (int i = 0; i < radioAddresses.size(); ++i) {
        const QString& address = radioAddresses[i];
        progressDialog.setValue(i);
        progressDialog.setLabelText(QString("Attempting connection to %1...").arg(address));

        // If the user cancels, break the loop
        if (progressDialog.wasCanceled()) {
            qDebug() << "Connection check canceled by user";
            break;
        }

        QNetworkRequest request((QUrl(address)));
        reply = manager.get(request);
        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();

        status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        QString errorString = reply->errorString();

        if (reply->error() == QNetworkReply::NoError) {
            qDebug() << "Connection successful:" << address << "Status code:" << status;
            progressDialog.close();  // Force immediate dialog closure
            return reply;
        } else {
            qDebug() << "Connection failed:" << address << "Status:" << status
                     << "Error:" << errorString;

            QMessageBox::critical(nullptr, "Connection Error",
                                  QString("Failed to connect to %1.\nError: %2")
                                      .arg(address, errorString));

            reply->deleteLater();
        }
    }

    progressDialog.setValue(radioAddresses.size()); // Complete progress if all attempts failed
    qDebug() << "No available servers found";
    return nullptr;
}


QJsonDocument JsonListProcessor::createJasonDocument(QNetworkReply *reply)
{
    QByteArray data = reply->readAll();
    doc = QJsonDocument::fromJson(data);
    return doc;
}

void JsonListProcessor::setTableRows(const QVector<TableRow> &rows)
{
    tableRows = rows;
}

void JsonListProcessor::setStreamAddresses(const QVector<QString> &addresses)
{
    streamAddresses = addresses;
}

void JsonListProcessor::setIconAddresses(const QVector<QString> &icons)
{
    iconAddresses = icons;
}
