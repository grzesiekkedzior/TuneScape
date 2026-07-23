#include "include/radiolist.h"
#include <QFile>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonObject>
#include <QPainter>
#include <QScrollBar>
#include "include/RadioBrowserPlaylistEditor.h"

RadioList::RadioList(QObject *parent)
    : QObject{parent}
{}

RadioList::RadioList(Ui::MainWindow *ui, FavoriteManager *favoriteManager)
    : radioStationsModel(new RadioStationsModel{this})
    , ui(ui)
    , iconLoader(new IconLoader(this))
    , favoriteManager{favoriteManager}
{
    jsonListProcesor.setUi(ui);
    jsonListProcesor.setRadioList(this);
    radioInfo = new RadioInfo(ui);

    streamRecorder->setUI(ui);
    ui->playPause->setShortcut(QKeySequence(Qt::Key_Space));

    playlistEditor.reset(new RadioBrowserPlaylistEditor(*this, favoriteManager));

    connect(ui->treeView, &QTreeView::clicked, this, &RadioList::onTreeViewItemClicked);
    connect(ui->tableView, &QTableView::doubleClicked, this, &RadioList::setRadioImage);
    connect(ui->tableView, &QTableView::activated, this, &RadioList::setRadioImage);

    connect(ui->playPause, &QPushButton::clicked, this, &RadioList::onPlayPauseButtonCliced);
    connect(ui->next, &QPushButton::clicked, this, &RadioList::onNextButtonClicked);
    connect(ui->previous, &QPushButton::clicked, this, &RadioList::onPrevButtonClicked);
    connect(ui->stop, &QPushButton::clicked, this, &RadioList::onStopButtonClicked);
    connect(ui->tableView, &QTableView::clicked, this, &RadioList::onTableViewClicked);
    connect(ui->tableView, &QTableView::activated, this, &RadioList::tableViewActivated);

    connect(ui->horizontalVolumeSlider, &QSlider::sliderMoved, this, &RadioList::sliderMoved);
    connect(ui->horizontalVolumeSlider, &QSlider::valueChanged, this, &RadioList::sliderMoved);
    connect(miniPlayer.getMui()->dial, &QDial::sliderMoved, this, &RadioList::sliderMoved);
    connect(miniPlayer.getMui()->dial, &QDial::valueChanged, this, &RadioList::sliderMoved);

    connect(&streamReader, &StreamReader::dataReceived, this, &RadioList::handleDataReceived);
    //connect(ui->record, &QPushButton::clicked, this, &RadioList::setMp3FileName);
    connect(ui->record, &QPushButton::clicked, this, &RadioList::startStopRecord);
    connect(ui->serachInput, &QLineEdit::returnPressed, this, &RadioList::searchStations);
    connect(ui->favorite, &QPushButton::clicked, this, &RadioList::addRadioToFavorite);

    connect(ui->themeButton, &QPushButton::clicked, this, &RadioList::setDarkMode);
    connect(ui->minplr, &QPushButton::clicked, this, &RadioList::showMiniplayer);
    connect(miniPlayer.getMui()->maxWindow, &QPushButton::clicked, this, &RadioList::maximizeWindow);

    //trash header signal
    connect(ui->tableView, &QTableView::clicked, this, &RadioList::onTrashIconCliced);
    ui->tableView->setModel(radioStationsModel);
    header = ui->tableView->horizontalHeader();
    header->setSectionResizeMode(QHeaderView::Interactive);
    ui->tableView->verticalHeader()->setDefaultSectionSize(ui->tableView->fontMetrics().height()
                                                           + 2);
    audioProcessor.setUi(ui);
    audioProcessor.setPlayer(playbackController.mediaPlayer());
    miniPlayer.setUi(ui);
    miniPlayer.setRadioList(this);
    imageManager = new RadioImageManager{ui, &miniPlayer, this};
}

void RadioList::setRawDarkRadioImage()
{
    ui->infoLabel->setPixmap(QPixmap(RADIO_ICON));
    ui->radioIcon->setPixmap(QPixmap(RADIO_ICON));
    miniPlayer.getMui()->radioImage->setPixmap(QPixmap(RADIO_ICON));
}

void RadioList::showMiniplayer()
{
    miniPlayer.showMiniPlayer();
    int value = ui->horizontalVolumeSlider->value();
    miniPlayer.getMui()->dial->setValue(value);
    mainWindow->hide();
}

