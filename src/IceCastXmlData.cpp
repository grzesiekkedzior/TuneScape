#include "include/IceCastXmlData.h"
#include <QFuture>
#include <QFutureWatcher>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QXmlStreamReader>
#include <QtConcurrent>
#include "include/radiolist.h"

IceCastXmlData::IceCastXmlData() {}

IceCastXmlData::IceCastXmlData(Ui::MainWindow *ui)
{
    this->ui = ui;
    ui->icecastTable->verticalHeader()->setDefaultSectionSize(18);
    ui->icecastTable->setColumnCount(5);
    ui->icecastTable->setHorizontalHeaderLabels({"Station", "Genre", "Codec", "Bitrate", "Sample"});
    QObject::connect(ui->icecastTable,
                     &QTableWidget::doubleClicked,
                     this,
                     &IceCastXmlData::onDoubleListClicked);
    ui->iceCastprogressBar->setFormat("Download. " + QString::number(0) + " bytes");
}

void IceCastXmlData::loadXmlData()
{
    iceCastTableRows.clear();
    if (!jsonListProcesor->isConnected)
        return;
    qDebug() << "Hello Ice-Cast";
    QUrl url(iceCastUrl);
    QNetworkAccessManager manager;
    QNetworkRequest request(url);
    QNetworkReply *reply = manager.get(request);

    QEventLoop loop;
    ui->iceCastprogressBar->show();
    QObject::connect(reply,
                     &QNetworkReply::downloadProgress,
                     [=](qint64 bytesReceived, qint64 bytesTotal) {
                         QMetaObject::invokeMethod(this,
                                                   "updateProgressBar",
                                                   Qt::QueuedConnection,
                                                   Q_ARG(int, bytesReceived));
                     });

    QObject::connect(reply, &QNetworkReply::finished, [&loop, this]() {
        qDebug() << "Download finished";
        ui->iceCastprogressBar->hide();
        loop.quit();
    });

    QObject::connect(reply, &QObject::destroyed, [&loop, this]() {
        qDebug() << "Download destroyed";
        ui->iceCastprogressBar->hide();
        loop.quit();
    });

    loop.exec();

    if (reply->error()) {
        reply->deleteLater();
        qDebug() << "Error";
        return;
    }

    QXmlStreamReader xml(reply->readAll());

    while (!xml.atEnd()) {
        IceCastTableRow icast;
        xml.readNext();
        if (xml.name().toString() == "entry") {
            while (!(xml.isEndElement() && xml.name().toString() == "entry")) {
                xml.readNext();

                if (xml.isStartElement()) {
                    QString elementName = xml.name().toString();

                    if (elementName == "server_name") {
                        icast.station = xml.readElementText();
                    } else if (elementName == "genre") {
                        icast.genre = xml.readElementText();
                    } else if (elementName == "server_type") {
                        icast.codec = xml.readElementText();
                    } else if (elementName == "bitrate") {
                        icast.bitrate = xml.readElementText();
                    } else if (elementName == "samplerate") {
                        icast.sample = xml.readElementText();
                    } else if (elementName == "listen_url") {
                        icast.listen_url = xml.readElementText();
                    }
                }
            }
            iceCastTableRows.push_back(icast);
        }
    }

    reply->deleteLater();
}

void IceCastXmlData::loadXmlToTable()
{
    ui->icecastTable->clearContents();
    ui->icecastTable->setRowCount(0);
    for (const auto &row : iceCastTableRows) {
        addRowToTable(row);
    }

    if (ui->iceCastprogressBar->isVisible())
        ui->iceCastprogressBar->hide();
    setIsStationsLoaded(true);
}

void IceCastXmlData::addRowToTable(const IceCastTableRow &row)
{
    int rowPosition = ui->icecastTable->rowCount();
    ui->icecastTable->insertRow(rowPosition);

    ui->icecastTable->setItem(rowPosition, 0, new QTableWidgetItem(row.station));
    ui->icecastTable->setItem(rowPosition, 1, new QTableWidgetItem(row.genre));
    ui->icecastTable->setItem(rowPosition, 2, new QTableWidgetItem(row.codec));
    ui->icecastTable->setItem(rowPosition, 3, new QTableWidgetItem(row.bitrate));
    ui->icecastTable->setItem(rowPosition, 4, new QTableWidgetItem(row.sample));
}

