#include "include/trayicon.h"

#include <QApplication>
#include <QColor>
#include <QCursor>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QMediaPlayer>
#include <QMenu>
#include <QScreen>
#include <QVBoxLayout>

#include <algorithm>

Notifications NOTIFICATIONS;

namespace {

constexpr auto DefaultStationIconPath = ":/images/img/radio30.png";
constexpr auto PlayingIconPath = ":/images/img/playing32.png";
constexpr auto PlayIconPath = ":/images/img/play30.png";
constexpr auto PauseIconPath = ":/images/img/pause30.png";

} // namespace

TrayIcon::TrayIcon(Ui::MainWindow *ui, QMainWindow &mainWindow)
    : ui(ui)
    , mainWindow(&mainWindow)
    , topIconLoader(this)
    , popularIconLoader(this)
    , newIconLoader(this)
{
    appConfig = new AppConfig("application.properties");

    systemTrayIcon = new QSystemTrayIcon(QIcon(RadioIcon), &mainWindow);

    trayMenu = new QMenu(this);

    top = new QMenu(tr("Top"), trayMenu);
    top->setIcon(QIcon(":/images/img/top.png"));

    popular = new QMenu(tr("Popular"), trayMenu);
    popular->setIcon(QIcon(":/images/img/popular.png"));

    newRadio = new QMenu(tr("New"), trayMenu);
    newRadio->setIcon(QIcon(":/images/img/new.png"));

    const QString stationMenuStyle = QStringLiteral(R"(
    QMenu {
        padding: 2px;
    }

    QMenu::item {
        min-height: 22px;
        padding: 3px 22px 3px 8px;
        margin: 1px 2px;
        border-radius: 4px;
    }

    QMenu::item:selected {
        background-color: palette(alternate-base);
        color: palette(text);
    }

    QMenu::item:checked {
        background-color: palette(highlight);
        color: palette(highlighted-text);
    }

    QMenu::item:checked:selected {
        background-color: palette(highlight);
        color: palette(highlighted-text);
    }
)");

    top->setStyleSheet(stationMenuStyle);
    popular->setStyleSheet(stationMenuStyle);
    newRadio->setStyleSheet(stationMenuStyle);

    setClearTimer();
    createNowPlayingPanel();

    playPauseAction = trayMenu->addAction(QIcon(":/images/img/play32.png"), tr("Play"));

    exitAction = trayMenu->addAction(QIcon(":/images/img/exit64.png"), tr("Exit"));

    trayMenu->addMenu(top);
    trayMenu->addMenu(popular);
    trayMenu->addMenu(newRadio);

    const QString notificationActionText = appConfig->checkBoolState(NotificationsProperty)
                                               ? NOTIFICATIONS.YES
                                               : NOTIFICATIONS.NO;

    turnOnOffNotification = trayMenu->addAction(QIcon(":/images/img/notification-64.png"),
                                                notificationActionText);

    keepNowPlayingVisibleAction = trayMenu->addAction(tr("Keep now playing visible"));
    keepNowPlayingVisibleAction->setCheckable(true);

    connect(keepNowPlayingVisibleAction, &QAction::toggled, this, &TrayIcon::setNowPlayingPinned);

    isNotificationEnabled = appConfig->checkBoolState(NotificationsProperty);

    systemTrayIcon->setContextMenu(trayMenu);

    connect(ui->tryIcon, &QPushButton::clicked, this, &TrayIcon::trayIconButtonClicked);

    connect(systemTrayIcon, &QSystemTrayIcon::activated, this, &TrayIcon::iconActivated);

    connect(trayMenu, &QMenu::triggered, this, &TrayIcon::trayMenuClicked);

    connect(&topIconLoader, &IconLoader::iconLoaded, this, [this](int row, const QIcon &icon) {
        if (row < 0 || row >= topVector.size() || row >= topIcons.size()) {
            return;
        }

        topIcons[row] = icon;
        topVector[row]->setIcon(icon);

        if (playbackController.isPlaying())
            setTrayListIcon();

        updateNowPlayingPanel();
    });

    connect(&popularIconLoader, &IconLoader::iconLoaded, this, [this](int row, const QIcon &icon) {
        if (row < 0 || row >= popularVector.size() || row >= popularIcons.size()) {
            return;
        }

        popularIcons[row] = icon;
        popularVector[row]->setIcon(icon);

        if (playbackController.isPlaying())
            setTrayListIcon();

        updateNowPlayingPanel();
    });

    connect(&newIconLoader, &IconLoader::iconLoaded, this, [this](int row, const QIcon &icon) {
        if (row < 0 || row >= newRadioVector.size() || row >= newRadioIcons.size()) {
            return;
        }

        newRadioIcons[row] = icon;
        newRadioVector[row]->setIcon(icon);

        if (playbackController.isPlaying())
            setTrayListIcon();

        updateNowPlayingPanel();
    });
}

