#ifndef MENU_H
#define MENU_H
#include <QObject>
#include "../ui_mainwindow.h"
#include "radiolist.h"

class Menu : public QObject
{
    Q_OBJECT
public:
    Menu(Ui::MainWindow *ui, RadioList *radioList, IceCastXmlData *iceCastXml);

private slots:
    void importPlaylists();
    void exportRadioBrowserPlaylist();
    void exortIceCastPlaylist();

private:
    Ui::MainWindow *ui = nullptr;
    RadioList *radioList = nullptr;
    IceCastXmlData *iceCastXmlData = nullptr;

    QMenu *mainMenu = nullptr;
    QMenu *exportPls = nullptr;
    QAction *importPls = nullptr;
    QAction *exportRadioBrowser = nullptr;
    QAction *exportIceCastRadio = nullptr;

    const QString RADIO_BROWSER = "radiobrowser.txt";
    const QString ICE_CAST = "icecast.txt";

    void exportRadio(const QString &playlist);
};

#endif // MENU_H
