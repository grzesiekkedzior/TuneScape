#include "include/trayicon.h"
#include <QMediaPlayer>
#include <QMenu>

Notifications NOTIFICATIONS;

TrayIcon::TrayIcon(Ui::MainWindow *ui, QMainWindow &mainWindow)
    : ui(ui)
    , mainWindow(&mainWindow)
{
    appConfig = new AppConfig("application.properties");
    systemTrayIcon = new QSystemTrayIcon(QIcon(":/images/img/radio30.png"), &mainWindow);
    trayMenu = new QMenu(this);
    setClearTimer();

    playPauseAction = trayMenu->addAction(QIcon(":/images/img/play32.png"), "Play");
    exitAction = trayMenu->addAction(QIcon(":/images/img/exit64.png"), "Exit");

    // load settings from application.properties file
    QString action = appConfig->checkBoolState() ? NOTIFICATIONS.YES : NOTIFICATIONS.NO;
    turnOnOffNotification = trayMenu->addAction(QIcon(":/images/img/notification-64.png"), action);
    isNotificationEnabled = appConfig->checkBoolState();

    systemTrayIcon->setContextMenu(trayMenu);
    connect(ui->tryIcon, &QPushButton::clicked, this, &TrayIcon::trayIconButtonClicked);
    connect(systemTrayIcon, &QSystemTrayIcon::activated, this, &TrayIcon::iconActivated);
    connect(trayMenu, &QMenu::triggered, this, &TrayIcon::trayMenuClicked);
}

void TrayIcon::setClearTimer()
{
    clearRecentTitlesTimer = new QTimer(this);
    clearRecentTitlesTimer->setInterval(240000);
    connect(clearRecentTitlesTimer, &QTimer::timeout, this, &TrayIcon::clearRecentTitles);
    clearRecentTitlesTimer->start();
}

void TrayIcon::trayIconButtonClicked()
{
    if (!systemTrayIcon->isVisible())
        systemTrayIcon->show();
    mainWindow->hide();
    if (radioAudioManager->getMediaPlayer()->isPlaying()) {
        trayMenu->actions().at(0)->setText("Pause");
        trayMenu->actions().at(0)->setIcon(QIcon(":/images/img/pause30.png"));
    } else {
        trayMenu->actions().at(0)->setText("Play");
        trayMenu->actions().at(0)->setIcon(QIcon(":/images/img/play30.png"));
    }
}

void TrayIcon::setNotifications(bool isNotificationEnabled)
{
    if (isNotificationEnabled) {
        setIsNotificationEnable(false);
        qDebug() << "disconnect";
        disconnect(radioList,
                   &RadioList::sendTitleToTray,
                   this,
                   &TrayIcon::handleTitleFromRadioList);
        trayMenu->actions().at(2)->setText(NOTIFICATIONS.NO);
        appConfig->changeBoolState(false);
    } else {
        qDebug() << "connect";
        connect(radioList, &RadioList::sendTitleToTray, this, &TrayIcon::handleTitleFromRadioList);
        setIsNotificationEnable(true);
        trayMenu->actions().at(2)->setText(NOTIFICATIONS.YES);
        appConfig->changeBoolState(true);
    }
}

void TrayIcon::trayMenuClicked(QAction *action)
{
    if (action == exitAction)
        QApplication::quit();
    if (action == playPauseAction) {
        if (radioAudioManager->getMediaPlayer()->isPlaying()) {
            trayMenu->actions().at(0)->setText("Pause");
            trayMenu->actions().at(0)->setIcon(QIcon(":/images/img/pause30.png"));
        } else {
            trayMenu->actions().at(0)->setText("Play");
            trayMenu->actions().at(0)->setIcon(QIcon(":/images/img/play30.png"));
        }
        radioList->onPlayPauseButtonCliced();
    }

    if (action == turnOnOffNotification) {
        qDebug() << "Hello Notification";
        setNotifications(isNotificationEnabled);
    }
}

void TrayIcon::handleTitleFromRadioList(const QString &data)
{
    if (recentTitles.isEmpty() || !recentTitles.contains(data)) {
        if (!mainWindow->isVisible()) {
            qDebug() << "Try message";
            systemTrayIcon->showMessage("Currently playing ",
                                        data,
                                        QIcon(":/images/img/playing32.png"),
                                        TRAY_TIME_MESSAGE);
        }

        recentTitles.insert(data);
    }
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
    connect(radioList, &RadioList::sendTitleToTray, this, &TrayIcon::handleTitleFromRadioList);
    if (getIsNotificationEnable())
        setNotifications(false);
    else
        setNotifications(true);
}

void TrayIcon::setRadioAudioManager(RadioAudioManager *newRadioAudioManager)
{
    radioAudioManager = newRadioAudioManager;
}

void TrayIcon::iconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (radioAudioManager->getMediaPlayer()->isPlaying()) {
        trayMenu->actions().at(0)->setText("Pause");
        trayMenu->actions().at(0)->setIcon(QIcon(":/images/img/pause30.png"));
    } else {
        trayMenu->actions().at(0)->setText("Play");
        trayMenu->actions().at(0)->setIcon(QIcon(":/images/img/play30.png"));
    }
    switch (reason) {
    case QSystemTrayIcon::Trigger:
        qDebug() << "Trriger";
        break;
    case QSystemTrayIcon::DoubleClick:
        if (mainWindow->isVisible()) {
            mainWindow->hide();
        } else {
            mainWindow->showNormal();
            mainWindow->activateWindow();
        }
        break;
    case QSystemTrayIcon::MiddleClick:
        // showMessage();
        break;
    default:;
    }
}

TrayIcon::~TrayIcon()
{
    delete systemTrayIcon;
    delete trayMenu;
    delete playPauseAction;
    delete exitAction;
}
