#include "include/radiolist.h"
#include <QHeaderView>
#include <QScrollBar>

RadioList::RadioList(QObject *parent)
    : QObject{parent}
{}

RadioList::RadioList(Ui::MainWindow *ui) : ui(ui), model(new QStandardItemModel(this))
{
    jsonListProcesor.setUi(ui);
    jsonListProcesor.setRadioList(this);
    connect(ui->treeView, &QTreeView::clicked, this, &RadioList::onTreeViewItemClicked);
    connect(ui->tableView->verticalScrollBar(), &QScrollBar::valueChanged, this, &RadioList::loadMoreStationsIfNeeded);
    //connect(ui->tableView, &QTableView::doubleClicked, this, &RadioList::onTableViewDoubleClicked);
    connect(ui->tableView, &QTableView::doubleClicked, this, &RadioList::setRadioImage);
    connect(ui->tableView, &QTableView::activated, this, &RadioList::setRadioImage);
    connect(ui->playPause, &QPushButton::clicked, this, &RadioList::onPlayPauseButtonCliced);
    connect(ui->next, &QPushButton::clicked, this, &RadioList::onNextButtonClicked);
    connect(ui->previous, &QPushButton::clicked, this, &RadioList::onPrevButtonClicked);
    connect(ui->stop, &QPushButton::clicked, this, &RadioList::onStopButtonClicked);
    connect(ui->tableView, &QTableView::clicked, this, &RadioList::onTableViewClicked);
    connect(ui->tableView, &QTableView::activated, this, &RadioList::tableViewActivated);
    connect(ui->horizontalVolumeSlider, &QSlider::sliderMoved, this, &RadioList::sliderMoved);
    connect(&streamReader, &StreamReader::dataReceived, this, &RadioList::handleDataReceived);
    connect(ui->serachInput, &QLineEdit::returnPressed, this, &RadioList::searchStations);



    header = ui->tableView->horizontalHeader();
    headers << STATION << COUNTRY << GENRE << HOMEPAGE;
    header->setSectionResizeMode(QHeaderView::Interactive);
    model->setHorizontalHeaderLabels(headers);
}

void RadioList::loadRadioList()
{
    int rowCount = model->rowCount();
    qDebug() << "loadRadioList" << treeItem;
    if (rowCount > 0 && treeItem != "Search") {
        model->removeRows(0, rowCount);
    }
    int dataSize = jsonListProcesor.getTableRows().size();
    int batchSize = 50;

    for (int row = loadedStationsCount; row < qMin(loadedStationsCount + batchSize, dataSize); ++row) {
        QList<QStandardItem*> rowItems;
        rowItems.append(new QStandardItem(jsonListProcesor.getTableRows().at(row).station));
        rowItems.append(new QStandardItem(jsonListProcesor.getTableRows().at(row).country));
        rowItems.append(new QStandardItem(jsonListProcesor.getTableRows().at(row).genre));
        rowItems.append(new QStandardItem(jsonListProcesor.getTableRows().at(row).stationUrl));
        model->appendRow(rowItems);
    }
    ui->tableView->setModel(model);
    this->treeItem = "Search";
    ui->tableView->resizeRowsToContents();
    loadedStationsCount += batchSize;
}

void RadioList::setTopListOnStart()
{
    QModelIndex libraryIndex = ui->treeView->model()->index(0, 0);
    QModelIndex topIndex = ui->treeView->model()->index(0, 0, libraryIndex);
    onTreeViewItemClicked(topIndex);
    ui->treeView->selectionModel()->select(topIndex, QItemSelectionModel::Select);
}

void RadioList::loadAllData()
{
    QStringList endpoints = {
        JSON_ENDPOINT_TOP,
        //JSON_ENDPOINT_DISCOVER,
        JSON_ENDPOINT_POPULAR,
        JSON_ENDPOINT_NEW
    };

    for (const QString &endpoint : endpoints) {
        setVectorsOfStation(endpoint);
    }

    setTopListOnStart();
}

