#ifndef TRAYICON_H
#define TRAYICON_H
#include "radioaudiomanager.h"
#include "radiolist.h"
#include "ui_mainwindow.h"
#include <QSystemTrayIcon>
#include <QWidget>

class TrayIcon : public QWidget {
    Q_OBJECT
public:
    TrayIcon(Ui::MainWindow *ui, QMainWindow &mainWindow);
    ~TrayIcon();

    void setRadioAudioManager(RadioAudioManager *newRadioAudioManager);

    void setRadioList(RadioList *newRadioList);

    QSystemTrayIcon *getSystemTrayIcon() const;

    void setClearTimer();

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

    QSet<QString> recentTitles;
    QTimer *clearRecentTitlesTimer = nullptr;
    const int MAX_RECENT_TITLES = 2;
    const int DELAY_BETWEEN_NOTIFICATIONS = 5 * 60 * 1000;
    const int TRAY_TIME_MESSAGE = 5000;
};

#endif // TRAYICON_H
