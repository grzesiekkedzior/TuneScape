#include "include/radiolist.h"
#include <QCoreApplication>
#include <QDesktopServices>
#include <QFile>
#include <QProgressDialog>
#include <QPersistentModelIndex>
#include <QTimer>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonObject>
#include <QPainter>
#include <QScrollBar>
#include <QUrl>
#include <algorithm>
#include <utility>
#include "include/StationDetailsDialog.h"

RadioList::RadioList(Ui::MainWindow *ui, FavoriteManager *favoriteManager, RadioBrowserApi *catalog)
    : radioStationsModel(new RadioStationsModel{this})
    , ui(ui)
    , m_catalog(catalog ? catalog : new RadioBrowserApi(this))
    , m_catalogProgress(new QProgressDialog(ui->centralwidget))
    , radioInfo(ui)
    , favoriteManager{favoriteManager}
    , iconLoader(this)
{
    m_catalogProgress->setWindowModality(Qt::NonModal);
    m_catalogProgress->setWindowTitle(tr("Radio Browser"));
    m_catalogProgress->setCancelButtonText(tr("Cancel"));
    m_catalogProgress->setRange(0, 0);
    connect(m_catalogProgress, &QProgressDialog::canceled, this, [this] {
        m_catalog->cancel(m_progressRequest);
    });
    connect(ui->tabRadioListWidget, &QTabWidget::currentChanged, this, [this](int index) {
        if (index == 0) {
            if (currentPlaylistIndex >= Stations::TOP && currentPlaylistIndex <= Stations::NEW)
                requestPlaylist(static_cast<Stations>(currentPlaylistIndex), true);
            if (m_playlistDisplayPending)
                displayPlaylist();
        } else {
            cancelSearch();
            if (currentPlaylistIndex >= Stations::TOP && currentPlaylistIndex <= Stations::NEW)
                hideCatalogProgress(m_playlistRequests[currentPlaylistIndex]);
        }
    });
    connect(&imageManager, &RadioImageManager::imageLoaded, this, [this](const QPixmap &image) {
        m_hasStationImage = true;
        playerUIController.setStationImage(image);
    });

    ui->playPause->setShortcut(QKeySequence(Qt::Key_Space));

    connect(ui->treeView, &QTreeView::clicked, this, &RadioList::onTreeViewItemClicked);

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

RadioList::~RadioList()
{
    delete m_catalogProgress;
}

RadioBrowserApi &RadioList::catalog() const
{
    return *m_catalog;
}

void RadioList::showCatalogProgress(RadioBrowserApi::RequestId id, const QString &message)
{
    m_progressRequest = id;
    m_progressMessage = message;
    m_catalogProgress->setLabelText(message);
    ui->statusbar->showMessage(message);
    QTimer::singleShot(650, this, [this, id] {
        if (m_progressRequest == id && ui->centralwidget->isVisible())
            m_catalogProgress->show();
    });
}

void RadioList::hideCatalogProgress(RadioBrowserApi::RequestId id)
{
    if (!id || m_progressRequest != id)
        return;
    m_progressRequest = 0;
    m_catalogProgress->hide();
    if (ui->statusbar->currentMessage() == m_progressMessage)
        ui->statusbar->clearMessage();
    m_progressMessage.clear();
}

void RadioList::showCatalogResult(RadioBrowserApi::RequestId id, const RadioBrowserApi::Result &result)
{
    if (!id || m_progressRequest != id)
        return;
    hideCatalogProgress(id);

    using Status = RadioBrowserApi::Status;
    switch (result.status) {
    case Status::Success:
        return;
    case Status::EmptyResult:
        ui->statusbar->showMessage(tr("Radio Browser returned no results."), 5000);
        return;
    case Status::Cancelled:
        ui->statusbar->showMessage(tr("Radio Browser request cancelled."), 5000);
        return;
    case Status::NetworkError:
    case Status::InvalidResponse:
    case Status::Timeout:
        ui->statusbar->showMessage(tr("Radio Browser request failed — %1").arg(result.detail), 10000);
        return;
    }
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

    if (!playbackController.isPlaying() || !m_hasStationImage) {
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
    allStations.resize(Stations::SEARCH + 1);
    QVector<RadioStation> favorites;
    const QString error = favoriteManager->load(favorites);
    if (error.isEmpty())
        applyFavoritePlaylist(std::move(favorites));
    else
        QMessageBox::warning(mainWindow, tr("Cannot read Favorites"), error);

    setTopListOnStart();
    requestPlaylist(Stations::POPULAR, false);
    requestPlaylist(Stations::NEW, false);
}

auto checkItem = [](const QString &item, const QString &target) { return item == target; };

//Main function
void RadioList::onTreeViewItemClicked(const QModelIndex &index)
{
    if (!index.isValid() || !index.parent().isValid())
        return;
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
    updateStationColoring();
}

void RadioList::resetTreeItemIfSearch()
{
    if (this->treeItem == "Search")
        this->treeItem = "";
}

void RadioList::switchToPlaylist(Stations station)
{
    cancelSearch();
    for (auto id : m_playlistRequests)
        hideCatalogProgress(id);
    resetTreeItemIfSearch();
    currentPlaylistIndex = station;
    displayPlaylist();
    if (station <= Stations::NEW)
        requestPlaylist(station, ui->tabRadioListWidget->currentIndex() == 0);
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

void RadioList::playSelectedStation(int radioNumber)
{
    if (radioNumber < 0 || radioNumber >= radioStationsModel->size())
        return;
    radioIndexCurrentPlaying = radioNumber;
    currentPlayListPlaying = currentPlaylistIndex;

    currentPlayingStation = radioStationsModel->station(radioNumber);
    currentRadioPlayingAddress = currentPlayingStation.streamUrl;

    playerUIController.setFavorite(isFavorite(currentPlayingStation));
    const QUrl streamUrl(currentRadioPlayingAddress);
    prepareStationChange(streamUrl);
    playbackController.play(streamUrl);
    loadPlayingStationDetails(currentPlayingStation);
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

void RadioList::loadPlayingStationDetails(const RadioStation &station)
{
    clearPlayingStationDetails();
    radioInfo.showStation(station);
    playerUIController.setDefaultImage();
    imageManager.load(QUrl(station.iconUrl));

    QUrl endpoint;
    if (station.stationUuid.isNull()) {
        endpoint = RadioBrowserApi::endpoint("json/stations/search", "name", station.station);
        endpoint.setQuery(endpoint.query(QUrl::FullyEncoded) + "&nameExact=true", QUrl::StrictMode);
    } else {
        endpoint = QUrl("json/stations/byuuid/" + station.stationUuid.toString(QUuid::WithoutBraces));
    }
    m_detailsRequest = m_catalog->request(endpoint, this, [this, station](auto id, const auto &result) {
        if (id != m_detailsRequest)
            return;
        m_detailsRequest = 0;
        showCatalogResult(id, result);
        if (result.status != RadioBrowserApi::Status::Success)
            return;
        const auto matches = JsonListProcessor::stationsFromJson(result.data);
        auto match = matches.cbegin();
        if (station.stationUuid.isNull() || matches.size() > 1) {
            match = std::find_if(matches.cbegin(), matches.cend(), [&station](const RadioStation &item) {
                return FavoriteManager::sameStation(station, item);
            });
        }
        if (match == matches.cend())
            return;
        const auto index = match - matches.cbegin();
        m_detailsCodec = match->codec;
        radioInfo.showStation(*match, QString::number(result.data.at(index).toObject()["clicktrend"].toInt()));
    });
    if (!m_progressRequest)
        showCatalogProgress(m_detailsRequest, tr("Loading station details…"));
}

void RadioList::clearPlayingStationDetails()
{
    const auto id = std::exchange(m_detailsRequest, 0);
    hideCatalogProgress(id);
    m_catalog->cancel(id);
    imageManager.cancel();
    m_hasStationImage = false;
    m_detailsCodec.clear();
    radioInfo.clearInfo();
}

void RadioList::onTableViewDoubleClicked(const QModelIndex &index)
{
    if (!index.isValid() || index.row() < 0 || index.row() >= radioStationsModel->size())
        return;

    radioIndexNumber = index.row();
    currentStationIndex = index.row();
    playSelectedStation(radioIndexNumber);
    clearTableViewColor();
    country.clearTableColor();
    country.setIsPlaying(false);
    country.setCurrentIndexPlaying(-1);
    setIndexColor(radioIndexNumber);
    setIsBrowseStationLoaded(true);
    playerUIController.setPauseIcon();
    isStopClicked = false;
    playerUIController.clearMetadata();
    setIsSearchTablelDoubleCliced(treeItem == "Search");
}

//Main function
void RadioList::onPlayPauseButtonCliced()
{
    if (!isTreeClicked && !playbackController.isPlaying() && !playbackController.isPaused())
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
        playSelectedStation(ui->tableView->currentIndex().row());
    }

    if (currentPlayListPlaying == currentPlaylistIndex)
        setIndexColor(radioIndexNumber);

    updatePlayPauseIcons();
    setIsBrowseStationLoaded(playbackController.isPlaying() || playbackController.isPaused());
    switchToDefaultTabIfNoCountryStationPlaying();
    if (playbackController.isPlaying())
        isStopClicked = false;
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
    if (radioIndexNumber < 0 || radioIndexNumber >= radioStationsModel->size())
        radioIndexNumber = 0;
    playSelectedStation(radioIndexNumber);

    const QModelIndex newIndex = ui->tableView->model()->index(radioIndexNumber, 0);
    ui->tableView->setCurrentIndex(newIndex);

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
    clearPlayingStationDetails();
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
    if (codec.isEmpty())
        codec = m_detailsCodec;
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

void RadioList::requestPlaylist(Stations station, bool showProgress)
{
    static const std::array<QUrl, 3> endpoints = {
        QUrl("json/stations/topvote/20"), QUrl("json/stations/topclick/20"),
        QUrl("json/stations/lastchange/20")
    };
    if (m_playlistLoaded[station])
        return;
    auto &request = m_playlistRequests[station];
    if (!request) {
        request = m_catalog->request(endpoints[station], this, [this, station](auto id, const auto &result) {
            m_playlistRequests[station] = 0;
            showCatalogResult(id, result);
            using Status = RadioBrowserApi::Status;
            if (result.status != Status::Success && result.status != Status::EmptyResult)
                return;
            m_playlistLoaded[station] = true;
            allStations[station] = JsonListProcessor::stationsFromJson(result.data);
            if (currentPlaylistIndex == station) {
                m_playlistDisplayPending = true;
                if (ui->tabRadioListWidget->currentIndex() == 0)
                    displayPlaylist();
            }
            emit catalogPlaylistLoaded(station);
        });
    }
    if (showProgress)
        showCatalogProgress(request, tr("Loading %1 stations…").arg(item));
}

void RadioList::displayPlaylist()
{
    if (currentPlaylistIndex < 0 || currentPlaylistIndex >= allStations.size())
        return;
    m_playlistDisplayPending = false;
    radioStationsModel->setStations(allStations[currentPlaylistIndex]);
    loadStationIcons();
    updateStationColoring();
}

void RadioList::cancelSearch()
{
    const auto id = std::exchange(m_searchRequest, 0);
    hideCatalogProgress(id);
    m_catalog->cancel(id);
}

void RadioList::searchStations()
{
    cancelSearch();
    for (auto id : m_playlistRequests)
        hideCatalogProgress(id);
    const auto endpoint = RadioBrowserApi::endpoint("json/stations/search", "name", ui->serachInput->text());
    if (allStations.size() <= Stations::SEARCH)
        allStations.resize(Stations::SEARCH + 1);
    allStations[Stations::SEARCH].clear();
    currentPlaylistIndex = Stations::SEARCH;
    treeItem = "Search";
    item = "Search";
    ui->tabRadioListWidget->setCurrentIndex(0);
    displayPlaylist();

    m_searchRequest = m_catalog->request(endpoint, this, [this](auto id, const auto &result) {
        if (id != m_searchRequest)
            return;
        m_searchRequest = 0;
        showCatalogResult(id, result);
        using Status = RadioBrowserApi::Status;
        if (currentPlaylistIndex == Stations::SEARCH && ui->tabRadioListWidget->currentIndex() == 0
            && (result.status == Status::Success || result.status == Status::EmptyResult)) {
            allStations[Stations::SEARCH] = JsonListProcessor::stationsFromJson(result.data);
            displayPlaylist();
        }
    });
    showCatalogProgress(m_searchRequest, tr("Searching Radio Browser…"));
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
    iconLoader.cancelPending();
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

    const QPersistentModelIndex index(ui->tableView->indexAt(position));

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
            clearPlayingStationDetails();

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
