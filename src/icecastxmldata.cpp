#include "include/icecastxmldata.h"
#include <QFuture>
#include <QFutureWatcher>
#include <QMessageBox>
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
    discoveryStations.clear();
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
        QMetaObject::invokeMethod(this,
                                  "showErrorMessageBox",
                                  Qt::QueuedConnection,
                                  Q_ARG(QString, "Network error: " + reply->errorString()));
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
            discoveryStations.push_back(icast);
        }
    }
    iceCastStationTableRows = discoveryStations;
    reply->deleteLater();
}

void IceCastXmlData::addToFavoriteStations()
{
    favoriteStations.push_back(getIceCastTableRow(getCurrentPlayingStation()));
}

void IceCastXmlData::loadFavoriteIceCastStations()
{
    ui->icecastTable->clearContents();
    ui->icecastTable->setRowCount(0);
    for (const auto &row : favoriteStations) {
        addRowToTable(row);
    }

    if (ui->iceCastprogressBar->isVisible())
        ui->iceCastprogressBar->hide();
    if (getIsFavoritePlaying() && !radioList->getIsPlaying())
        setIndexColor(this->indexPlayingStation);
    setIsStationsLoaded(true);
}

void IceCastXmlData::loadDiscoveryStations()
{
    ui->icecastTable->clearContents();
    ui->icecastTable->setRowCount(0);
    for (const auto &row : discoveryStations) {
        addRowToTable(row);
    }

    if (ui->iceCastprogressBar->isVisible())
        ui->iceCastprogressBar->hide();
    // clear color
    if (!getIsFavoritePlaying() && !radioList->getIsPlaying())
        setIndexColor(this->indexPlayingStation);
    setIsStationsLoaded(true);
}

void IceCastXmlData::loadXmlToTable()
{
    ui->icecastTable->clearContents();
    ui->icecastTable->setRowCount(0);
    for (const auto &row : iceCastStationTableRows) {
        addRowToTable(row);
    }

    if (ui->iceCastprogressBar->isVisible())
        ui->iceCastprogressBar->hide();
    setIsStationsLoaded(true);
}
void IceCastXmlData::setFavoriteStations()
{
    favoriteStations.clear();
    QFile file("icecast.txt");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = in.readLine();
            // divide line and add to vectors

            auto it = std::find_if(iceCastStationTableRows.begin(),
                                   iceCastStationTableRows.end(),
                                   [line](const IceCastTableRow &row) {
                                       return row.station == line;
                                   });

            if (it != iceCastStationTableRows.end()) {
                favoriteStations.push_back(*it);
            }
        }
        file.close();
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
        setFavoriteStations();
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
        if (getIsFavoriteOnTreeCliced()) {
            setFavoriteList();
            setIsFavoritePlaying(true);
        } else {
            setIsFavoritePlaying(false);
            setDiscoveryList();
        }

        clearTableViewColor();
        QString url = iceCastStationTableRows[index.row()].listen_url;

        setCurrentPlayingStation(index.row());
        indexPlayingStation = index;
        radioAudioManager->loadStream(url);
        radioAudioManager->playStream();
        setIndexColor(index);
        radioList->clearTableViewColor();
        radioList->clearIconLabelColor();
        ui->infoData->clear();
        radioInfo->clearInfo();
        setIceCastInfo(index.row());
        if (radioList->getIsDarkMode()) {
            ui->infoLabel->setPixmap(QPixmap(":/images/img/radiodark-10-96.png"));
            ui->radioIcon->setPixmap(QPixmap(":/images/img/radiodark-10-96.png"));
        } else {
            ui->infoLabel->setPixmap(QPixmap(":/images/img/radio-10-96.png"));
            ui->radioIcon->setPixmap(QPixmap(":/images/img/radio-10-96.png"));
        }

        ui->playPause->setIcon(QIcon(radioAudioManager->getMediaPlayer()->isPlaying()
                                         ? ":/images/img/pause30.png"
                                         : ":/images/img/play30.png"));
        radioList->getSongTitle(url);
        checkIsRadioOnPlaylist();
        isPlaying = true;
        radioList->setIsPlaying(false);
        if (streamRecorder->getIsRecording()) {
            streamRecorder->stopRecording();
            streamRecorder->setIsRecording(false);
        }
    }
}

