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
#include <QPointer>
#include <functional>
class QScreen;

#include "ui_mainwindow.h"
#include "AppConfig.h"
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
    explicit TrayIcon(Ui::MainWindow *ui, QMainWindow &mainWindow, std::function<bool()> trayAvailable = {});
    ~TrayIcon() override;

    void setRadioList(RadioList *newRadioList);

    QSystemTrayIcon *getSystemTrayIcon() const;

    bool getIsNotificationEnable() const;
    void setIsNotificationEnable(bool newIsNotificationEnable);

    void loadTrayList(Stations station);
    void prepareForQuit();

signals:
    void quitRequested();

private slots:
    void iconActivated(QSystemTrayIcon::ActivationReason reason);
    void trayIconButtonClicked();
    void trayMenuClicked(QAction *action);
    void handleTitleFromRadioList(const QString &data);
    void clearRecentTitles();
    void restoreMainWindow();
    void checkTrayAvailability();

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
    void updatePlayPauseAction();
    void watchScreen(QScreen *screen);
    void updatePanelGeometry();

    struct StationAction {
        RadioStation station;
        Stations playlist;
    };

    QPointer<QLabel> stationImage;
    QPointer<QLabel> metadataText;
    QSystemTrayIcon *systemTrayIcon = nullptr;
    QPointer<QMainWindow> mainWindow;

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

    QPointer<PlaybackController> playback;
    QPointer<RadioList> radioList;
    QHash<QAction *, StationAction> stationActions;

    QAction *playPauseAction = nullptr;
    QAction *exitAction = nullptr;
    QAction *restoreAction = nullptr;
    QAction *turnOnOffNotification = nullptr;
    QAction *keepNowPlayingVisibleAction = nullptr;

    QVector<QAction *> topVector;
    QVector<QAction *> popularVector;
    QVector<QAction *> newRadioVector;

    QVector<QIcon> topIcons;
    QVector<QIcon> popularIcons;
    QVector<QIcon> newRadioIcons;


    AppConfig *appConfig = nullptr;
    bool isNotificationEnabled = true;
    bool keepNowPlayingVisible = false;
    bool m_shuttingDown = false;
    bool m_hiddenToTray = false;
    std::function<bool()> m_trayAvailable;
    QTimer m_trayWatch;

    QSet<QString> recentTitles;
    QTimer *clearRecentTitlesTimer = nullptr;

    static constexpr int TrayTimeMessage = 5000;

    const QString NotificationsProperty = QStringLiteral("notifications");
    const QString RadioIcon = QStringLiteral(":/images/img/radio96x96.png");
};

#endif // TRAYICON_H