void RadioList::maximizeWindow()
{
    miniPlayer.window()->close();
    mainWindow->show();
    int value = miniPlayer.getMui()->dial->value();
    ui->horizontalVolumeSlider->setValue(value);
}

void RadioList::setRawRadioImage()
{
    ui->infoLabel->setPixmap(QPixmap(RADIO_ICON));
    ui->radioIcon->setPixmap(QPixmap(RADIO_ICON));
    miniPlayer.getMui()->radioImage->setPixmap(QPixmap(RADIO_ICON));
}

void RadioList::updateThemeAppearance(bool darkMode)
{
    clearTableViewColor();

    if (!isStopClicked)
        setIndexColor();

    if (!playbackController.isPlaying() || radioInfo->getInfoData().favicon.isEmpty()) {
        darkMode ? setRawDarkRadioImage() : setRawRadioImage();
    }
}

void RadioList::setDarkMode()
{
    isDarkMode = !isDarkMode;
    updateThemeAppearance(isDarkMode);
}

void RadioList::isDark() {}

void RadioList::setMp3FileName()
{
    QString title = "";
    QString extension = "";

    if (playbackController.isPlaying()) {
        title = ui->tableWidget->item(0, 1)->text();
        extension = ui->tableWidget->item(5, 1)->text().toLower();
    } else if (country.getIsPlaying()) {
        title = country.dtoFavorite.station;
        extension = ui->tableWidget->item(5, 1)->text().toLower();
    } else {
        return;
    }
    extension.replace("+", "");
    streamRecorder->setFileName(title, extension);
}

int RadioList::getCurrentStationIndex() const
{
    return currentStationIndex;
}

void RadioList::setCurrentStationIndex(int newCurrentStationIndex)
{
    currentStationIndex = newCurrentStationIndex;
}

const QVector<RadioStation> &RadioList::stations(Stations station) const
{
    return allStations[station];
}

void RadioList::setRadioIndexNumber(int newRadioIndexNumber)
{
    radioIndexNumber = newRadioIndexNumber;
}

Ui::MainWindow *RadioList::getUi() const
{
    return ui;
}

bool RadioList::getIsStopClicked() const
{
    return isStopClicked;
}

void RadioList::setIsStopClicked(bool newIsStopClicked)
{
    isStopClicked = newIsStopClicked;
}

RadioInfo *RadioList::getRadioInfo() const
{
    return radioInfo;
}

QMainWindow *RadioList::getMainWindow() const
{
    return mainWindow;
}

void RadioList::setMainWindow(QMainWindow *newMainWindow)
{
    mainWindow = newMainWindow;
}

void RadioList::setFavoriteLibrary()
{
    QModelIndex radioBrowserIndex = ui->treeView->model()->index(0, 0);
    QModelIndex favoriteIndex = ui->treeView->model()->index(3, 0, radioBrowserIndex);
    qDebug() << "index" << favoriteIndex.data().toString();
    if (favoriteIndex.isValid()) {
        onTreeViewItemClicked(favoriteIndex);
        ui->treeView->selectionModel()->select(favoriteIndex, QItemSelectionModel::Select);

    } else {
        qDebug() << "Error";
    }
}

bool RadioList::getIsSearchTablelDoubleCliced() const
{
    return isSearchTablelDoubleCliced;
}

void RadioList::setIsSearchTablelDoubleCliced(bool newIsSearchTablelDoubleCliced)
{
    isSearchTablelDoubleCliced = newIsSearchTablelDoubleCliced;
}

QString RadioList::getTreeItem() const
{
    return treeItem;
}

int RadioList::getRadioIndexNumber() const
{
    return radioIndexNumber;
}

QString RadioList::getItem() const
{
    return item;
}

void RadioList::setIsTreeClicked(bool newIsTreeClicked)
{
    isTreeClicked = newIsTreeClicked;
}

void RadioList::onTrayViewItemClicked(const QModelIndex &index)
{
    if (index.isValid()) {
        onTreeViewItemClicked(index);
    } else {
        qDebug() << "Wrong index.";
    }
}

void RadioList::onTrayClickedandPlay(const QModelIndex &index)
{
    onTableViewDoubleClicked(index);
}

void RadioList::setTrayRadioImage(const QModelIndex &index)
{
    setRadioImage(index);
}

JsonListProcessor *RadioList::getJsonListProcessor()
{
    return &jsonListProcesor;
}

bool RadioList::getIsBrowseStationLoaded() const
{
    return isBrowseStationLoaded;
}

