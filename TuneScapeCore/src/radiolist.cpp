#include "include/radiolist.h"
#include <QCoreApplication>
#include <QDesktopServices>
#include <QMenu>
#include <QMessageBox>
#include "include/jsonlistprocessor.h"
#include <QProgressDialog>
#include <QPersistentModelIndex>
#include <QTimer>
#include <QHeaderView>
#include <QJsonObject>
#include <QUrl>
#include <algorithm>
#include <utility>
#include "include/StationDetailsDialog.h"

RadioList::RadioList(Ui::MainWindow *ui, FavoriteManager *favoriteManager, RadioBrowserApi *catalog,
                     PlaybackController *playback)
    : radioStationsModel(new RadioStationsModel{this})
    , ui(ui)
    , m_catalog(catalog ? catalog : new RadioBrowserApi(this))
    , m_playback(playback ? playback : new PlaybackController(this))
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

    ui->playPause->setShortcut(QKeySequence(Qt::Key_Space));

    connect(ui->treeView, &QTreeView::clicked, this, &RadioList::onTreeViewItemClicked);

    connect(ui->playPause, &QPushButton::clicked, this, &RadioList::togglePlayback);
    connect(miniPlayer.getMui()->play, &QPushButton::clicked, this, &RadioList::togglePlayback);
    connect(ui->next, &QPushButton::clicked, this, [this] { playAdjacentStation(1); });
    connect(ui->previous, &QPushButton::clicked, this, [this] { playAdjacentStation(-1); });
    connect(ui->stop, &QPushButton::clicked, m_playback, &PlaybackController::stop);
    connect(ui->tableView, &QTableView::activated, this, &RadioList::playIndex);
    connect(ui->tableView, &QTableView::doubleClicked, this, &RadioList::playIndex);
    connect(m_playback, &PlaybackController::sessionChanged, this, &RadioList::onSessionChanged);
    connect(radioStationsModel, &RadioStationsModel::modelReset, this, &RadioList::updateStationColoring);

    connect(ui->horizontalVolumeSlider, &QSlider::valueChanged, this, &RadioList::sliderMoved);
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
            m_playback, &PlaybackController::stop);
    updateRecordButton(streamRecorder.state());
    connect(ui->serachInput, &QLineEdit::returnPressed, this, &RadioList::searchStations);
    connect(ui->favorite, &QPushButton::clicked, this, &RadioList::addRadioToFavorite);
    connect(ui->minplr, &QPushButton::clicked, this, &RadioList::showMiniplayer);
    connect(miniPlayer.getMui()->maxWindow, &QPushButton::clicked, this, &RadioList::maximizeWindow);

    //trash header signal
    ui->tableView->setModel(radioStationsModel);
    auto *header = ui->tableView->horizontalHeader();
    header->setSectionResizeMode(QHeaderView::Interactive);
    ui->tableView->verticalHeader()->setDefaultSectionSize(ui->tableView->fontMetrics().height()
                                                           + 2);
    playerUIController.initialize(ui, &miniPlayer, *m_playback);
    m_playback->setVolume(ui->horizontalVolumeSlider->value());
    ui->tableView->setIconSize(QSize(20, 20));
    radioStationsModel->bindIcons(ui->tableView, &iconLoader);

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
    if (m_playback)
        m_playback->stop();
    delete m_catalogProgress;
}

RadioBrowserApi &RadioList::catalog() const
{
    return *m_catalog;
}

IconLoader &RadioList::stationImages()
{
    return iconLoader;
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
    miniPlayer.show();
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
    updateStationColoring();
}

PlaybackController &RadioList::playback() const
{
    return *m_playback;
}

miniplayer &RadioList::miniPlayerWindow()
{
    return miniPlayer;
}

const QVector<RadioStation> &RadioList::stations(Stations station) const
{
    static const QVector<RadioStation> empty;
    return station >= 0 && station < allStations.size() ? allStations.at(station) : empty;
}

void RadioList::setMainWindow(QMainWindow *window)
{
    mainWindow = window;
}

void RadioList::setFavoriteLibrary()
{
    selectPlaylist(Stations::FAVORITE);
}

void RadioList::selectPlaylist(Stations station)
{
    const auto *model = ui->treeView->model();
    if (!model)
        return;
    const auto index = model->index(station, 0, model->index(0, 0));
    if (!index.isValid())
        return;
    ui->treeView->selectionModel()->select(index, QItemSelectionModel::ClearAndSelect);
    onTreeViewItemClicked(index);
}

