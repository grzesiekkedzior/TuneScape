#include "radiolist.h"
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
    connect(ui->tableView, &QTableView::doubleClicked, this, &RadioList::onTableViewDoubleClicked);
    connect(ui->playPause, &QPushButton::clicked, this, &RadioList::onPlayPauseButtonCliced);
    connect(ui->stop, &QPushButton::clicked, this, &RadioList::onStopButtonClicked);

    header = ui->tableView->horizontalHeader();
    headers << STATION << COUNTRY << GENRE << HOMEPAGE;
    header->setSectionResizeMode(QHeaderView::Interactive);
    model->setHorizontalHeaderLabels(headers);
}

void RadioList::loadRadioList()
{
    int rowCount = model->rowCount();
    qDebug() << "loadRadioList" << treeItem;
    if (rowCount > 0 && treeItem != "Discover") {
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

    ui->tableView->resizeRowsToContents();
    loadedStationsCount += batchSize;
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
}

auto checkItem = [] (const QString &item, const QString &target) {
    return item == target;
};

void RadioList::onTreeViewItemClicked(const QModelIndex &index)
{
    QString item = index.data().toString();
    qDebug() << "onTreeViewItemClicked " << item;
    if (!checkItem(item, LIBRARY_TREE) && !checkItem(item, FAVORITE_TREE)) {
        if (checkItem(item, "Top")) {
            if(this->treeItem == "Discover") this->treeItem = "";
            jsonListProcesor.loadEndpoint(JSON_ENDPOINT_TOP);
        }if (checkItem(item, "Discover")) {
            this->treeItem = item;
            jsonListProcesor.loadEndpoint(JSON_ENDPOINT_DISCOVER);
        }if (checkItem(item, "Popular")) {
            if(this->treeItem == "Discover") this->treeItem = "";
            jsonListProcesor.loadEndpoint(JSON_ENDPOINT_POPULAR);
        } if (checkItem(item, "New")) {
            if(this->treeItem == "Discover") this->treeItem = "";
            jsonListProcesor.loadEndpoint(JSON_ENDPOINT_NEW);
        }
        if (jsonListProcesor.checkInternetConnection()) {
            loadedStationsCount = 0;
            jsonListProcesor.processJsonQuery();
            loadRadioList();
        }
    }
}

void RadioList::playStream(int radioNumber)
{
    currentRadioPlayingAddress = jsonListProcesor.getStreamAddresses(radioNumber);
    QUrl streamUrl(currentRadioPlayingAddress);
    radioManager.loadStream(streamUrl);
    radioManager.playStream();
}

void RadioList::onTableViewDoubleClicked(const QModelIndex &index)
{
    radioIndexNumber = index.row();
    playStream(radioIndexNumber);

    if (radioManager.getMediaPlayer()->isPlaying())
        ui->playPause->setIcon(QIcon(":/images/img/pause30.png"));
}

void RadioList::onPlayPauseButtonCliced()
{

    if (radioManager.getMediaPlayer()->isPlaying()) {
        ui->playPause->setIcon(QIcon(":/images/img/play30.png"));
        radioManager.stopStream();
    } else if (currentRadioPlayingAddress != ""
               && radioManager.getMediaPlayer()->isAvailable()) {
        ui->playPause->setIcon(QIcon(":/images/img/pause30.png"));
        radioManager.playStream();
    }

    if (currentRadioPlayingAddress.isEmpty() && !jsonListProcesor.getTableRows().isEmpty()) {
        playStream(radioIndexNumber);
        ui->playPause->setIcon(QIcon(":/images/img/pause30.png"));
    }
}

void RadioList::onStopButtonClicked()
{
    if (radioManager.getMediaPlayer()->isPlaying()) {
        ui->playPause->setIcon(QIcon(":/images/img/play30.png"));
        radioManager.stopStream();
        currentRadioPlayingAddress = "";
    }
}