void TrayIcon::createNowPlayingPanel()
{
    nowPlayingWidget = new QWidget(nullptr,
                                   Qt::Tool | Qt::FramelessWindowHint | Qt::WindowDoesNotAcceptFocus
                                       | Qt::WindowStaysOnTopHint);

    nowPlayingWidget->setObjectName(QStringLiteral("nowPlayingWindow"));
    nowPlayingWidget->setAttribute(Qt::WA_ShowWithoutActivating);
    nowPlayingWidget->setAttribute(Qt::WA_TranslucentBackground);
    nowPlayingWidget->setAutoFillBackground(false);
    nowPlayingWidget->setMinimumWidth(260);

    nowPlayingFrame = new QWidget(nowPlayingWidget);
    nowPlayingFrame->setObjectName(QStringLiteral("nowPlayingPanel"));

    nowPlayingIcon = new QLabel(nowPlayingFrame);
    nowPlayingIcon->setFixedSize(44, 44);
    nowPlayingIcon->setAlignment(Qt::AlignCenter);
    nowPlayingIcon->setScaledContents(false);

    nowPlayingStation = new QLabel(tr("Nothing playing"), nowPlayingFrame);
    nowPlayingStation->setTextInteractionFlags(Qt::NoTextInteraction);

    QFont stationFont = nowPlayingStation->font();
    stationFont.setBold(true);
    nowPlayingStation->setFont(stationFont);

    nowPlayingTitle = new QLabel(tr("Select a station"), nowPlayingFrame);
    nowPlayingTitle->setTextInteractionFlags(Qt::NoTextInteraction);

    QFont titleFont = nowPlayingTitle->font();
    if (titleFont.pointSize() > 0)
        titleFont.setPointSize(qMax(8, titleFont.pointSize() - 1));
    nowPlayingTitle->setFont(titleFont);

    nowPlayingStatus = new QLabel(tr("Stopped"), nowPlayingFrame);
    nowPlayingStatus->setTextInteractionFlags(Qt::NoTextInteraction);

    nowPlayingPlayPauseButton = new QPushButton(nowPlayingFrame);
    nowPlayingPlayPauseButton->setFixedSize(30, 30);
    nowPlayingPlayPauseButton->setFocusPolicy(Qt::NoFocus);
    nowPlayingPlayPauseButton->setCursor(Qt::PointingHandCursor);
    nowPlayingPlayPauseButton->setToolTip(tr("Play"));

    connect(nowPlayingPlayPauseButton, &QPushButton::clicked, this, [this]() {
        if (!radioList)
            return;

        radioList->onPlayPauseButtonCliced();
        updatePlayPauseAction();
        updateNowPlayingPanel();

        if (playbackController.isPlaying()) {
            setTrayListIcon();
        } else {
            restoreStationIcons();
            clearStationHighlights();
        }
    });

    QFont statusFont = nowPlayingStatus->font();
    if (statusFont.pointSize() > 0)
        statusFont.setPointSize(qMax(8, statusFont.pointSize() - 1));
    nowPlayingStatus->setFont(statusFont);

    auto *textLayout = new QVBoxLayout;
    textLayout->setContentsMargins(0, 0, 0, 0);
    textLayout->setSpacing(1);
    textLayout->addWidget(nowPlayingStation);
    textLayout->addWidget(nowPlayingTitle);
    textLayout->addWidget(nowPlayingStatus);

    auto *panelLayout = new QHBoxLayout(nowPlayingFrame);
    panelLayout->setContentsMargins(9, 7, 9, 7);
    panelLayout->setSpacing(9);
    panelLayout->addWidget(nowPlayingIcon);
    panelLayout->addLayout(textLayout, 1);
    panelLayout->addWidget(nowPlayingPlayPauseButton, 0, Qt::AlignVCenter);

    auto *windowLayout = new QVBoxLayout(nowPlayingWidget);
    windowLayout->setContentsMargins(0, 0, 0, 0);
    windowLayout->setSpacing(0);
    windowLayout->addWidget(nowPlayingFrame);

    nowPlayingWidget->setStyleSheet(QStringLiteral(R"(
        QWidget#nowPlayingWindow {
            background: transparent;
            border: none;
        }

        QLabel {
            border: none;
            background: transparent;
        }
    )"));

    connect(trayMenu, &QMenu::aboutToShow, this, &TrayIcon::showNowPlayingPanel);

    connect(trayMenu, &QMenu::aboutToHide, this, &TrayIcon::hideNowPlayingPanel);

    clearNowPlayingPanel();
}

