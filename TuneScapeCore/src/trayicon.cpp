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
{
    appConfig = new AppConfig("application.properties");
    appConfig->setParent(this);

    systemTrayIcon = new QSystemTrayIcon(QIcon(RadioIcon), this);

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
    nowPlayingStation->setObjectName(QStringLiteral("nowPlayingStation"));
    nowPlayingStation->setTextInteractionFlags(Qt::NoTextInteraction);

    QFont stationFont = nowPlayingStation->font();
    stationFont.setBold(true);
    nowPlayingStation->setFont(stationFont);

    nowPlayingTitle = new QLabel(tr("Select a station"), nowPlayingFrame);
    nowPlayingTitle->setTextFormat(Qt::PlainText);
    nowPlayingTitle->setTextInteractionFlags(Qt::NoTextInteraction);

    QFont titleFont = nowPlayingTitle->font();
    if (titleFont.pointSize() > 0)
        titleFont.setPointSize(qMax(8, titleFont.pointSize() - 1));
    nowPlayingTitle->setFont(titleFont);

    nowPlayingStatus = new QLabel(tr("Stopped"), nowPlayingFrame);
    nowPlayingStatus->setObjectName(QStringLiteral("nowPlayingStatus"));
    nowPlayingStatus->setTextInteractionFlags(Qt::NoTextInteraction);

    nowPlayingPlayPauseButton = new QPushButton(nowPlayingFrame);
    nowPlayingPlayPauseButton->setFixedSize(30, 30);
    nowPlayingPlayPauseButton->setFocusPolicy(Qt::NoFocus);
    nowPlayingPlayPauseButton->setCursor(Qt::PointingHandCursor);
    nowPlayingPlayPauseButton->setToolTip(tr("Play"));

    connect(nowPlayingPlayPauseButton, &QPushButton::clicked, this, [this]() {
        if (radioList)
            radioList->togglePlayback();
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
    nowPlayingStatus->setToolTip({});

    updateNowPlayingButton();
}

void TrayIcon::updateNowPlayingPanel()
{
    if (!playback || !playback->station()) {
        clearNowPlayingPanel();
        return;
    }

    const auto &station = *playback->station();
    const auto image = ui->infoLabel->pixmap();
    const QSize size(44, 44);
    nowPlayingIcon->setPixmap(image.isNull()
        ? QIcon(DefaultStationIconPath).pixmap(size)
        : QIcon(image).pixmap(size, nowPlayingIcon->devicePixelRatioF()));
    nowPlayingStation->setText(station.station);
    nowPlayingStation->setToolTip(station.station);

    const auto metadata = ui->infoData->text();
    const auto title = metadata.isEmpty() ? tr("Live stream") : metadata;
    nowPlayingTitle->setText(title);
    nowPlayingTitle->setToolTip(title);

    using State = PlaybackController::State;
    QString status;
    switch (playback->state()) {
    case State::Connecting:
        status = tr("Connecting");
        break;
    case State::Buffering:
        status = tr("Buffering");
        break;
    case State::Playing:
        status = tr("Playing");
        break;
    case State::Paused:
        status = tr("Paused");
        break;
    case State::Error:
        status = tr("Playback failed");
        break;
    default:
        status = tr("Stopped");
        break;
    }
    nowPlayingStatus->setText(status);
    nowPlayingStatus->setToolTip(playback->lastError());
    updateNowPlayingButton();
}

void TrayIcon::updateNowPlayingButton()
{
    if (!nowPlayingPlayPauseButton)
        return;

    if (playback && playback->canPause()) {
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
    if (!playback || !playback->hasActiveSession())
        return;

    for (auto it = stationActions.cbegin(); it != stationActions.cend(); ++it) {
        if (!playback->matches(it.value().station))
            continue;
        auto *action = it.key();
        if (playback->state() == PlaybackController::State::Playing)
            action->setIcon(QIcon(PlayingIconPath));
        action->setChecked(true);
        QFont playingFont = action->font();
        playingFont.setBold(true);
        if (playingFont.pointSize() > 0)
            playingFont.setPointSize(playingFont.pointSize() + 1);
        action->setFont(playingFont);
    }
}

void TrayIcon::trayIconButtonClicked()
{
    if (!systemTrayIcon->isVisible())
        systemTrayIcon->show();

    mainWindow->hide();

    updateNowPlayingPanel();
}

void TrayIcon::setNotifications(bool enabled)
{
    setIsNotificationEnable(enabled);
    turnOnOffNotification->setText(enabled ? NOTIFICATIONS.YES : NOTIFICATIONS.NO);
    appConfig->changeBoolState(enabled, NotificationsProperty);
}

void TrayIcon::loadTrayList(Stations station)
{
    if (!radioList)
        return;

    const QIcon defaultIcon(DefaultStationIconPath);
    const auto &stations = radioList->stations(station);
    const auto load = [&](QMenu *menu, QVector<QAction *> &actions, QVector<QIcon> &icons) {
        for (auto *action : actions)
            stationActions.remove(action);
        menu->clear();
        actions.clear();
        icons.fill(defaultIcon, stations.size());

        for (int row = 0; row < stations.size(); ++row) {
            QAction *action = menu->addAction(defaultIcon, stations[row].station);
            action->setCheckable(true);
            actions.append(action);
            stationActions.insert(action, {stations[row], station});
            const QUrl iconUrl(stations[row].iconUrl);
            if (!iconUrl.isEmpty()) {
                radioList->stationImages().request(iconUrl, QSize(32, 32), mainWindow->devicePixelRatioF(),
                    action, [this, action, row, &actions, &icons](auto, const QPixmap &pixmap) {
                        if (pixmap.isNull() || actions.value(row) != action)
                            return;
                        icons[row] = QIcon(pixmap);
                        if (action->isChecked() && playback
                            && playback->state() == PlaybackController::State::Playing)
                            updateNowPlayingPanel();
                        else
                            action->setIcon(icons[row]);
                    }, IconLoader::Priority::Background);
            }
        }
    };

    switch (station) {
    case Stations::TOP:
        load(top, topVector, topIcons);
        break;
    case Stations::POPULAR:
        load(popular, popularVector, popularIcons);
        break;
    case Stations::NEW:
        load(newRadio, newRadioVector, newRadioIcons);
        break;
    default:
        return;
    }
    setTrayListIcon();
}

void TrayIcon::trayMenuClicked(QAction *action)
{
    if (!action)
        return;
    if (action == exitAction) {
        QApplication::quit();
    } else if (action == playPauseAction && radioList) {
        radioList->togglePlayback();
    } else if (action == turnOnOffNotification) {
        setNotifications(!isNotificationEnabled);
    } else if (radioList && playback) {
        const auto entry = stationActions.constFind(action);
        if (entry != stationActions.cend()) {
            const auto selected = entry.value();
            radioList->selectPlaylist(selected.playlist);
            playback->play(selected.station);
        }
    }
}

void TrayIcon::handleTitleFromRadioList(const QString &data)
{
    updateNowPlayingPanel();
    if (data.isEmpty())
        return;

    if (!isNotificationEnabled || recentTitles.contains(data))
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

void TrayIcon::setRadioList(RadioList *list)
{
    if (radioList == list)
        return;
    if (radioList)
        disconnect(radioList, nullptr, this, nullptr);
    if (playback)
        disconnect(playback, nullptr, this, nullptr);
    radioList = list;
    playback = list ? &list->playback() : nullptr;
    if (list) {
        connect(list, &RadioList::trackTitleReceived, this, &TrayIcon::handleTitleFromRadioList);
        connect(list, &RadioList::stationImageChanged, this, &TrayIcon::updateNowPlayingPanel);
        connect(playback, &PlaybackController::stateChanged, this, [this] {
            updatePlayPauseAction();
            setTrayListIcon();
            updateNowPlayingPanel();
        });
        connect(playback, &PlaybackController::sessionChanged, this, [this] {
            recentTitles.clear();
        });
    }
    updatePlayPauseAction();
    updateNowPlayingPanel();
}

void TrayIcon::iconActivated(QSystemTrayIcon::ActivationReason reason)
{
    updateNowPlayingPanel();

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

void TrayIcon::updatePlayPauseAction()
{
    if (playback && playback->canPause()) {
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