void RadioList::setLoadedStationsCount(int num)
{
    this->loadedStationsCount = num;
}

void RadioList::loadMoreStationsIfNeeded()
{
    QScrollBar *scrollBar = ui->tableView->verticalScrollBar();
    int currentPosition = scrollBar->value();
    int maximumPosition = scrollBar->maximum();

    if (currentPosition >= maximumPosition * 0.8) {

        loadRadioList();
    }
    if (currentPlayListPlaying == currentPlaylistIndex) {
        setIndexColor();
    }
}

auto checkItem = [] (const QString &item, const QString &target) {
    return item == target;
};

void RadioList::onTreeViewItemClicked(const QModelIndex &index)
{
    isTreeClicked = true;
    item = index.data().toString();
    qDebug() << "onTreeViewItemClicked " << item;

    if (!checkItem(item, LIBRARY_TREE) && !checkItem(item, FAVORITE_TREE)) {
        if (checkItem(item, "Top")) {
            if (this->treeItem == "Search") this->treeItem = "";
            jsonListProcesor.setTableRows(allTableRows[Stations::TOP]);
            jsonListProcesor.setStreamAddresses(allStreamAddresses[Stations::TOP]);
            jsonListProcesor.setIconAddresses(allIconsAddresses[Stations::TOP]);
            currentPlaylistIndex = Stations::TOP;
        } /*else if (checkItem(item, "Discover")) {
            int rowCount = model->rowCount();
            qDebug() << "loadRadioList" << treeItem;
            if (rowCount > 0) {
                model->removeRows(0, rowCount);
            }
            this->treeItem = item;
            jsonListProcesor.setTableRows(allTableRows[Stations::DISCOVERY]);
            jsonListProcesor.setStreamAddresses(allStreamAddresses[Stations::DISCOVERY]);
            jsonListProcesor.setIconAddresses(allIconsAddresses[Stations::DISCOVERY]);
            currentPlaylistIndex = Stations::DISCOVERY;
        } */else if (checkItem(item, "Popular")) {
            if (this->treeItem == "Search") this->treeItem = "";
            jsonListProcesor.setTableRows(allTableRows[Stations::POPULAR]);
            jsonListProcesor.setStreamAddresses(allStreamAddresses[Stations::POPULAR]);
            jsonListProcesor.setIconAddresses(allIconsAddresses[Stations::POPULAR]);
            currentPlaylistIndex = Stations::POPULAR;
        } else if (checkItem(item, "New")) {
            if (this->treeItem == "Search") this->treeItem = "";
            jsonListProcesor.setTableRows(allTableRows[Stations::NEW]);
            jsonListProcesor.setStreamAddresses(allStreamAddresses[Stations::NEW]);
            jsonListProcesor.setIconAddresses(allIconsAddresses[Stations::NEW]);
            currentPlaylistIndex = Stations::NEW;
        }
        if (jsonListProcesor.checkInternetConnection()) {
            loadedStationsCount = 0;
            loadRadioList();
        }

        if (currentPlayListPlaying == currentPlaylistIndex) {
            setIndexColor();
        }
    }
}

void RadioList::getSongTitle(const QString &url)
{
    streamReader.startStreaming(url);
}

void RadioList::playStream(int radioNumber)
{
    currentRadioPlayingAddress = jsonListProcesor.getStreamAddresses(radioNumber);
    getSongTitle(currentRadioPlayingAddress);
    QUrl streamUrl(currentRadioPlayingAddress);
    radioManager.loadStream(streamUrl);
    radioManager.playStream();

}

void RadioList::setIndexColor()
{
    for (int column = 0; column < model->columnCount(); column++) {
        model->setData(model->index(radioIndexNumber, column), QColor(222, 255, 223), Qt::BackgroundRole);
    }
}

void RadioList::sliderMoved(int move)
{
    radioManager.setVolume(move);
    if (move == 0) {
        ui->volume->setIcon(QIcon(":/images/img/audiostop.png"));
    }
    if (move == 1){
        ui->volume->setIcon(QIcon(":/images/img/audioplay.png"));
    }
}