void TrayIcon::showNowPlayingPanel()
{
    if (!nowPlayingWidget)
        return;

    updateNowPlayingPanel();
    nowPlayingWidget->adjustSize();

    QTimer::singleShot(0, this, [this]() {
        if (!nowPlayingWidget)
            return;

        updateNowPlayingPanelStyle();

        if (keepNowPlayingVisible) {
            positionPinnedNowPlayingPanel();
        } else {
            if (!trayMenu || !trayMenu->isVisible())
                return;

            positionNowPlayingPanel();
        }

        nowPlayingWidget->show();
        nowPlayingWidget->raise();
    });
}

void TrayIcon::hideNowPlayingPanel()
{
    if (!nowPlayingWidget || keepNowPlayingVisible)
        return;

    nowPlayingWidget->hide();
}

void TrayIcon::updateNowPlayingPanelStyle()
{
    if (!trayMenu || !nowPlayingFrame)
        return;

    QColor background = trayMenu->palette().color(QPalette::Window);
    QColor text = trayMenu->palette().color(QPalette::WindowText);
    QColor border = trayMenu->palette().color(QPalette::Mid);

    // QSS themes do not always update QPalette. Once the menu is rendered,
    // sample its real background so the detached panel matches it.
    const QPixmap menuSnapshot = trayMenu->grab();

    if (!menuSnapshot.isNull()) {
        const QImage image = menuSnapshot.toImage();

        if (!image.isNull()) {
            const QPoint samplePoint(image.width() / 2, qMin(8, image.height() - 1));

            const QColor sampledColor = image.pixelColor(samplePoint);

            if (sampledColor.isValid() && sampledColor.alpha() > 0)
                background = sampledColor;
        }
    }

    const int luminance = (background.red() * 299 + background.green() * 587
                           + background.blue() * 114)
                          / 1000;

    text = luminance < 128 ? QColor(235, 235, 235) : QColor(30, 30, 30);

    border = luminance < 128 ? background.lighter(145) : background.darker(135);

    const QString panelStyle = QStringLiteral("QWidget#nowPlayingPanel {"
                                              "background-color: %1;"
                                              "border: 1px solid %2;"
                                              "border-radius: 8px;"
                                              "}"
                                              "QWidget#nowPlayingPanel QLabel {"
                                              "color: %3;"
                                              "border: none;"
                                              "background: transparent;"
                                              "}"
                                              "QWidget#nowPlayingPanel QPushButton {"
                                              "background-color: transparent;"
                                              "border: 1px solid %2;"
                                              "border-radius: 15px;"
                                              "padding: 3px;"
                                              "}"
                                              "QWidget#nowPlayingPanel QPushButton:hover {"
                                              "background-color: rgba(127, 127, 127, 45);"
                                              "}"
                                              "QWidget#nowPlayingPanel QPushButton:pressed {"
                                              "background-color: rgba(127, 127, 127, 75);"
                                              "}")
                                   .arg(background.name(QColor::HexArgb),
                                        border.name(QColor::HexArgb),
                                        text.name(QColor::HexArgb));

    nowPlayingFrame->setStyleSheet(panelStyle);
}

void TrayIcon::positionPinnedNowPlayingPanel()
{
    if (!nowPlayingWidget)
        return;

    constexpr int Margin = 16;

    nowPlayingWidget->adjustSize();

    QScreen *screen = QGuiApplication::screenAt(QCursor::pos());

    if (!screen && mainWindow)
        screen = mainWindow->screen();

    if (!screen)
        screen = QGuiApplication::primaryScreen();

    if (!screen)
        return;

    const QRect area = screen->availableGeometry();

    const int x = area.right() - nowPlayingWidget->width() - Margin + 1;

    const int y = area.top() + Margin;

    nowPlayingWidget->move(x, y);
}