void RadioList::setIsBrowseStationLoaded(bool newIsBrowseStationLoaded)
{
    isBrowseStationLoaded = newIsBrowseStationLoaded;
}

QSharedPointer<StreamRecorder> RadioList::getStreamRecorder() const
{
    return streamRecorder;
}

bool RadioList::getIsDarkMode() const
{
    return isDarkMode;
}

void RadioList::setIsDarkMode(bool newIsDarkMode)
{
    isDarkMode = newIsDarkMode;
}

void RadioList::updateFavoriteColumnLayout()
{
    header->setSectionResizeMode(QHeaderView::Interactive);

    if (item == FAVORITE)
        ui->tableView->setColumnWidth(1, 16);
}

void RadioList::onTrashIconCliced(const QModelIndex &index)
{
    qDebug() << "Trash" << index.column() << " tree " << item;
    if (item == FAVORITE && index.column() == 1) {
        if (playlistEditor) {
            bool success = playlistEditor->remove(index);
            if (!success) {
                qDebug() << "Error!!!";
            }
        }
    }
}

void RadioList::refreshFavoritePlaylist()
{
    setFavoriteStatons();

    if (item == FAVORITE) {
        radioStationsModel->setStations(allStations[Stations::FAVORITE]);
    }
}

void RadioList::setTopListOnStart()
{
    QModelIndex libraryIndex = ui->treeView->model()->index(0, 0);
    QModelIndex topIndex = ui->treeView->model()->index(0, 0, libraryIndex);
    onTreeViewItemClicked(topIndex);
    ui->treeView->selectionModel()->select(topIndex, QItemSelectionModel::Select);
}

void RadioList::setFavoriteStatons()
{
    QVector<RadioStation> favoriteStations;

    favoriteManager->readFavoriteStationsFromFile(favoriteStations);

    if (allStations.size() > Stations::FAVORITE)
        allStations[Stations::FAVORITE] = favoriteStations;
    else
        allStations.push_back(favoriteStations);
}

void RadioList::loadAllData()
{
    setVectorsOfStation(JSON_ENDPOINT_TOP, Stations::TOP);
    setVectorsOfStation(JSON_ENDPOINT_POPULAR, Stations::POPULAR);
    setVectorsOfStation(JSON_ENDPOINT_NEW, Stations::NEW);

    setFavoriteStatons();
    if (!isDarkMode) {
    }

    setTopListOnStart();
    //loadRadioIconList();
}

void RadioList::setLoadedStationsCount(int num)
{
    this->loadedStationsCount = num;
}

auto checkItem = [](const QString &item, const QString &target) { return item == target; };

//Main function
void RadioList::onTreeViewItemClicked(const QModelIndex &index)
{
    isTreeClicked = true;
    item = index.data().toString();
    qDebug() << "onTreeViewItemClicked " << item << " ROW " << index.row();

    if (checkItem(item, LIBRARY_TREE))
        return;

    if (checkItem(item, "Top"))
        switchToPlaylist(Stations::TOP);
    else if (checkItem(item, "Popular"))
        switchToPlaylist(Stations::POPULAR);
    else if (checkItem(item, "New"))
        switchToPlaylist(Stations::NEW);
    else if (checkItem(item, "Favorite"))
        switchToPlaylist(Stations::FAVORITE);
    if (jsonListProcesor.checkInternetConnection()) {
        loadedStationsCount = 0;
    }

    updateStationColoring();
}

void RadioList::resetTreeItemIfSearch()
{
    if (this->treeItem == "Search")
        this->treeItem = "";
}

void RadioList::switchToPlaylist(Stations station)
{
    resetTreeItemIfSearch();
    radioStationsModel->setStations(allStations[station]);
    currentPlaylistIndex = station;
}

void RadioList::updateStationColoring()
{
    if (currentPlayListPlaying == currentPlaylistIndex) {
        if (playbackController.isPlaying())
            setIndexColor();
    } else {
        // Clear the color-marked station when changing tree items.
        if (customColor)
            customColor->clearRowColor();
    }
}
//end

void RadioList::getSongTitle(const QString &url)
{
    streamReader.startStreaming(url);
}

void RadioList::checkIsRadioOnPlaylist(const QString &station)
{
    if (favoriteManager->isAddressExists(station, RADIO_BROWSER_PLAYLIST)) {
        ui->favorite->setIcon(QIcon(":/images/img/bookmark-file.png"));
    } else {
        ui->favorite->setIcon(QIcon(":/images/img/bookmark-empty.png"));
    }
}

