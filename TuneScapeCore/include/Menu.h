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
    void exportRadioBrowserPlaylistM3U();
    void exortIceCastPlaylist();
    void exportIceCastPlaylistM3U();

private:
    Ui::MainWindow *ui = nullptr;
    RadioList *radioList = nullptr;
    IceCastXmlData *iceCastXmlData = nullptr;

    QMenu *mainMenu = nullptr;
    QMenu *exportPls = nullptr;
    QAction *importPls = nullptr;
    QAction *exportRadioBrowser = nullptr;
    QAction *exportRadioBrowserM3U = nullptr;
    QAction *exportIceCastRadio = nullptr;
    QAction *exportIceCastRadioM3U = nullptr;

    const QString RADIO_BROWSER = "radiobrowser.txt";
    const QString RADIO_BROWSER_M3U = "radiobrowser.m3u";
    const QString FORMAT_M3U = ".m3u";
    const QString ICE_CAST = "icecast.txt";
    const QString ICE_CAST_M3U = "icecast.m3u";

    const QString TUNSCAPE_FORMAT = "txt";
    const QString M3U_FORMAT = "m3u";

    void exportRadio(const QString &playlist, QString format);
};

#endif // MENU_H