void TrayIcon::setNowPlayingPinned(bool pinned)
{
    keepNowPlayingVisible = pinned;

    if (!nowPlayingWidget)
        return;

    if (pinned) {
        nowPlayingWidget->setWindowOpacity(0.88);
        updateNowPlayingPanel();
        updateNowPlayingPanelStyle();
        positionPinnedNowPlayingPanel();
        nowPlayingWidget->show();
        nowPlayingWidget->raise();
        return;
    }

    nowPlayingWidget->setWindowOpacity(1.0);

    if (trayMenu && trayMenu->isVisible()) {
        positionNowPlayingPanel();
        nowPlayingWidget->show();
        nowPlayingWidget->raise();
    } else {
        nowPlayingWidget->hide();
    }
}

void TrayIcon::positionNowPlayingPanel()
{
    if (!trayMenu || !nowPlayingWidget || keepNowPlayingVisible)
        return;

    constexpr int Gap = 8;

    nowPlayingWidget->adjustSize();

    const QRect menuGeometry = trayMenu->frameGeometry();
    const QSize panelSize = nowPlayingWidget->size();

    QScreen *screen = QGuiApplication::screenAt(menuGeometry.center());
    if (!screen)
        screen = QGuiApplication::primaryScreen();

    const QRect availableGeometry = screen ? screen->availableGeometry() : QRect();

    int x = menuGeometry.right() - panelSize.width() + 1;
    int y = menuGeometry.top() - panelSize.height() - Gap;

    if (!availableGeometry.isNull()) {
        x = qBound(availableGeometry.left(), x, availableGeometry.right() - panelSize.width() + 1);

        if (y < availableGeometry.top())
            y = menuGeometry.bottom() + Gap;

        if (y + panelSize.height() > availableGeometry.bottom() + 1)
            y = availableGeometry.bottom() - panelSize.height() + 1;
    }

    nowPlayingWidget->move(x, y);
}

void TrayIcon::clearNowPlayingPanel()
{
    if (!nowPlayingIcon || !nowPlayingStation || !nowPlayingTitle || !nowPlayingStatus)
        return;

    nowPlayingIcon->setPixmap(QIcon(DefaultStationIconPath).pixmap(QSize(44, 44)));

    nowPlayingStation->setText(tr("Nothing playing"));
    nowPlayingTitle->setText(tr("Select a station"));
    nowPlayingStatus->setText(tr("Stopped"));

    updateNowPlayingButton();
}

QString TrayIcon::currentStationName() const
{
    if (!radioList)
        return {};

    const int position = radioList->getRadioIndexNumber();
    const QString playlistName = radioList->getItem();

    if (position < 0)
        return {};

    const QVector<RadioStation> *stations = nullptr;

    if (playlistName == radioPlaylist.TOP)
        stations = &radioList->stations(Stations::TOP);
    else if (playlistName == radioPlaylist.POPULAR)
        stations = &radioList->stations(Stations::POPULAR);
    else if (playlistName == radioPlaylist.NEW)
        stations = &radioList->stations(Stations::NEW);

    if (!stations || position >= stations->size())
        return {};

    return stations->at(position).station;
}

QIcon TrayIcon::currentStationIcon() const
{
    if (!radioList)
        return QIcon(DefaultStationIconPath);

    const int position = radioList->getRadioIndexNumber();
    const QString playlistName = radioList->getItem();

    if (position < 0)
        return QIcon(DefaultStationIconPath);

    if (playlistName == radioPlaylist.TOP && position < topIcons.size())
        return topIcons.at(position);

    if (playlistName == radioPlaylist.POPULAR && position < popularIcons.size())
        return popularIcons.at(position);

    if (playlistName == radioPlaylist.NEW && position < newRadioIcons.size())
        return newRadioIcons.at(position);

    return QIcon(DefaultStationIconPath);
}

