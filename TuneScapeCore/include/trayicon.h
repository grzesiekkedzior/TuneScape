#ifndef TRAYICON_H
#define TRAYICON_H
#include <QSystemTrayIcon>
#include <QWidget>
#include "../ui_mainwindow.h"
#include "AppConfig.h"
#include "radioaudiomanager.h"
#include "radiolist.h"

struct Notifications
{
    const QString NO = "OFF";
    const QString YES = "ON";
};

struct RadioTable
{
    const int TOP = 0;
    const int POPULAR = 1;
    const int NEWRADIO = 2;
};

struct RadioPlaylistName
{
    const QString TOP = "Top";
    const QString POPULAR = "Popular";
    const QString NEW = "New";
    const QString FAVORITE = "Favorite";
};

class TrayIcon : public QWidget
{
    Q_OBJECT
public:
    TrayIcon(Ui::MainWindow *ui, QMainWindow &mainWindow);
    ~TrayIcon();

    void setRadioAudioManager(RadioAudioManager *newRadioAudioManager);

    void setRadioList(RadioList *newRadioList);

    QSystemTrayIcon *getSystemTrayIcon() const;

    void setClearTimer();

    bool getIsNotificationEnable() const;
    void setIsNotificationEnable(bool newIsNotificationEnable);
    void loadTrayLists();
    void clearIcon();

private slots:
    void iconActivated(QSystemTrayIcon::ActivationReason reason);
    void trayIconButtonClicked();
    void trayMenuClicked(QAction *action);
    void handleTitleFromRadioList(const QString &data);
    void clearRecentTitles();

private:
    Ui::MainWindow *ui;
    QSystemTrayIcon *systemTrayIcon = nullptr;
    QMainWindow *mainWindow = nullptr;
    // Tray menu
    QMenu *trayMenu = nullptr;
    QMenu *top = nullptr;
    QMenu *popular = nullptr;
    QMenu *newRadio = nullptr;

    RadioTable radioTable;
    RadioPlaylistName radioPlaylist;

    RadioAudioManager *radioAudioManager = nullptr;
    RadioList *radioList = nullptr;
    QAction *playPauseAction = nullptr;
    QAction *exitAction = nullptr;
    QAction *turnOnOffNotification = nullptr;
    QAction *topStations = nullptr;

    QVector<QAction *> topVector;
    QVector<QAction *> popularVector;
    QVector<QAction *> newRadioVector;

    AppConfig *appConfig = nullptr;
    bool isNotificationEnabled = true;

    QSet<QString> recentTitles;
    QTimer *clearRecentTitlesTimer = nullptr;
    const int MAX_RECENT_TITLES = 2;
    const int DELAY_BETWEEN_NOTIFICATIONS = 5 * 60 * 1000;
    const int TRAY_TIME_MESSAGE = 5000;
    const QString NOTIFICATIONS_PROPERTY = "notifications";
    const QString RADIO_ICON = ":/images/img/radio72x72.png";
    void setNotifications(bool isNotificationEnabled);
    QModelIndex createTrayRadioLists(QAction *action);

    void setTrayListIcon();
};

#endif // TRAYICON_H