void RadioList::onInternetConnectionRestored()
{
    clearRadioDataVectors();
    //this->onStopButtonClicked();
    prepareRestoredConnectionMessage();

    clearTableViewColor();

    ui->treeView->clearSelection();
    setIndexColor();
    loadAllData();
    if (getMainWindow()->isHidden())
        getMainWindow()->show();
    message.show();
}

void RadioList::prepareRestoredConnectionMessage()
{
    message.setText("The connection has been restored. Select a station.");
    message.setIcon(QMessageBox::Information);
    message.setWindowIcon(QIcon(":/images/img/radio30.png"));
}

void RadioList::clearRadioDataVectors()
{
    allStations.clear();
}

void RadioList::playSelectedStation(int radioNumber)
{
    radioIndexCurrentPlaying = radioNumber;
    radioPlaylistCurrentPlaying = currentPlaylistIndex;
    currentPlayingStation = radioStationsModel->station(radioNumber);
    currentRadioPlayingAddress = currentPlayingStation.streamUrl;
    QString stationUrl = radioStationsModel->station(radioNumber).streamUrl;
    checkIsRadioOnPlaylist(stationUrl);
    getSongTitle(currentRadioPlayingAddress);
    QUrl streamUrl(currentRadioPlayingAddress);
    playbackController.play(streamUrl);
    audioProcessor.start(currentRadioPlayingAddress);
}

void RadioList::setIndexColor()
{
    customColor.reset(new CustomColorDelegate(radioIndexNumber, QColor(222, 255, 223), this));
    ui->tableView->setItemDelegate(customColor.get());
}

void RadioList::sliderMoved(int move)
{
    playbackController.setVolume(move);
    if (move == 0)
        ui->volume->setIcon(QIcon(":/images/img/audiostop.png"));
    if (move > 0 && move < 5)
        ui->volume->setIcon(QIcon(":/images/img/audioplay.png"));
}

void RadioList::setRadioImage(const QModelIndex &index)
{
    if (!jsonListProcesor.isConnected || radioStationsModel->isEmpty())
        return;

    QUrl imageUrl(radioStationsModel->station(index.row()).iconUrl);
    QPixmap pixmap = imageManager->downloadImageSync(imageUrl);
    imageManager->setImageToUI(pixmap);

    qDebug() << "Image is loaded.";
}

void RadioList::onTableViewDoubleClicked(const QModelIndex &index)
{
    if (jsonListProcesor.checkInternetConnection()) {
        radioIndexNumber = index.row();
        currentStationIndex = index.row();
        currentPlayListPlaying = currentPlaylistIndex;

        // change bookmark
        playSelectedStation(radioIndexNumber);
        clearTableViewColor();
        country.clearTableColor();
        country.setIsPlaying(false);
        country.setCurrentIndexPlaying(-1);
        setIndexColor();

        setIsBrowseStationLoaded(true);
        if (playbackController.isPlaying()) {
            ui->playPause->setIcon(QIcon(":/images/img/pause30.png"));
            miniPlayer.getMui()->play->setIcon(QIcon(":/images/img/pause30.png"));
        }

        isStopClicked = false;
        ui->infoData->clear();
        if (playbackController.isPlaying()) {
            radioInfo->loadEndpoint(radioStationsModel->station(currentStationIndex).station);
            radioInfo->processInfoJsonQuery();
            radioInfo->setDataOnTable();
        }
        radioIndexNumber = index.row();
        setIndexColor();
    }

    if (streamRecorder->getIsRecording()) {
        streamRecorder->stopRecording();
        streamRecorder->setIsRecording(false);
    }

    if (this->treeItem == "Search")
        setIsSearchTablelDoubleCliced(true);
    else
        setIsSearchTablelDoubleCliced(false);
}

//Main function
void RadioList::onPlayPauseButtonCliced()
{
    if (!isTreeClicked || !jsonListProcesor.isConnected)
        return;

    if (country.getIsPlaying())
        country.setIsPlaying(false);

    if (playbackController.isPlaying()) {
        playbackController.pause();
        audioProcessor.getUpdateTimer()->stop();

    } else if (playbackController.isPaused()) {
        playbackController.resume();
        audioProcessor.getUpdateTimer()->start();

    } else if (country.getCurrentIndexPlaying() != -1) {
        playCountryStream();

    } else if (currentRadioPlayingAddress.isEmpty() && !radioStationsModel->isEmpty()) {
        startRadioBrowserStream();

    } else if (playbackController.isAvailable()) {
        playbackController.resume();
        audioProcessor.getUpdateTimer()->start();

    } else if (ui->tableView->currentIndex().isValid()) {
        playSelectedStation(radioIndexNumber);
    }

    if (currentPlayListPlaying == currentPlaylistIndex)
        setIndexColor();

    updatePlayPauseIcons();
    setIsBrowseStationLoaded(true);
    switchToDefaultTabIfNoCountryStationPlaying();
    resetImageIfStopped();
}