void TrayIcon::updateNowPlayingPanel()
{
    if (!nowPlayingIcon || !nowPlayingStation || !nowPlayingTitle || !nowPlayingStatus)
        return;

    const QString stationName = currentStationName();

    if (stationName.isEmpty()) {
        clearNowPlayingPanel();
        return;
    }

    nowPlayingIcon->setPixmap(currentStationIcon().pixmap(QSize(44, 44)));
    nowPlayingStation->setText(stationName);
    nowPlayingStation->setToolTip(stationName);

    const QString title = currentTrackTitle.isEmpty() ? tr("Live stream") : currentTrackTitle;

    nowPlayingTitle->setText(title);
    nowPlayingTitle->setToolTip(title);

    nowPlayingStatus->setText(playbackController.isPlaying() ? tr("Playing") : tr("Paused"));

    updateNowPlayingButton();
}

void TrayIcon::updateNowPlayingButton()
{
    if (!nowPlayingPlayPauseButton)
        return;

    if (playbackController.isPlaying()) {
        nowPlayingPlayPauseButton->setIcon(QIcon(PauseIconPath));
        nowPlayingPlayPauseButton->setToolTip(tr("Pause"));
    } else {
        nowPlayingPlayPauseButton->setIcon(QIcon(PlayIconPath));
        nowPlayingPlayPauseButton->setToolTip(tr("Play"));
    }

    nowPlayingPlayPauseButton->setIconSize(QSize(18, 18));
}

void TrayIcon::setClearTimer()
{
    clearRecentTitlesTimer = new QTimer(this);
    clearRecentTitlesTimer->setInterval(240000);

    connect(clearRecentTitlesTimer, &QTimer::timeout, this, &TrayIcon::clearRecentTitles);

    clearRecentTitlesTimer->start();
}

void TrayIcon::setTrayListIcon()
{
    restoreStationIcons();
    clearStationHighlights();

    if (!radioList || !playbackController.isPlaying())
        return;

    const int position = radioList->getRadioIndexNumber();
    const QString treeItem = radioList->getItem();

    if (position < 0 || treeItem.isEmpty())
        return;

    QAction *playingAction = nullptr;

    if (treeItem == radioPlaylist.TOP && position < topVector.size()) {
        playingAction = topVector[position];
    } else if (treeItem == radioPlaylist.POPULAR && position < popularVector.size()) {
        playingAction = popularVector[position];
    } else if (treeItem == radioPlaylist.NEW && position < newRadioVector.size()) {
        playingAction = newRadioVector[position];
    }

    if (!playingAction)
        return;

    playingAction->setIcon(QIcon(PlayingIconPath));
    playingAction->setChecked(true);

    QFont playingFont = playingAction->font();
    playingFont.setBold(true);

    if (playingFont.pointSize() > 0)
        playingFont.setPointSize(playingFont.pointSize() + 1);

    playingAction->setFont(playingFont);
    updateNowPlayingPanel();
}

void TrayIcon::trayIconButtonClicked()
{
    if (!systemTrayIcon->isVisible())
        systemTrayIcon->show();

    mainWindow->hide();

    updatePlayPauseAction();

    if (playbackController.isPlaying()) {
        setTrayListIcon();
    } else {
        restoreStationIcons();
        clearStationHighlights();
        updateNowPlayingPanel();
    }
}

void TrayIcon::setNotifications(bool notificationsEnabled)
{
    if (notificationsEnabled) {
        setIsNotificationEnable(false);

        disconnect(radioList,
                   &RadioList::sendTitleToTray,
                   this,
                   &TrayIcon::handleTitleFromRadioList);

        turnOnOffNotification->setText(NOTIFICATIONS.NO);

        appConfig->changeBoolState(false, NotificationsProperty);

        return;
    }

    connect(radioList,
            &RadioList::sendTitleToTray,
            this,
            &TrayIcon::handleTitleFromRadioList,
            Qt::UniqueConnection);

    setIsNotificationEnable(true);

    turnOnOffNotification->setText(NOTIFICATIONS.YES);

    appConfig->changeBoolState(true, NotificationsProperty);
}

