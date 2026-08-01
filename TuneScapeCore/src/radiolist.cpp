#include "include/radiolist.h"
#include <QDesktopServices>
#include <QFile>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonObject>
#include <QPainter>
#include <QScrollBar>
#include <QUrl>
#include "include/RadioBrowserPlaylistEditor.h"
#include "include/StationDetailsDialog.h"

RadioList::RadioList(QObject *parent)
    : QObject{parent}
{}

RadioList::RadioList(Ui::MainWindow *ui, FavoriteManager *favoriteManager)
    : radioStationsModel(new RadioStationsModel{this})
    , ui(ui)
    , favoriteManager{favoriteManager}
    , iconLoader(this)
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
    connect(ui->minplr, &QPushButton::clicked, this, &RadioList::showMiniplayer);
    connect(miniPlayer.getMui()->maxWindow, &QPushButton::clicked, this, &RadioList::maximizeWindow);
    connect(&iconLoader,
            &IconLoader::iconLoaded,
            radioStationsModel,
            &RadioStationsModel::setStationIcon);

    //trash header signal
    ui->tableView->setModel(radioStationsModel);
    header = ui->tableView->horizontalHeader();
    header->setSectionResizeMode(QHeaderView::Interactive);
    ui->tableView->verticalHeader()->setDefaultSectionSize(ui->tableView->fontMetrics().height()
                                                           + 2);
    // audioProcessor.setUi(ui);
    // audioProcessor.setPlayer(playbackController.mediaPlayer());
    miniPlayer.setUi(ui);
    miniPlayer.setRadioList(this);
    imageManager = new RadioImageManager{this};
    playerUIController.initialize(ui, &miniPlayer);
    ui->tableView->setIconSize(QSize(20, 20));

    ui->tableView->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(ui->tableView,
            &QTableView::customContextMenuRequested,
            this,
            &RadioList::showStationContextMenu);

    ui->tableView->verticalHeader()->setDefaultSectionSize(30);
    header->setSectionResizeMode(static_cast<int>(Column::StationColumn), QHeaderView::Stretch);
    header->setSectionResizeMode(static_cast<int>(Column::GenreColumn),
                                 QHeaderView::ResizeToContents);
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

void RadioList::updateThemeAppearance()
{
    clearTableViewColor();

    if (!isStopClicked)
        setIndexColor(radioIndexNumber);

    if (!playbackController.isPlaying() || radioInfo->getInfoData().favicon.isEmpty()) {
        playerUIController.setDefaultImage();
    }
}

