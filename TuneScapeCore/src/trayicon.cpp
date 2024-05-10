#include "include/trayicon.h"
#include <QMediaPlayer>
#include <QMenu>

TrayIcon::TrayIcon(Ui::MainWindow *ui, QMainWindow &mainWindow)
    : ui(ui)
    , mainWindow(&mainWindow)
{
    systemTrayIcon = new QSystemTrayIcon(QIcon(":/images/img/radio30.png"), &mainWindow);
    trayMenu = new QMenu(this);
    setClearTimer();

    playPauseAction = trayMenu->addAction(QIcon(":/images/img/play32.png"), "Play");
    exitAction = trayMenu->addAction(QIcon(":/images/img/exit64.png"), "Exit");
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
    }
    radioList->onPlayPauseButtonCliced();
}

void TrayIcon::handleTitleFromRadioList(const QString &data)
{
    if (recentTitles.isEmpty() || !recentTitles.contains(data)) {
        if (!mainWindow->isVisible()) {
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

QSystemTrayIcon *TrayIcon::getSystemTrayIcon() const
{
    return systemTrayIcon;
}

void TrayIcon::setRadioList(RadioList *newRadioList)
{
    radioList = newRadioList;
    connect(radioList, &RadioList::sendTitleToTray, this, &TrayIcon::handleTitleFromRadioList);
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
