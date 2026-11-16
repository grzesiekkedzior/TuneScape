#include "include/radiolist.h"
#include <QCoreApplication>
#include <QDesktopServices>
#include <QFile>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonObject>
#include <QPainter>
#include <QScrollBar>
#include <QUrl>
#include <algorithm>
#include <utility>
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

    ui->playPause->setShortcut(QKeySequence(Qt::Key_Space));

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
    connect(ui->record, &QPushButton::clicked, this, &RadioList::startStopRecord);
    connect(&streamRecorder, &StreamRecorder::stateChanged, this, &RadioList::updateRecordButton);
    ui->recordLabel->setTextFormat(Qt::PlainText);
    connect(&streamRecorder, &StreamRecorder::bytesWrittenChanged, this, [this](qint64 bytes) {
        this->ui->recordLabel->setText(tr("Recording: %L1 bytes").arg(bytes));
    });
    connect(&streamRecorder, &StreamRecorder::recordingFinished, this,
            [this](const QString &path, qint64 bytes) {
                this->ui->recordLabel->setText(bytes > 0 ? tr("Recording saved (%L1 bytes)").arg(bytes)
                                                         : tr("Recording stopped (0 bytes)"));
                this->ui->recordLabel->setToolTip(path);
            });
    connect(&streamRecorder, &StreamRecorder::recordingFailed, this,
            [this](const QString &error, const QString &path, qint64 bytes) {
                this->ui->recordLabel->setText(path.isEmpty()
                    ? tr("Recording could not start")
                    : tr("Recording interrupted (%L1 bytes)").arg(bytes));
                this->ui->recordLabel->setToolTip(path.isEmpty() ? error : error + '\n' + path);
                this->ui->statusbar->showMessage(error, 10000);
            });
    connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit,
            this, &RadioList::stopStreamServices);
    updateRecordButton(streamRecorder.state());
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

    if (currentPlaylistIndex == Stations::FAVORITE)
        updateStationColoring();
    else if (!isStopClicked)
        setIndexColor(radioIndexNumber);

    if (!playbackController.isPlaying() || radioInfo->getInfoData().favicon.isEmpty()) {
        playerUIController.setDefaultImage();
    }
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
    const auto *model = ui->treeView->model();
    if (!model)
        return;
    const QModelIndex radioBrowserIndex = model->index(0, 0);
    const QModelIndex favoriteIndex = model->index(3, 0, radioBrowserIndex);
    if (!favoriteIndex.isValid())
        return;

    isTreeClicked = true;
    item = FAVORITE;
    if (currentPlaylistIndex != Stations::FAVORITE)
        switchToPlaylist(Stations::FAVORITE);
    ui->treeView->selectionModel()->select(favoriteIndex, QItemSelectionModel::ClearAndSelect);
    updateStationColoring();
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

StreamRecorder &RadioList::getStreamRecorder()
{
    return streamRecorder;
}

void RadioList::updateFavoriteColumnLayout()
{
    header->setSectionResizeMode(QHeaderView::Interactive);

    if (item == FAVORITE)
        ui->tableView->setColumnWidth(1, 16);
}