void RadioList::setRadioImage(const QModelIndex &index)
{
    QEventLoop loop;
    QUrl imageUrl(jsonListProcesor.getIconAddresses(index.row()));

    QNetworkAccessManager manager;

    QNetworkRequest request(imageUrl);
    QNetworkReply *reply = manager.get(request);

    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() == QNetworkReply::NoError) {

        QVariant contentType = reply->header(QNetworkRequest::ContentTypeHeader);
        QString contentTypeString = contentType.toString();

        if (contentTypeString.startsWith("image/")) {
            QByteArray imageData = reply->readAll();
            QPixmap pixmap;
            pixmap.loadFromData(imageData);

            if (!pixmap.isNull()) {
                QSize imageSize(120, 120);
                pixmap = pixmap.scaled(imageSize, Qt::KeepAspectRatio);
                ui->infoLabel->setPixmap(pixmap);
                ui->infoLabel->show();
            }
        } else {
            ui->infoLabel->setPixmap(QPixmap(":/images/img/radio-10-96.png"));
            ui->infoLabel->show();
        }
    } else {
        qDebug() << "Error:" << reply->errorString();
        ui->infoLabel->setPixmap(QPixmap(":/images/img/radio-10-96.png"));
        ui->infoLabel->show();
    }

    reply->deleteLater();
}


void RadioList::onTableViewDoubleClicked(const QModelIndex &index)
{
    qDebug() << "onTableViewDoubleClicked";
    radioIndexNumber = index.row();
    currentStationIndex = index.row();
    currentPlayListPlaying = currentPlaylistIndex;
    playStream(radioIndexNumber);
    clearTableViewColor();
    setIndexColor();

    if (radioManager.getMediaPlayer()->isPlaying())
        ui->playPause->setIcon(QIcon(":/images/img/pause30.png"));
    isStopClicked = false;
    ui->infoData->clear();
}

void RadioList::onPlayPauseButtonCliced()
{
    if (isTreeClicked) {
        if (radioManager.getMediaPlayer()->isPlaying()) {
            radioManager.stopStream();
        } else if (currentRadioPlayingAddress != ""
                   && radioManager.getMediaPlayer()->isAvailable()) {
            radioManager.playStream();
        } else if (currentRadioPlayingAddress.isEmpty()
                   && !jsonListProcesor.getTableRows().isEmpty()) {
            clearTableViewColor();
            setIndexColor();
            playStream(radioIndexNumber);
            QModelIndex newIndex = ui->tableView->model()->index(0, 0);
            setRadioImage(newIndex);
        } else if (!radioManager.getMediaPlayer()->isPlaying()
                   && currentRadioPlayingAddress == "") {
            clearTableViewColor();
            setIndexColor();
            playStream(radioIndexNumber);
        }

        if (isStopClicked) {
            setRadioImage(ui->tableView->currentIndex());
            isStopClicked = false;
        }

        ui->playPause->setIcon(QIcon(
            radioManager.getMediaPlayer()->isPlaying()
                ? ":/images/img/pause30.png" : ":/images/img/play30.png"));
    }

}


void RadioList::onNextButtonClicked()
{
    if (radioManager.getMediaPlayer()->isPlaying()
        && radioIndexNumber < jsonListProcesor.getTableRows().size()-1) {
        ++radioIndexNumber;
        clearTableViewColor();
        setIndexColor();
        playStream(radioIndexNumber);

        qDebug() << radioIndexNumber << jsonListProcesor.getTableRows().size();
    }
}

void RadioList::onPrevButtonClicked()
{
    if (radioManager.getMediaPlayer()->isPlaying()
        && radioIndexNumber > 0) {
        --radioIndexNumber;
        playStream(radioIndexNumber);

        clearTableViewColor();
        setIndexColor();

        qDebug() << radioIndexNumber << jsonListProcesor.getTableRows().size();
    }

}