void RadioList::stopRadioBrowserStream()
{
    playbackController.stop();
    audioProcessor.getUpdateTimer()->stop();
}

void RadioList::returnRadioBrowserToPlay()
{
    playbackController.resume();
    audioProcessor.getUpdateTimer()->start();
}

void RadioList::playCountryStream()
{
    playbackController.resume();
    audioProcessor.getUpdateTimer()->start();
    country.setIsPlaying(true);
}

void RadioList::startRadioBrowserStream()
{
    currentPlayListPlaying = currentPlaylistIndex;
    playSelectedStation(radioIndexNumber);
    radioInfo->loadEndpoint(radioStationsModel->station(radioEnterIndexNumber).station);
    radioInfo->processInfoJsonQuery();
    radioInfo->setDataOnTable();
    QModelIndex newIndex = ui->tableView->model()->index(0, 0);
    setRadioImage(newIndex);
    ui->radioIcon->setPixmap(ui->infoLabel->pixmap());
    /***************************************************/
    miniPlayer.getMui()->radioImage->setPixmap(ui->infoLabel->pixmap());
    /***************************************************/
    country.setCurrentIndexPlaying(-1);
}

void RadioList::updatePlayPauseIcons() {
    QIcon icon(playbackController.isPlaying() ? ":/images/img/pause30.png"
                                              : ":/images/img/play30.png");
    ui->playPause->setIcon(icon);
    miniPlayer.getMui()->play->setIcon(icon);
}

void RadioList::switchToDefaultTabIfNoCountryStationPlaying()
{
    if (!country.getIsPlaying() && country.getCurrentIndexPlaying() == -1)
        ui->tabRadioListWidget->setCurrentIndex(0);
}

void RadioList::resetImageIfStopped()
{
    if (isStopClicked) {
        setRadioImage(radioStationsModel->index(0, 0));
        isStopClicked = false;
    }
}
//End

// Not use for now
void RadioList::onNextButtonClicked()
{
    if (playbackController.isPlaying() && radioIndexNumber < radioStationsModel->size() - 1) {
        ++radioIndexNumber;
        clearTableViewColor();
        setIndexColor();
        playSelectedStation(radioIndexNumber);
    }
}

// Not use for now
void RadioList::onPrevButtonClicked()
{
    if (playbackController.isPlaying() && radioIndexNumber > 0) {
        --radioIndexNumber;
        playSelectedStation(radioIndexNumber);

        clearTableViewColor();
        setIndexColor();
    }
}

void RadioList::clearTableViewColor()
{
    if (customColor) {
        customColor->clearRowColor();
        ui->tableView->update();
    }
}

void RadioList::onStopButtonClicked()
{
    // TODO when internet connection is lost then stop button is unable to turn off music!!!
    if ((playbackController.isPlaying() || playbackController.isPaused())
        && jsonListProcesor.isConnected) {
        isStopClicked = true;

        ui->playPause->setIcon(QIcon(":/images/img/play30.png"));
        miniPlayer.getMui()->play->setIcon(QIcon(":/images/img/play30.png"));

        playbackController.stop();
        audioProcessor.stop();

        currentRadioPlayingAddress.clear();

        ui->infoLabel->setPixmap(QPixmap(RADIO_ICON));
        ui->radioIcon->setPixmap(QPixmap(RADIO_ICON));
        miniPlayer.getMui()->radioImage->setPixmap(QPixmap(RADIO_ICON));

        ui->infoLabel->show();
        ui->infoData->clear();

        QModelIndex newIndex = ui->tableView->model()->index(0, 0);
        ui->tableView->setCurrentIndex(newIndex);

        radioIndexNumber = 0;

        clearTableViewColor();

        radioInfo->clearInfo();

        setIsBrowseStationLoaded(false);

        if (streamRecorder->getIsRecording()) {
            streamRecorder->stopRecording();
            streamRecorder->setIsRecording(false);
        }

        country.clearTableColor();
        country.setCurrentIndexPlaying(-1);
    }
}

