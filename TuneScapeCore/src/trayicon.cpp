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
    //Top
    top = new QMenu("Top", trayMenu);
    top->setIcon(QIcon(":/images/img/top.png"));

    //Popular
    popular = new QMenu("Popular", trayMenu);
    popular->setIcon(QIcon(":/images/img/popular.png"));

    // NewStations
    newRadio = new QMenu("New", trayMenu);
    newRadio->setIcon(QIcon(":/images/img/new.png"));

    setClearTimer();

    playPauseAction = trayMenu->addAction(QIcon(":/images/img/play32.png"), "Play");
    exitAction = trayMenu->addAction(QIcon(":/images/img/exit64.png"), "Exit");
    trayMenu->addMenu(top);
    trayMenu->addMenu(popular);
    trayMenu->addMenu(newRadio);

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

void TrayIcon::setTrayListIcon()
{
    if (radioList->getIsSearchTablelDoubleCliced() || radioList->getItem() == radioPlaylist.FAVORITE
        || radioList->getItem() == radioPlaylist.TOP
        || radioList->getItem() == radioPlaylist.POPULAR
        || radioList->getItem() == radioPlaylist.NEW) {
        clearIcon();
    }

    int position = radioList->getRadioIndexNumber();
    QString treeItem = radioList->getItem();
    if (treeItem.isEmpty() || position > topVector.size())
        return;
    qDebug() << "item ---------> " << treeItem;
    QString iconAddress = ":/images/img/playing32.png";
    if (treeItem == radioPlaylist.TOP)
        topVector[position]->setIcon(QIcon(iconAddress));
    else if (treeItem == radioPlaylist.POPULAR)
        popularVector[position]->setIcon(QIcon(iconAddress));
    else if (treeItem == radioPlaylist.NEW)
        newRadioVector[position]->setIcon(QIcon(iconAddress));
}

void TrayIcon::trayIconButtonClicked()
{
    if (!systemTrayIcon->isVisible())
        systemTrayIcon->show();
    mainWindow->hide();
    if (radioAudioManager->getMediaPlayer()->isPlaying()) {
        trayMenu->actions().at(0)->setText("Pause");
        trayMenu->actions().at(0)->setIcon(QIcon(":/images/img/pause30.png"));

        setTrayListIcon();

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
        trayMenu->actions().at(5)->setText(NOTIFICATIONS.NO);
        appConfig->changeBoolState(false);
    } else {
        qDebug() << "connect";
        connect(radioList, &RadioList::sendTitleToTray, this, &TrayIcon::handleTitleFromRadioList);
        setIsNotificationEnable(true);
        trayMenu->actions().at(5)->setText(NOTIFICATIONS.YES);
        appConfig->changeBoolState(true);
    }
}

void TrayIcon::loadTrayLists()
{
    int size = radioList->getJsonListProcessor()->getTableRows().size();

    topVector.resize(size);
    popularVector.resize(size);
    newRadioVector.resize(size);

    for (int i = 0; i < size; i++) {
        QString topStation = radioList->getAllTableRows().at(radioTable.TOP).at(i).station;
        QString popularStation = radioList->getAllTableRows().at(radioTable.POPULAR).at(i).station;
        QString newStation = radioList->getAllTableRows().at(radioTable.NEWRADIO).at(i).station;
        topVector[i] = top->addAction(topStation);
        popularVector[i] = popular->addAction(popularStation);
        newRadioVector[i] = newRadio->addAction(newStation);
    }
}

void TrayIcon::clearIcon()
{
    for (QAction *a : topVector)
        a->setIcon(QIcon());
    for (QAction *a : popularVector)
        a->setIcon(QIcon());
    for (QAction *a : newRadioVector)
        a->setIcon(QIcon());
}

QModelIndex TrayIcon::createTrayRadioLists(QAction *action)
{
    auto topIt = std::find_if(topVector.begin(), topVector.end(), [action](QAction *topAction) {
        return action == topAction;
    });
    auto popIt = std::find_if(popularVector.begin(),
                              popularVector.end(),
                              [action](QAction *popularAction) { return action == popularAction; });
    auto newIt = std::find_if(newRadioVector.begin(),
                              newRadioVector.end(),
                              [action](QAction *newAction) { return action == newAction; });
    int treeNumber = -1;

    if (topIt != topVector.end()) {
        treeNumber = radioTable.TOP;
    } else if (popIt != popularVector.end()) {
        treeNumber = radioTable.POPULAR;
    } else if (newIt != newRadioVector.end()) {
        treeNumber = radioTable.NEWRADIO;
    }

    if (treeNumber != -1) {
        QModelIndex libraryIndex = ui->treeView->model()->index(0, 0);
        QModelIndex treeIndex = ui->treeView->model()->index(treeNumber, 0, libraryIndex);
        ui->treeView->selectionModel()->clearSelection();
        qDebug() << "------------------> " << treeIndex.data().toString();
        ui->treeView->selectionModel()->select(treeIndex, QItemSelectionModel::Select);

        if (treeIndex.isValid()) {
            radioList->setIsTreeClicked(true);
            radioList->onTrayViewItemClicked(treeIndex);
        } else {
            qDebug() << "Wrong index for treeView.";
        }
    } else {
        qDebug() << "Element not found.";
    }

    int dis = 0;

    // Must rebuild...
    clearIcon();

    if (topIt != topVector.end()) {
        dis = std::distance(topVector.begin(), topIt);
        topVector[dis]->setIcon(QIcon(":/images/img/playing32.png"));
    }
    if (popIt != popularVector.end()) {
        dis = std::distance(popularVector.begin(), popIt);
        popularVector[dis]->setIcon(QIcon(":/images/img/playing32.png"));
    }
    if (newIt != newRadioVector.end()) {
        dis = std::distance(newRadioVector.begin(), newIt);
        newRadioVector[dis]->setIcon(QIcon(":/images/img/playing32.png"));
    }
    QModelIndex radioIndex = ui->tableView->model()->index(dis, 0);

    return radioIndex;
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
    } else if (action == turnOnOffNotification) {
        qDebug() << "Hello Notification";
        setNotifications(isNotificationEnabled);
    } else {
        QModelIndex radioIndex = createTrayRadioLists(action);

        radioList->onTrayClickedandPlay(radioIndex);
        radioList->setTrayRadioImage(radioIndex);
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