void RadioList::applyFavoritePlaylist(QVector<RadioStation> stations)
{
    if (allStations.size() <= Stations::FAVORITE)
        allStations.resize(Stations::FAVORITE + 1);
    allStations[Stations::FAVORITE] = std::move(stations);

    if (currentPlaylistIndex == Stations::FAVORITE) {
        radioStationsModel->setStations(allStations[Stations::FAVORITE]);
        loadStationIcons();
        updateStationColoring();
    }

    const bool active = playbackController.isPlaying() || playbackController.isPaused();
    const auto &station = country.getIsPlaying() ? country.getCurrentStation() : currentPlayingStation;
    playerUIController.setFavorite(active && isFavorite(station));
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
    allStations.resize(Stations::FAVORITE + 1);
    QVector<RadioStation> favorites;
    const QString error = favoriteManager->load(favorites);
    if (error.isEmpty())
        applyFavoritePlaylist(std::move(favorites));
    else
        QMessageBox::warning(mainWindow, tr("Cannot read Favorites"), error);

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
    if (currentPlaylistIndex == Stations::FAVORITE) {
        clearTableViewColor();
        if (currentPlayListPlaying == Stations::FAVORITE && playbackController.isPlaying()
            && !country.getIsPlaying()) {
            const auto &favorites = allStations[Stations::FAVORITE];
            const auto selected = std::find_if(favorites.cbegin(), favorites.cend(), [this](const auto &entry) {
                return FavoriteManager::sameStation(currentPlayingStation, entry);
            });
            if (selected != favorites.cend())
                setIndexColor(static_cast<int>(selected - favorites.cbegin()));
        }
        return;
    }

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

void RadioList::prepareStationChange(const QUrl &url)
{
    streamRecorder.stopRecording();
    streamReader.startStreaming(url);
}

bool RadioList::isFavorite(const RadioStation &station) const
{
    if (allStations.size() <= Stations::FAVORITE)
        return false;

    const auto &favorites = allStations[Stations::FAVORITE];
    return std::any_of(favorites.cbegin(), favorites.cend(), [&station](const RadioStation &entry) {
        return FavoriteManager::sameStation(station, entry);
    });
}

void RadioList::setFavorite(const RadioStation &station, bool favorite)
{
    QVector<RadioStation> updated;
    const QString error = favoriteManager->setFavorite(station, favorite, updated);
    if (!error.isEmpty()) {
        QMessageBox::warning(mainWindow, tr("Cannot update Favorites"), error);
        return;
    }
    applyFavoritePlaylist(std::move(updated));
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
    auto favorites = allStations.value(Stations::FAVORITE);
    allStations.clear();
    allStations.resize(Stations::FAVORITE + 1);
    allStations[Stations::FAVORITE] = std::move(favorites);
}

void RadioList::playSelectedStation(int radioNumber)
{
    radioIndexCurrentPlaying = radioNumber;
    currentPlayListPlaying = currentPlaylistIndex;

    currentPlayingStation = radioStationsModel->station(radioNumber);
    currentRadioPlayingAddress = currentPlayingStation.streamUrl;

    playerUIController.setFavorite(isFavorite(currentPlayingStation));
    const QUrl streamUrl(currentRadioPlayingAddress);
    prepareStationChange(streamUrl);
    playbackController.play(streamUrl);
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
    } else if (playbackController.isPaused()) {
        playbackController.resume();
    } else if (country.getCurrentIndexPlaying() != -1) {
        playCountryStream();

    } else if (currentRadioPlayingAddress.isEmpty() && !radioStationsModel->isEmpty()) {
        startRadioBrowserStream();

    } else if (playbackController.isAvailable()) {
        playbackController.resume();
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
    stopStreamServices();
    playbackController.stop();
}

void RadioList::returnRadioBrowserToPlay()
{
    playbackController.resume();
}

void RadioList::playCountryStream()
{
    playbackController.resume();
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
    stopStreamServices();
    if (!playbackController.isPlaying() && !playbackController.isPaused())
        return;

    isStopClicked = true;

    playerUIController.setPlayIcon();

    playbackController.stop();

    currentRadioPlayingAddress.clear();
    playerUIController.setFavorite(false);

    playerUIController.setDefaultImage();
    playerUIController.clearMetadata();

    const QModelIndex newIndex = ui->tableView->model()->index(0, 0);
    ui->tableView->setCurrentIndex(newIndex);

    radioIndexNumber = 0;

    clearTableViewColor();
    radioInfo->clearInfo();

    setIsBrowseStationLoaded(false);

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
        const auto &station = country.getCurrentStation();
        setFavorite(station, !isFavorite(station));
    } else if (playbackController.isPlaying()) {
        setFavorite(currentPlayingStation, !isFavorite(currentPlayingStation));
    }
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
    if (streamRecorder.state() != StreamRecorder::State::Idle) {
        streamRecorder.stopRecording();
        return;
    }

    if (!playbackController.isPlaying()) {
        ui->statusbar->showMessage(tr("Play a station to start recording."), 5000);
        return;
    }

    const auto &station = country.getIsPlaying() ? country.getCurrentStation() : currentPlayingStation;
    QString codec = station.codec;
    if (codec.isEmpty()) {
        const auto info = radioInfo->getInfoData();
        if (info.station == station.station)
            codec = info.codec;
    }
    streamRecorder.startRecording(QUrl(station.streamUrl), station.station, codec);
}

void RadioList::updateRecordButton(StreamRecorder::State state)
{
    using State = StreamRecorder::State;
    const bool active = state != State::Idle;
    const QString action = active ? tr("Stop recording") : tr("Start recording");
    ui->record->setIcon(QIcon(active ? ":/images/img/stoprecord.png" : ":/images/img/red-save-32.png"));
    ui->record->setToolTip(action);
    ui->record->setAccessibleName(action);
    ui->record->setEnabled(state != State::Stopping);

    if (state == State::Starting) {
        ui->recordLabel->setText(tr("Connecting for recording…"));
        ui->recordLabel->setToolTip({});
    }
}

void RadioList::stopStreamServices()
{
    streamRecorder.stopRecording();
    streamReader.stopStreaming();
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

    const bool favorite = isFavorite(station);

    QAction *favoriteAction = menu.addAction(favorite ? tr("Remove from favorites")
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
            stopRadioBrowserStream();

            currentRadioPlayingAddress.clear();
            playerUIController.setFavorite(false);

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
        setFavorite(station, !favorite);
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