void IceCastXmlData::loadXmlAsync()
{
    QFutureWatcher<void> *watcher = new QFutureWatcher<void>(this);

    connect(watcher, &QFutureWatcher<void>::finished, [=]() {
        watcher->deleteLater();
        loadXmlToTable();
    });

    QFuture<void> future = QtConcurrent::run([this]() { loadXmlData(); });

    watcher->setFuture(future);
}

void IceCastXmlData::setJsonListProcessor(JsonListProcessor &jsonListProcessor)
{
    this->jsonListProcesor = &jsonListProcessor;
}

void IceCastXmlData::setRadioAudioManager(RadioAudioManager &radioAudioManager)
{
    this->radioAudioManager = &radioAudioManager;
}

void IceCastXmlData::setRadioList(RadioList *radioList)
{
    this->radioList = radioList;
}

void IceCastXmlData::setRadioInfo(RadioInfo *radioInfo)
{
    this->radioInfo = radioInfo;
}

void IceCastXmlData::onDoubleListClicked(const QModelIndex &index)
{
    if (jsonListProcesor->isConnected) {
        clearTableViewColor();
        QString url = iceCastTableRows[index.row()].listen_url;
        radioAudioManager->loadStream(url);
        radioAudioManager->playStream();
        setIndexColor(index);
        radioList->clearTableViewColor();
        radioList->clearIconLabelColor();
        ui->infoData->clear();
        radioInfo->clearInfo();
        ui->infoLabel->setPixmap(QPixmap(":/images/img/radio-10-96.png"));
        ui->radioIcon->setPixmap(QPixmap(":/images/img/radio-10-96.png"));
        ui->playPause->setIcon(QIcon(radioAudioManager->getMediaPlayer()->isPlaying()
                                         ? ":/images/img/pause30.png"
                                         : ":/images/img/play30.png"));
        radioList->getSongTitle(url);
        isPlaying = true;
    }
}

void IceCastXmlData::updateProgressBar(int progress)
{
    ui->iceCastprogressBar->setMaximum(100);
    ui->iceCastprogressBar->setMinimum(0);
    ui->iceCastprogressBar->setValue(progress % 100);
    ui->iceCastprogressBar->setFormat("Download. " + QString::number(progress) + " bytes");
    ui->iceCastprogressBar->setAlignment(Qt::AlignCenter);
}

bool IceCastXmlData::getIsStationsLoaded() const
{
    return isStationsLoaded;
}

void IceCastXmlData::setIsStationsLoaded(bool newIsStationsLoaded)
{
    isStationsLoaded = newIsStationsLoaded;
}

void IceCastXmlData::setIndexColor(const QModelIndex &index)
{
    for (int column = 0; column < ui->icecastTable->columnCount(); column++) {
        QTableWidgetItem *item = ui->icecastTable->item(index.row(), column);
        if (item)
            item->setBackground(QColor(222, 255, 223));
    }
    ui->icecastTable->setAlternatingRowColors(true);
}

void IceCastXmlData::clearTableViewColor()
{
    int rowCount = ui->icecastTable->rowCount();
    int columnCount = ui->icecastTable->columnCount();

    for (int row = 0; row < rowCount; ++row) {
        for (int column = 0; column < columnCount; ++column) {
            QTableWidgetItem *item = ui->icecastTable->item(row, column);
            if (item) {
                if (row % 2 == 0) {
                    item->setBackground(QColor(Qt::white));
                } else {
                    item->setBackground(QColor(245, 245, 245));
                }
            }
        }
    }
}

bool IceCastXmlData::getPlaying()
{
    return isPlaying;
}

void IceCastXmlData::playStreamOnStart(const QModelIndex &index)
{
    onDoubleListClicked(index);
}

IceCastXmlData::~IceCastXmlData()
{
    if (radioAudioManager)
        delete radioAudioManager;
    if (jsonListProcesor)
        delete jsonListProcesor;
}

void IceCastXmlData::setPlaying(bool b)
{
    isPlaying = b;
}
