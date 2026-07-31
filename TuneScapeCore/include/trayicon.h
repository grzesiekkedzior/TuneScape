#ifndef TRAYICON_H
#define TRAYICON_H

#include <QIcon>
#include <QLabel>
#include <QPushButton>
#include <QSet>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QVector>
#include <QWidget>

#include "../ui_mainwindow.h"
#include "AppConfig.h"
#include "IconLoader.h"
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
    explicit TrayIcon(Ui::MainWindow *ui, QMainWindow &mainWindow);
    ~TrayIcon() override;

    void setRadioAudioManager(RadioAudioManager *newRadioAudioManager);
    void setRadioList(RadioList *newRadioList);

    QSystemTrayIcon *getSystemTrayIcon() const;

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
    void setClearTimer();
    void setNotifications(bool isNotificationEnabled);

    void createNowPlayingPanel();
    void showNowPlayingPanel();
    void hideNowPlayingPanel();
    void positionNowPlayingPanel();
    void positionPinnedNowPlayingPanel();
    void setNowPlayingPinned(bool pinned);
    void updateNowPlayingPanelStyle();
    void updateNowPlayingPanel();
    void updateNowPlayingButton();
    void clearNowPlayingPanel();

    void setTrayListIcon();
    void restoreStationIcons();
    void clearStationHighlights();
    void loadTrayStationIcons();
    void updatePlayPauseAction();

    QIcon currentStationIcon() const;
    QString currentStationName() const;

    QIcon stationTrayIcon(const RadioStation &station) const;
    QModelIndex createTrayRadioLists(QAction *action);

    Ui::MainWindow *ui = nullptr;
    QSystemTrayIcon *systemTrayIcon = nullptr;
    QMainWindow *mainWindow = nullptr;

    QMenu *trayMenu = nullptr;
    QMenu *top = nullptr;
    QMenu *popular = nullptr;
    QMenu *newRadio = nullptr;

    QWidget *nowPlayingWidget = nullptr;
    QWidget *nowPlayingFrame = nullptr;
    QLabel *nowPlayingIcon = nullptr;
    QLabel *nowPlayingStation = nullptr;
    QLabel *nowPlayingTitle = nullptr;
    QLabel *nowPlayingStatus = nullptr;
    QPushButton *nowPlayingPlayPauseButton = nullptr;

    RadioTable radioTable;
    RadioPlaylistName radioPlaylist;

    PlaybackController &playbackController = SingletonContainer::getSingleton()
                                                 .getInstance<PlaybackController>();

    RadioList *radioList = nullptr;

    QAction *playPauseAction = nullptr;
    QAction *exitAction = nullptr;
    QAction *turnOnOffNotification = nullptr;
    QAction *keepNowPlayingVisibleAction = nullptr;

    QVector<QAction *> topVector;
    QVector<QAction *> popularVector;
    QVector<QAction *> newRadioVector;

    QVector<QIcon> topIcons;
    QVector<QIcon> popularIcons;
    QVector<QIcon> newRadioIcons;

    IconLoader topIconLoader;
    IconLoader popularIconLoader;
    IconLoader newIconLoader;

    AppConfig *appConfig = nullptr;
    bool isNotificationEnabled = true;
    bool keepNowPlayingVisible = false;

    QString currentTrackTitle;

    QSet<QString> recentTitles;
    QTimer *clearRecentTitlesTimer = nullptr;

    static constexpr int TrayTimeMessage = 5000;

    const QString NotificationsProperty = QStringLiteral("notifications");
    const QString RadioIcon = QStringLiteral(":/images/img/radio96x96.png");
};

#endif // TRAYICON_H