void IceCastXmlData::setIceCastInfo(int index)
{
    ui->tableWidget->setItem(0, 1, new QTableWidgetItem(iceCastStationTableRows[index].station));
    ui->tableWidget->setItem(5, 1, new QTableWidgetItem(iceCastStationTableRows[index].codec));
    ui->tableWidget->setItem(6, 1, new QTableWidgetItem(iceCastStationTableRows[index].bitrate));
}
void IceCastXmlData::updateProgressBar(int progress)
{
    ui->iceCastprogressBar->setMaximum(100);
    ui->iceCastprogressBar->setMinimum(0);
    ui->iceCastprogressBar->setValue(progress % 100);
    ui->iceCastprogressBar->setFormat("Download. " + QString::number(progress) + " bytes");
    ui->iceCastprogressBar->setAlignment(Qt::AlignCenter);
}

bool IceCastXmlData::getIsFavoriteOnTreeCliced() const
{
    return isFavoriteOnTreeCliced;
}

void IceCastXmlData::setIsFavoriteOnTreeCliced(bool newIsFavoriteOnTreeCliced)
{
    isFavoriteOnTreeCliced = newIsFavoriteOnTreeCliced;
}

void IceCastXmlData::setDiscoveryList()
{
    //iceCastStationTableRows.clear();
    this->iceCastStationTableRows = discoveryStations;
    this->loadXmlToTable();
}

void IceCastXmlData::setFavoriteList()
{
    //iceCastStationTableRows.clear();
    this->iceCastStationTableRows = favoriteStations;
    this->loadXmlToTable();
}

QVector<IceCastTableRow> IceCastXmlData::getIceCastStationTableRows() const
{
    return iceCastStationTableRows;
}

void IceCastXmlData::setIceCastStationTableRows(
    const QVector<IceCastTableRow> &newIceCastStationTableRows)
{
    iceCastStationTableRows = newIceCastStationTableRows;
}

IceCastTableRow IceCastXmlData::getIceCastTableRow(int index)
{
    return this->iceCastStationTableRows[index];
}

int IceCastXmlData::getCurrentPlayingStation() const
{
    return currentPlayingStation;
}

void IceCastXmlData::setCurrentPlayingStation(int newCurrentPlayingStation)
{
    currentPlayingStation = newCurrentPlayingStation;
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
    customColor.reset(new CustomColorDelegate(index.row(), QColor(222, 255, 223), this));
    ui->icecastTable->setItemDelegate(customColor.get());
}

void IceCastXmlData::checkIsRadioOnPlaylist()
{
    qDebug() << "Playing station:" << getIceCastTableRow(getCurrentPlayingStation()).station;
    if (radioList->isAddressExists(getIceCastTableRow(getCurrentPlayingStation()).station,
                                   "icecast.txt")) {
        ui->favorite->setIcon(QIcon(":/images/img/bookmark-file.png"));
    } else {
        ui->favorite->setIcon(QIcon(":/images/img/bookmark-empty.png"));
    }
}

void IceCastXmlData::showErrorMessageBox(const QString &errorMessage)
{
    QMessageBox::critical(qobject_cast<QWidget *>(this), "Ice-Cast server error!!!", errorMessage);
}

void IceCastXmlData::tableViewActivated(const QModelIndex &index)
{
    this->onDoubleListClicked(index);
}

QModelIndex IceCastXmlData::getIndexPlayingStation() const
{
    return indexPlayingStation;
}

void IceCastXmlData::setIndexPlayingStation(const QModelIndex &newIndexPlayingStation)
{
    indexPlayingStation = newIndexPlayingStation;
}

void IceCastXmlData::makeShareStreamRecorder(QSharedPointer<StreamRecorder> streamRecorder)
{
    this->streamRecorder = streamRecorder;
}

bool IceCastXmlData::getIsFavoritePlaying() const
{
    return isFavoritePlaying;
}

void IceCastXmlData::setIsFavoritePlaying(bool newIsFavoritePlaying)
{
    isFavoritePlaying = newIsFavoritePlaying;
}

void IceCastXmlData::clearTableViewColor()
{
    int rowCount = ui->icecastTable->rowCount();
    int columnCount = ui->icecastTable->columnCount();
    if (radioList->getIsDarkMode()) {
        for (int row = 0; row < rowCount; ++row) {
            for (int column = 0; column < columnCount; ++column) {
                QTableWidgetItem *item = ui->icecastTable->item(row, column);
                if (item) {
                    item->setBackground(QColor(60, 60, 60));
                    item->setForeground(QBrush(QColor(Qt::white)));
                }
            }
        }

    } else {
        for (int row = 0; row < rowCount; ++row) {
            for (int column = 0; column < columnCount; ++column) {
                QTableWidgetItem *item = ui->icecastTable->item(row, column);
                item->setBackground(QColor(Qt::white));
                //item->setForeground(QBrush(QColor(Qt::black)));
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