void TrayIcon::loadTrayLists()
{
    if (!radioList)
        return;

    top->clear();
    popular->clear();
    newRadio->clear();

    topVector.clear();
    popularVector.clear();
    newRadioVector.clear();

    topIcons.clear();
    popularIcons.clear();
    newRadioIcons.clear();

    const auto &topStations = radioList->stations(Stations::TOP);

    const auto &popularStations = radioList->stations(Stations::POPULAR);

    const auto &newStations = radioList->stations(Stations::NEW);

    const QIcon defaultIcon(DefaultStationIconPath);

    topIcons.fill(defaultIcon, topStations.size());
    popularIcons.fill(defaultIcon, popularStations.size());
    newRadioIcons.fill(defaultIcon, newStations.size());

    for (const RadioStation &station : topStations) {
        QAction *action = top->addAction(defaultIcon, station.station);
        action->setCheckable(true);
        topVector.append(action);
    }

    for (const RadioStation &station : popularStations) {
        QAction *action = popular->addAction(defaultIcon, station.station);
        action->setCheckable(true);
        popularVector.append(action);
    }

    for (const RadioStation &station : newStations) {
        QAction *action = newRadio->addAction(defaultIcon, station.station);
        action->setCheckable(true);
        newRadioVector.append(action);
    }

    loadTrayStationIcons();
}

void TrayIcon::clearIcon()
{
    restoreStationIcons();
}

QModelIndex TrayIcon::createTrayRadioLists(QAction *action)
{
    if (!action || !radioList)
        return {};

    const auto topIt = std::find(topVector.cbegin(), topVector.cend(), action);

    const auto popularIt = std::find(popularVector.cbegin(), popularVector.cend(), action);

    const auto newIt = std::find(newRadioVector.cbegin(), newRadioVector.cend(), action);

    int treeNumber = -1;
    int stationIndex = -1;

    if (topIt != topVector.cend()) {
        treeNumber = radioTable.TOP;

        stationIndex = static_cast<int>(std::distance(topVector.cbegin(), topIt));

    } else if (popularIt != popularVector.cend()) {
        treeNumber = radioTable.POPULAR;

        stationIndex = static_cast<int>(std::distance(popularVector.cbegin(), popularIt));

    } else if (newIt != newRadioVector.cend()) {
        treeNumber = radioTable.NEWRADIO;

        stationIndex = static_cast<int>(std::distance(newRadioVector.cbegin(), newIt));
    }

    if (treeNumber < 0 || stationIndex < 0)
        return {};

    const QModelIndex libraryIndex = ui->treeView->model()->index(0, 0);

    const QModelIndex treeIndex = ui->treeView->model()->index(treeNumber, 0, libraryIndex);

    if (!treeIndex.isValid())
        return {};

    ui->treeView->selectionModel()->clearSelection();

    ui->treeView->selectionModel()->select(treeIndex, QItemSelectionModel::Select);

    radioList->setIsTreeClicked(true);
    radioList->onTrayViewItemClicked(treeIndex);

    return ui->tableView->model()->index(stationIndex, static_cast<int>(Column::StationColumn));
}

void TrayIcon::trayMenuClicked(QAction *action)
{
    if (!action)
        return;

    if (action == exitAction) {
        QApplication::quit();
        return;
    }

    if (action == playPauseAction) {
        radioList->onPlayPauseButtonCliced();

        updatePlayPauseAction();

        if (playbackController.isPlaying()) {
            setTrayListIcon();
        } else {
            restoreStationIcons();
            clearStationHighlights();
            updateNowPlayingPanel();
        }

        return;
    }

    if (action == turnOnOffNotification) {
        setNotifications(isNotificationEnabled);
        return;
    }

    const QModelIndex radioIndex = createTrayRadioLists(action);

    if (!radioIndex.isValid())
        return;

    currentTrackTitle.clear();

    radioList->onTrayClickedandPlay(radioIndex);
    radioList->setTrayRadioImage(radioIndex);

    updatePlayPauseAction();
    setTrayListIcon();
    updateNowPlayingPanel();
}

void TrayIcon::handleTitleFromRadioList(const QString &data)
{
    if (data.isEmpty())
        return;

    currentTrackTitle = data;
    updateNowPlayingPanel();

    if (recentTitles.contains(data))
        return;

    if (!mainWindow->isVisible()) {
        systemTrayIcon->showMessage(tr("Currently playing"),
                                    data,
                                    QIcon(PlayingIconPath),
                                    TrayTimeMessage);
    }

    recentTitles.insert(data);
}

void TrayIcon::clearRecentTitles()
{
    recentTitles.clear();
}

bool TrayIcon::getIsNotificationEnable() const
{
    return isNotificationEnabled;
}