void RadioList::setMp3FileName()
{
    const QTableWidgetItem *titleItem = ui->tableWidget->item(0, 1);
    const QTableWidgetItem *extensionItem = ui->tableWidget->item(5, 1);

    if (!extensionItem)
        return;

    QString title;
    QString extension = extensionItem->text().toLower();

    if (playbackController.isPlaying() && !country.getIsPlaying()) {
        if (!titleItem)
            return;

        title = titleItem->text();
    } else if (country.getIsPlaying()) {
        title = country.getCurrentStation().station;
    } else {
        return;
    }

    extension.remove('+');
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

void RadioList::updateFavoriteColumnLayout()
{
    header->setSectionResizeMode(QHeaderView::Interactive);

    if (item == FAVORITE)
        ui->tableView->setColumnWidth(1, 16);
}

void RadioList::refreshFavoritePlaylist()
{
    setFavoriteStatons();

    if (item == FAVORITE) {
        radioStationsModel->setStations(allStations[Stations::FAVORITE]);
        loadStationIcons();
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
    allStations.resize(Stations::FAVORITE + 1);
    setFavoriteStatons();

    if (!jsonListProcesor.checkInternetConnection()) {
        radioStationsModel->setStations(allStations[Stations::FAVORITE]);

        currentPlaylistIndex = Stations::FAVORITE;
        item = FAVORITE;

        const QModelIndex libraryIndex = ui->treeView->model()->index(0, 0);

        const QModelIndex favoriteIndex = ui->treeView->model()->index(3, 0, libraryIndex);

        if (favoriteIndex.isValid()) {
            ui->treeView->selectionModel()->select(favoriteIndex,
                                                   QItemSelectionModel::ClearAndSelect);
        }

        QMessageBox::warning(mainWindow,
                             tr("No internet connection"),
                             tr("TuneScape could not connect to the internet.\n"
                                "Online station lists and playback are unavailable.\n"
                                "The application will retry when the connection is restored."));

        return;
    }

    setVectorsOfStation(JSON_ENDPOINT_TOP, Stations::TOP);
    setVectorsOfStation(JSON_ENDPOINT_POPULAR, Stations::POPULAR);
    setVectorsOfStation(JSON_ENDPOINT_NEW, Stations::NEW);

    setTopListOnStart();
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
    loadStationIcons();
    currentPlaylistIndex = station;
}

void RadioList::updateStationColoring()
{
    if (currentPlayListPlaying == currentPlaylistIndex) {
        if (playbackController.isPlaying())
            setIndexColor(radioIndexNumber);
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
    const bool isFavorite = favoriteManager->isAddressExists(station, RADIO_BROWSER_PLAYLIST);

    playerUIController.setFavorite(isFavorite);
}

void RadioList::onInternetConnectionRestored()
{
    clearRadioDataVectors();
    prepareRestoredConnectionMessage();

    clearTableViewColor();
    ui->treeView->clearSelection();

    loadAllData();

    emit internetConnectionRestored();

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
    currentPlayListPlaying = currentPlaylistIndex;

    currentPlayingStation = radioStationsModel->station(radioNumber);
    currentRadioPlayingAddress = currentPlayingStation.streamUrl;

    checkIsRadioOnPlaylist(currentRadioPlayingAddress);
    getSongTitle(currentRadioPlayingAddress);

    playbackController.play(QUrl(currentRadioPlayingAddress));
    //audioProcessor.start(currentRadioPlayingAddress);
}

void RadioList::setTheme(Theme *newTheme)
{
    theme = newTheme;
}

void RadioList::sliderMoved(int move)
{
    playbackController.setVolume(move);
    playerUIController.setVolumeIcon(move);
}

void RadioList::setRadioImage(const QModelIndex &index)
{
    if (!index.isValid())
        return;

    if (!jsonListProcesor.isConnected || radioStationsModel->isEmpty())
        return;

    if (index.row() < 0 || index.row() >= radioStationsModel->size())
        return;

    const QUrl imageUrl(radioStationsModel->station(index.row()).iconUrl);
    const QPixmap downloadedPixmap = imageManager->downloadImageSync(imageUrl);
    const QPixmap preparedPixmap = imageManager->prepareImage(downloadedPixmap);

    playerUIController.setStationImage(preparedPixmap);

    qDebug() << "Image is loaded.";
}

void RadioList::onTableViewDoubleClicked(const QModelIndex &index)
{
    if (!index.isValid())
        return;

    if (index.row() < 0 || index.row() >= radioStationsModel->size())
        return;

    if (jsonListProcesor.checkInternetConnection()) {
        radioIndexNumber = index.row();
        currentStationIndex = index.row();

        // change bookmark
        playSelectedStation(radioIndexNumber);
        clearTableViewColor();
        country.clearTableColor();
        country.setIsPlaying(false);
        country.setCurrentIndexPlaying(-1);
        setIndexColor(radioIndexNumber);

        setIsBrowseStationLoaded(true);
        if (playbackController.isPlaying()) {
            playerUIController.setPauseIcon();
        }

        isStopClicked = false;
        playerUIController.clearMetadata();
        if (playbackController.isPlaying()) {
            radioInfo->loadEndpoint(radioStationsModel->station(currentStationIndex).station);
            radioInfo->processInfoJsonQuery();
            radioInfo->setDataOnTable();
        }
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

    if (playbackController.isPlaying()) {
        playbackController.pause();
        //audioProcessor.getUpdateTimer()->stop();

    } else if (playbackController.isPaused()) {
        playbackController.resume();
        //audioProcessor.getUpdateTimer()->start();

    } else if (country.getCurrentIndexPlaying() != -1) {
        playCountryStream();

    } else if (currentRadioPlayingAddress.isEmpty() && !radioStationsModel->isEmpty()) {
        startRadioBrowserStream();

    } else if (playbackController.isAvailable()) {
        playbackController.resume();
        //audioProcessor.getUpdateTimer()->start();

    } else if (ui->tableView->currentIndex().isValid()) {
        playSelectedStation(radioIndexNumber);
    }

    if (currentPlayListPlaying == currentPlaylistIndex)
        setIndexColor(radioIndexNumber);

    updatePlayPauseIcons();
    setIsBrowseStationLoaded(true);
    switchToDefaultTabIfNoCountryStationPlaying();
    resetImageIfStopped();
}

void RadioList::stopRadioBrowserStream()
{
    playbackController.stop();
    //audioProcessor.getUpdateTimer()->stop();
}

void RadioList::returnRadioBrowserToPlay()
{
    playbackController.resume();
    //audioProcessor.getUpdateTimer()->start();
}

void RadioList::playCountryStream()
{
    playbackController.resume();
    //audioProcessor.getUpdateTimer()->start();
    country.setIsPlaying(true);
}

void RadioList::startRadioBrowserStream()
{
    playSelectedStation(radioIndexNumber);

    radioInfo->loadEndpoint(currentPlayingStation.station);
    radioInfo->processInfoJsonQuery();
    radioInfo->setDataOnTable();

    const QModelIndex newIndex = ui->tableView->model()->index(radioIndexNumber, 0);
    setRadioImage(newIndex);

    country.setCurrentIndexPlaying(-1);
    country.setIsPlaying(false);
}

void RadioList::updatePlayPauseIcons() {
    if (playbackController.isPlaying())
        playerUIController.setPauseIcon();
    else
        playerUIController.setPlayIcon();
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
        setIndexColor(radioIndexNumber);
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
        setIndexColor(radioIndexNumber);
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
    if (!playbackController.isPlaying() && !playbackController.isPaused())
        return;

    isStopClicked = true;

    playerUIController.setPlayIcon();

    playbackController.stop();
    //audioProcessor.stop();

    currentRadioPlayingAddress.clear();

    playerUIController.setDefaultImage();
    playerUIController.clearMetadata();

    const QModelIndex newIndex = ui->tableView->model()->index(0, 0);
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
    country.setIsPlaying(false);
}

// Not use now
void RadioList::onTableViewClicked(const QModelIndex &index)
{
    qDebug() << this->radioIndexNumber;
}

void RadioList::tableViewActivated(const QModelIndex &index)
{
    onTableViewDoubleClicked(index);
}

void RadioList::addRadioToFavorite()
{
    if (country.getIsPlaying()) {
        handleCountryFavorite();
    } else if (playbackController.isPlaying()) {
        handleRadioBrowserFavorite();
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

    playerUIController.setFavorite(isFavorite);
    refreshFavoritePlaylist();
}

void RadioList::handleCountryFavorite()
{
    if (!country.getIsPlaying())
        return;

    const RadioStation &station = country.getCurrentStation();

    const QString data = station.iconUrl + "," + station.streamUrl + "," + station.station + ","
                         + station.country + "," + station.genre + "," + station.homepage;

    const bool isFavorite = favoriteManager->toggleFavorite(station.streamUrl,
                                                            data,
                                                            RADIO_BROWSER_PLAYLIST);

    playerUIController.setFavorite(isFavorite);
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
        playerUIController.setMetadata(title);
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
    if (country.getIsPlaying())
        return country.getCurrentStation().streamUrl;

    if (playbackController.isPlaying())
        return currentRadioPlayingAddress;

    return {};
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
    loadStationIcons();
    currentPlaylistIndex = Stations::SEARCH;

    if (jsonListProcesor.checkInternetConnection()) {
        loadedStationsCount = 0;
    }

    treeItem = "Search";
    item = "Search";
    ui->tabRadioListWidget->setCurrentIndex(0);

    clearTableViewColor();
}

void RadioList::setIndexColor(int row)
{
    if (row < 0 || row >= radioStationsModel->size())
        return;

    if (!theme)
        return;

    customColor.reset(new CustomColorDelegate(row, theme->playingRowColor(), this));

    ui->tableView->setItemDelegate(customColor.get());
    ui->tableView->viewport()->update();
}

void RadioList::loadStationIcons()
{
    for (int row = 0; row < radioStationsModel->rowCount(QModelIndex()); ++row) {
        const RadioStation &station = radioStationsModel->station(row);

        if (station.iconUrl.isEmpty())
            continue;

        iconLoader.loadIcon(row, QUrl(station.iconUrl), QSize(128, 128));
    }
}

void RadioList::showStationContextMenu(const QPoint &position)
{
    if (!radioStationsModel || !favoriteManager)
        return;

    const QModelIndex index = ui->tableView->indexAt(position);

    if (!index.isValid())
        return;

    if (index.row() < 0 || index.row() >= radioStationsModel->size())
        return;

    const RadioStation station = radioStationsModel->station(index.row());

    QMenu menu(ui->tableView);

    const bool isCurrentStationPlaying = playbackController.isPlaying() && !country.getIsPlaying()
                                         && station.streamUrl == currentPlayingStation.streamUrl;

    QAction *playAction = menu.addAction(isCurrentStationPlaying ? tr("Stop") : tr("Play"));

    const bool isFavorite = favoriteManager->isAddressExists(station.streamUrl,
                                                             RADIO_BROWSER_PLAYLIST);

    QAction *favoriteAction = menu.addAction(isFavorite ? tr("Remove from favorites")
                                                        : tr("Add to favorites"));

    QAction *detailsAction = menu.addAction(tr("Details"));

    menu.addSeparator();

    QAction *openHomepageAction = menu.addAction(tr("Open homepage"));
    openHomepageAction->setEnabled(!station.homepage.isEmpty());

    QAction *selectedAction = menu.exec(ui->tableView->viewport()->mapToGlobal(position));

    if (!selectedAction)
        return;

    if (selectedAction == playAction) {
        if (isCurrentStationPlaying) {
            playbackController.stop();
            //audioProcessor.stop();

            currentRadioPlayingAddress.clear();

            playerUIController.setPlayIcon();
            playerUIController.setDefaultImage();
            playerUIController.clearMetadata();

            clearTableViewColor();
            radioInfo->clearInfo();

            setIsBrowseStationLoaded(false);
            setIsStopClicked(true);
        } else {
            radioIndexNumber = index.row();
            currentStationIndex = index.row();

            onTableViewDoubleClicked(index);
        }

        return;
    }

    if (selectedAction == favoriteAction) {
        const QString data = station.iconUrl + "," + station.streamUrl + "," + station.station + ","
                             + station.country + "," + station.genre + "," + station.homepage;

        const bool isNowFavorite = favoriteManager->toggleFavorite(station.streamUrl,
                                                                   data,
                                                                   RADIO_BROWSER_PLAYLIST);

        if (station.streamUrl == currentPlayingStation.streamUrl)
            playerUIController.setFavorite(isNowFavorite);

        refreshFavoritePlaylist();

        return;
    }

    if (selectedAction == detailsAction) {
        StationDetailsDialog dialog(station, ui->tableView);
        dialog.exec();
        return;
    }

    if (selectedAction == openHomepageAction)
        QDesktopServices::openUrl(QUrl(station.homepage));
}