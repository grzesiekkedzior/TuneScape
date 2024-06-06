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

private slots:
    void iconActivated(QSystemTrayIcon::ActivationReason reason);
    void trayIconButtonClicked();
    void trayMenuClicked(QAction *action);
    void handleTitleFromRadioList(const QString &data);
    void clearRecentTitles();

private:
    Ui::MainWindow *ui;
    QSystemTrayIcon *systemTrayIcon;
    QMainWindow *mainWindow;
    QMenu *trayMenu;
    RadioAudioManager *radioAudioManager = nullptr;
    RadioList *radioList = nullptr;
    QAction *playPauseAction = nullptr;
    QAction *exitAction = nullptr;
    QAction *turnOnOffNotification = nullptr;
    AppConfig *appConfig = nullptr;
    bool isNotificationEnabled = true;

    QSet<QString> recentTitles;
    QTimer *clearRecentTitlesTimer = nullptr;
    const int MAX_RECENT_TITLES = 2;
    const int DELAY_BETWEEN_NOTIFICATIONS = 5 * 60 * 1000;
    const int TRAY_TIME_MESSAGE = 5000;
    void setNotifications(bool isNotificationEnabled);
};

#endif // TRAYICON_H