void TrayIcon::setIsNotificationEnable(bool newIsNotificationEnable)
{
    isNotificationEnabled = newIsNotificationEnable;
}

QSystemTrayIcon *TrayIcon::getSystemTrayIcon() const
{
    return systemTrayIcon;
}

void TrayIcon::setRadioList(RadioList *newRadioList)
{
    radioList = newRadioList;

    if (!radioList)
        return;

    connect(radioList,
            &RadioList::sendTitleToTray,
            this,
            &TrayIcon::handleTitleFromRadioList,
            Qt::UniqueConnection);

    if (getIsNotificationEnable())
        setNotifications(false);
    else
        setNotifications(true);

    updateNowPlayingPanel();
}

void TrayIcon::iconActivated(QSystemTrayIcon::ActivationReason reason)
{
    updatePlayPauseAction();

    if (playbackController.isPlaying()) {
        setTrayListIcon();
    } else {
        restoreStationIcons();
        clearStationHighlights();
        updateNowPlayingPanel();
    }

    switch (reason) {
    case QSystemTrayIcon::DoubleClick:
        if (mainWindow->isVisible()) {
            mainWindow->hide();
        } else {
            mainWindow->showNormal();
            mainWindow->activateWindow();
        }
        break;

    case QSystemTrayIcon::Trigger:
    case QSystemTrayIcon::MiddleClick:
    case QSystemTrayIcon::Context:
    case QSystemTrayIcon::Unknown:
        break;
    }
}

QIcon TrayIcon::stationTrayIcon(const RadioStation &station) const
{
    if (!station.icon.isNull())
        return station.icon;

    return QIcon(DefaultStationIconPath);
}

void TrayIcon::restoreStationIcons()
{
    for (int i = 0; i < topVector.size() && i < topIcons.size(); ++i) {
        if (topVector[i])
            topVector[i]->setIcon(topIcons[i]);
    }

    for (int i = 0; i < popularVector.size() && i < popularIcons.size(); ++i) {
        if (popularVector[i])
            popularVector[i]->setIcon(popularIcons[i]);
    }

    for (int i = 0; i < newRadioVector.size() && i < newRadioIcons.size(); ++i) {
        if (newRadioVector[i])
            newRadioVector[i]->setIcon(newRadioIcons[i]);
    }
}

void TrayIcon::clearStationHighlights()
{
    const QFont defaultFont = top ? top->font() : font();

    const auto clearHighlights = [&defaultFont](const QVector<QAction *> &actions) {
        for (QAction *action : actions) {
            if (!action)
                continue;

            action->setChecked(false);
            action->setFont(defaultFont);
        }
    };

    clearHighlights(topVector);
    clearHighlights(popularVector);
    clearHighlights(newRadioVector);
}

void TrayIcon::loadTrayStationIcons()
{
    if (!radioList)
        return;

    const auto &topStations = radioList->stations(Stations::TOP);

    const auto &popularStations = radioList->stations(Stations::POPULAR);

    const auto &newStations = radioList->stations(Stations::NEW);

    for (int row = 0; row < topStations.size(); ++row) {
        const QString &iconUrl = topStations[row].iconUrl;

        if (iconUrl.isEmpty())
            continue;

        topIconLoader.loadIcon(row, QUrl(iconUrl), QSize(32, 32));
    }

    for (int row = 0; row < popularStations.size(); ++row) {
        const QString &iconUrl = popularStations[row].iconUrl;

        if (iconUrl.isEmpty())
            continue;

        popularIconLoader.loadIcon(row, QUrl(iconUrl), QSize(32, 32));
    }

    for (int row = 0; row < newStations.size(); ++row) {
        const QString &iconUrl = newStations[row].iconUrl;

        if (iconUrl.isEmpty())
            continue;

        newIconLoader.loadIcon(row, QUrl(iconUrl), QSize(32, 32));
    }
}

void TrayIcon::updatePlayPauseAction()
{
    if (playbackController.isPlaying()) {
        playPauseAction->setText(tr("Pause"));
        playPauseAction->setIcon(QIcon(PauseIconPath));
    } else {
        playPauseAction->setText(tr("Play"));
        playPauseAction->setIcon(QIcon(PlayIconPath));
    }
}

TrayIcon::~TrayIcon()
{
    delete nowPlayingWidget;
}