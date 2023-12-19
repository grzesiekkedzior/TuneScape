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
}

void IceCastXmlData::loadXmlData()
{
    QUrl url(iceCastUrl);
    QNetworkAccessManager manager;
    QNetworkRequest request(url);
    QNetworkReply *reply = manager.get(request);

    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
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
    for (const auto &row : iceCastTableRows) {
        addRowToTable(row);
    }
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
        isPlaying = true;
    }
}

void IceCastXmlData::setIndexColor(const QModelIndex &index)
{
    for (int column = 0; column < ui->icecastTable->columnCount(); column++) {
        QTableWidgetItem *item = ui->icecastTable->item(index.row(), column);
        if (item)
            item->setBackground(QColor(222, 255, 223));
    }
}

void IceCastXmlData::clearTableViewColor()
{
    int rowCount = ui->icecastTable->rowCount();
    int columnCount = ui->icecastTable->columnCount();

    for (int row = 0; row < rowCount; ++row) {
        for (int column = 0; column < columnCount; ++column) {
            QTableWidgetItem *item = ui->icecastTable->item(row, column);
            if (item) {
                item->setBackground(QColor(Qt::white));
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