StreamRecorder &RadioList::getStreamRecorder()
{
    return streamRecorder;
}

void RadioList::applyFavoritePlaylist(QVector<RadioStation> stations)
{
    if (allStations.size() <= Stations::FAVORITE)
        allStations.resize(Stations::FAVORITE + 1);
    allStations[Stations::FAVORITE] = std::move(stations);

    if (currentPlaylistIndex == Stations::FAVORITE) {
        radioStationsModel->setStations(allStations[Stations::FAVORITE]);
    }

    const auto &station = m_playback->station();
    playerUIController.setFavorite(m_playback->hasActiveSession() && station && isFavorite(*station));
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

void RadioList::onTreeViewItemClicked(const QModelIndex &index)
{
    if (!index.isValid() || !index.parent().isValid())
        return;

    const auto name = index.data().toString();
    if (name == "Top")
        switchToPlaylist(Stations::TOP);
    else if (name == "Popular")
        switchToPlaylist(Stations::POPULAR);
    else if (name == "New")
        switchToPlaylist(Stations::NEW);
    else if (name == "Favorite")
        switchToPlaylist(Stations::FAVORITE);
}

void RadioList::switchToPlaylist(Stations station)
{
    cancelSearch();
    for (auto id : m_playlistRequests)
        hideCatalogProgress(id);
    currentPlaylistIndex = station;
    displayPlaylist();
    if (station <= Stations::NEW)
        requestPlaylist(station, ui->tabRadioListWidget->currentIndex() == 0);
}

void RadioList::updateStationColoring()
{
    if (!theme || !m_playback)
        return;
    if (!customColor) {
        customColor = new CustomColorDelegate(-1, theme->playingRowColor(), this);
        ui->tableView->setItemDelegate(customColor);
    }
    const auto &station = m_playback->station();
    const int row = m_playback->hasActiveSession() && station
        ? radioStationsModel->rowForStation(*station) : -1;
    customColor->setPlayingRow(row, theme->playingRowColor());
    ui->tableView->viewport()->update();
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

void RadioList::setTheme(Theme *newTheme)
{
    theme = newTheme;
    updateStationColoring();
}

void RadioList::sliderMoved(int volume)
{
    m_playback->setVolume(volume);
    playerUIController.setVolumeIcon(volume);
}

void RadioList::onSessionChanged()
{
    stopStreamServices();
    clearPlayingStationDetails();
    m_trackTitle.clear();
    playerUIController.clearMetadata();
    playerUIController.setDefaultImage();

    if (!m_playbackStatus.isEmpty() && ui->statusbar->currentMessage() == m_playbackStatus)
        ui->statusbar->clearMessage();
    m_playbackStatus.clear();

    const auto &station = m_playback->station();
    const bool active = m_playback->hasActiveSession() && station.has_value();
    playerUIController.setFavorite(active && isFavorite(*station));
    updateStationColoring();
    if (active) {
        const auto current = *station;
        streamReader.startStreaming(m_playback->streamUrl(), m_playback->generation());
        loadPlayingStationDetails(current);
    } else if (m_playback->state() == PlaybackController::State::Error) {
        if (station)
            radioInfo.showStation(*station);
        m_playbackStatus = station
            ? tr("Playback failed for %1: %2").arg(station->station, m_playback->lastError())
            : tr("Playback failed: %1").arg(m_playback->lastError());
        ui->statusbar->showMessage(m_playbackStatus, 10000);
    }
}

void RadioList::loadPlayingStationDetails(const RadioStation &station)
{
    radioInfo.showStation(station);
    const auto generation = m_playback->generation();
    m_imageRequest = iconLoader.request(QUrl(station.iconUrl), QSize(120, 120),
        ui->infoLabel->devicePixelRatioF(), this, [this, generation](auto id, const QPixmap &image) {
            if (id != m_imageRequest || generation != m_playback->generation())
                return;
            m_imageRequest = 0;
            if (!image.isNull()) {
                playerUIController.setStationImage(image);
                emit stationImageChanged();
            }
        });

    QUrl endpoint;
    if (station.stationUuid.isNull()) {
        endpoint = RadioBrowserApi::endpoint("json/stations/search", "name", station.station);
        endpoint.setQuery(endpoint.query(QUrl::FullyEncoded) + "&nameExact=true", QUrl::StrictMode);
    } else {
        endpoint = QUrl("json/stations/byuuid/" + station.stationUuid.toString(QUuid::WithoutBraces));
    }
    m_detailsRequest = m_catalog->request(endpoint, this, [this, station, generation](auto id, const auto &result) {
        if (id != m_detailsRequest || generation != m_playback->generation())
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
    iconLoader.cancel(std::exchange(m_imageRequest, 0));
    m_detailsCodec.clear();
    radioInfo.clearInfo();
}

void RadioList::playIndex(const QModelIndex &index)
{
    if (index.isValid() && index.model() == radioStationsModel
        && index.row() >= 0 && index.row() < radioStationsModel->size()) {
        m_playback->play(radioStationsModel->station(index.row()));
    }
}

QTableView *RadioList::currentStationView() const
{
    return ui->tabRadioListWidget->currentWidget() == ui->listCountries
        ? ui->tableOfCoutries : ui->tableView;
}

void RadioList::togglePlayback()
{
    if (m_playback->hasActiveSession() || m_playback->state() == PlaybackController::State::Error) {
        m_playback->toggle();
        return;
    }

    const auto *view = currentStationView();
    const auto *model = qobject_cast<RadioStationsModel *>(view->model());
    if (!model || model->isEmpty())
        return;
    const auto index = view->currentIndex();
    const int selected = index.isValid() && index.model() == model ? index.row() : 0;
    if (selected >= 0 && selected < model->size())
        m_playback->toggle(model->station(selected));
}

void RadioList::playAdjacentStation(int offset)
{
    const auto &station = m_playback->station();
    if (!m_playback->hasActiveSession() || !station)
        return;
    const auto *model = qobject_cast<RadioStationsModel *>(currentStationView()->model());
    if (!model)
        return;
    const int current = model->rowForStation(*station);
    if (current >= 0 && current + offset >= 0 && current + offset < model->size())
        m_playback->play(model->station(current + offset));
}

void RadioList::addRadioToFavorite()
{
    const auto &station = m_playback->station();
    if (m_playback->hasActiveSession() && station)
        setFavorite(*station, !isFavorite(*station));
}

void RadioList::handleDataReceived(quint64 generation, const QString &data)
{
    if (generation != m_playback->generation() || !m_playback->hasActiveSession())
        return;
    const int start = data.indexOf("StreamTitle='");
    const int end = data.indexOf("';", start);
    if (start < 0 || end < 0)
        return;
    const QString title = data.mid(start + 13, end - (start + 13));
    if (title == m_trackTitle)
        return;
    m_trackTitle = title;
    playerUIController.setMetadata(title);
    emit trackTitleReceived(title);
}

void RadioList::startStopRecord()
{
    if (streamRecorder.state() != StreamRecorder::State::Idle) {
        streamRecorder.stopRecording();
        return;
    }

    const auto &station = m_playback->station();
    if (m_playback->state() != PlaybackController::State::Playing || !station) {
        ui->statusbar->showMessage(tr("Play a station to start recording."), 5000);
        return;
    }
    const auto codec = station->codec.isEmpty() ? m_detailsCodec : station->codec;
    streamRecorder.startRecording(m_playback->streamUrl(), station->station, codec);
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
        showCatalogProgress(request, tr("Loading %1 stations…").arg(ui->treeView->model()->index(
            station, 0, ui->treeView->model()->index(0, 0)).data().toString()));
}

void RadioList::displayPlaylist()
{
    if (currentPlaylistIndex < 0 || currentPlaylistIndex >= allStations.size())
        return;
    m_playlistDisplayPending = false;
    radioStationsModel->setStations(allStations[currentPlaylistIndex]);
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

    const bool activeStation = m_playback->hasActiveSession() && m_playback->matches(station);
    const auto generation = m_playback->generation();

    QAction *playAction = menu.addAction(activeStation ? tr("Stop") : tr("Play"));

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
        if (activeStation) {
            if (m_playback->generation() == generation && m_playback->matches(station))
                m_playback->stop();
        } else if (index.isValid()) {
            m_playback->play(station);
        }

        return;
    }

    if (selectedAction == favoriteAction) {
        setFavorite(station, !favorite);
        return;
    }

    if (selectedAction == detailsAction) {
        StationDetailsDialog dialog(station, iconLoader, ui->tableView);
        dialog.exec();
        return;
    }

    if (selectedAction == openHomepageAction)
        QDesktopServices::openUrl(QUrl(station.homepage));
}