void RadioList::clearTableViewColor()
{
    for (int row = 0; row < model->rowCount(); ++row) {
        for (int column = 0; column < model->columnCount(); ++column) {
            model->setData(model->index(row, column), QColor(Qt::white), Qt::BackgroundRole);
        }
    }
}

void RadioList::onStopButtonClicked()
{
    if (radioManager.getMediaPlayer()->isPlaying()) {
        isStopClicked = true;
        if (radioManager.getMediaPlayer()->isPlaying()) {
            ui->playPause->setIcon(QIcon(":/images/img/play30.png"));
            radioManager.stopStream();
            currentRadioPlayingAddress = "";
            radioIndexNumber = 0;
            ui->infoLabel->setPixmap(QPixmap(":/images/img/radio-10-96.png"));
            ui->infoLabel->show();
            ui->infoData->clear();
            QModelIndex newIndex = ui->tableView->model()->index(0, 0);
            ui->tableView->setCurrentIndex(newIndex);
        } else {
            ui->playPause->setIcon(QIcon(":/images/img/play30.png"));
            currentRadioPlayingAddress = "";
            radioIndexNumber = 0;
            QModelIndex newIndex = ui->tableView->model()->index(0, 0);
            ui->tableView->setCurrentIndex(newIndex);
        }
        clearTableViewColor();

    }
}

void RadioList::onTableViewClicked(const QModelIndex &index)
{
    //this->radioIndexNumber = index.row();
    qDebug() << this->radioIndexNumber;
}

void RadioList::tableViewActivated(const QModelIndex &index)
{
    clearTableViewColor();
    onTableViewDoubleClicked(index);
    this->radioIndexNumber = index.row();
}

void RadioList::handleDataReceived(const QString& data) {
    QString metaData = data;
    int titleStart = metaData.indexOf("StreamTitle='");
    int titleEnd = metaData.indexOf("';", titleStart);

    if (titleStart != -1 && titleEnd != -1) {
        QString title = metaData.mid(titleStart + 13, titleEnd - (titleStart + 13));
        qDebug() << title;
        ui->infoData->clear();
        ui->infoData->setText(title);
    }
    metaData = "";
}

void RadioList::setVectorsOfStation(const QString endpoint)
{
    jsonListProcesor.loadEndpoint(endpoint);
    jsonListProcesor.processJsonQuery();

    // Wait for processing data
    QCoreApplication::processEvents();

    QVector<TableRow> tableRows = jsonListProcesor.getTableRows();
    QVector<QString> streamAddresses = jsonListProcesor.getStreamAddresses();
    QVector<QString> iconAddresses = jsonListProcesor.getIconAddresses();
    allTableRows.push_back(tableRows);
    allStreamAddresses.push_back(streamAddresses);
    allIconsAddresses.push_back(iconAddresses);
}

void RadioList::searchStations()
{
    // This is ugly but I dont change it to dont complicate code
    //**********************************************************
    if (allTableRows.size() > 3
        && allIconsAddresses.size() > 3
        && allStreamAddresses.size() > 3) {
        allTableRows.pop_back();
        allIconsAddresses.pop_back();
        allStreamAddresses.pop_back();
    }
    //**********************************************************
    int rowCount = model->rowCount();
    model->removeRows(0, rowCount);

    QString data = ui->serachInput->text();
    const QString endpoint = JSON_ENDPOINT_SEARCH + data;
    qDebug() << allTableRows.size();

    setVectorsOfStation(endpoint);

    jsonListProcesor.setTableRows(allTableRows[Stations::SEARCH]);
    jsonListProcesor.setStreamAddresses(allStreamAddresses[Stations::SEARCH]);
    jsonListProcesor.setIconAddresses(allIconsAddresses[Stations::SEARCH]);
    currentPlaylistIndex = Stations::SEARCH;

    if (jsonListProcesor.checkInternetConnection()) {
        loadedStationsCount = 0;
        loadRadioList();
    }

    this->treeItem = "Search";
}
