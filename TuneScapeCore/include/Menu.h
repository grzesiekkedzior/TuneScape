#ifndef MENU_H
#define MENU_H
#include <QObject>
#include "ui_mainwindow.h"
#include "radiolist.h"

class Menu : public QObject
{
    Q_OBJECT
public:
    Menu(Ui::MainWindow *ui, RadioList *radioList, FavoriteManager *favoriteManager);

private slots:
    void importPlaylists();
    void exportRadioBrowserPlaylist();
    void exportRadioBrowserPlaylistM3U();

private:
    Ui::MainWindow *ui = nullptr;
    RadioList *radioList = nullptr;
    FavoriteManager *favoriteManager = nullptr;

    QMenu *mainMenu = nullptr;
    QMenu *exportPls = nullptr;
    QAction *importPls = nullptr;
    QAction *exportRadioBrowser = nullptr;
    QAction *exportRadioBrowserM3U = nullptr;

    void exportPlaylist(FavoriteManager::Format format);
};

#endif // MENU_H