// Not use now
void RadioList::onTableViewClicked(const QModelIndex &index)
{
    //this->radioIndexNumber = index.row();
    qDebug() << this->radioIndexNumber;
}

void RadioList::tableViewActivated(const QModelIndex &index)
{
    onTableViewDoubleClicked(index);
}

void RadioList::addRadioToFavorite()
{
    if (playbackController.isPlaying()) {
        handleRadioBrowserFavorite();
    } else if (country.getIsPlaying()) {
        handleCountryFavorite();
    }
}

void RadioList::handleRadioBrowserFavorite()
{
    if (!playbackController.isPlaying())
        return;

    QString data = currentPlayingStation.iconUrl + "," + currentPlayingStation.streamUrl + ","
                   + currentPlayingStation.station + "," + currentPlayingStation.country + ","
                   + currentPlayingStation.genre + "," + currentPlayingStation.homepage;

    bool isFavorite = favoriteManager->toggleFavorite(currentPlayingStation.streamUrl,
                                                      data,
                                                      RADIO_BROWSER_PLAYLIST);

    updateFavoriteIcon(isFavorite);
    refreshFavoritePlaylist();
}

void RadioList::updateFavoriteIcon(bool isFavorite)
{
    if (isFavorite)
        ui->favorite->setIcon(QIcon(":/images/img/bookmark-file.png"));
    else
        ui->favorite->setIcon(QIcon(":/images/img/bookmark-empty.png"));
}

void RadioList::handleCountryFavorite()
{
    if (!playbackController.isPlaying())
        return;

    QString data = country.dtoFavorite.icon + "," + country.dtoFavorite.stream + ","
                   + country.dtoFavorite.station + "," + country.dtoFavorite.country + ","
                   + country.dtoFavorite.genre + "," + country.dtoFavorite.stationUrl;

    QString stationStream = country.dtoFavorite.stream;

    bool isFavorite = favoriteManager->toggleFavorite(stationStream, data, RADIO_BROWSER_PLAYLIST);
    updateFavoriteIcon(isFavorite);
    refreshFavoritePlaylist();
}

void RadioList::handleDataReceived(const QString &data)
{
    QString metaData = data;
    int titleStart = metaData.indexOf("StreamTitle='");
    int titleEnd = metaData.indexOf("';", titleStart);

    if (titleStart != -1 && titleEnd != -1) {
        QString title = metaData.mid(titleStart + 13, titleEnd - (titleStart + 13));
        qDebug() << title;
        ui->infoData->clear();
        ui->infoData->setText(title);
        miniPlayer.getMui()->radioText->clear();
        miniPlayer.getMui()->radioText->setText(title);
        emit trackTitleReceived(title);
        emit sendTitleToTray(title);
    }
    //metaData = "";
}

void RadioList::startStopRecord()
{
    if (!(playbackController.isPlaying() && jsonListProcesor.isConnected))
        return;

    if (!streamRecorder->getIsRecording()) {
        setMp3FileName();
        streamRecorder->loadCurrentAddress(getCurrentStreamUrl());
        streamRecorder->startRecording();
    } else {
        qDebug() << "record false";
        streamRecorder->stopRecording();
    }
}

QString RadioList::getCurrentStreamUrl() const
{
    if (playbackController.isPlaying())
        return currentRadioPlayingAddress;
    if (country.getIsPlaying())
        return country.dtoFavorite.stream;
}

void RadioList::setVectorsOfStation(const QString &endpoint, Stations station)
{
    jsonListProcesor.loadEndpoint(endpoint);
    jsonListProcesor.processJsonQuery();

    // Wait for processing data
    QCoreApplication::processEvents();

    //new model
    if (allStations.size() > station)
        allStations[station] = jsonListProcesor.getStations();
    else
        allStations.push_back(jsonListProcesor.getStations());
}

void RadioList::searchStations()
{
    const QString endpoint = JSON_ENDPOINT_SEARCH + ui->serachInput->text();
    setVectorsOfStation(endpoint, Stations::SEARCH);

    radioStationsModel->setStations(allStations[Stations::SEARCH]);
    currentPlaylistIndex = Stations::SEARCH;

    if (jsonListProcesor.checkInternetConnection()) {
        loadedStationsCount = 0;
    }

    treeItem = "Search";
    item = "Search";
    ui->tabRadioListWidget->setCurrentIndex(0);

    clearTableViewColor();
}
