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
    void exportPlaylists();

private:
    Ui::MainWindow *ui = nullptr;
    RadioList *radioList = nullptr;
    IceCastXmlData *iceCastXml = nullptr;

    QMenu *mainMenu = nullptr;
    QAction *importPls = nullptr;
    QAction *exportPls = nullptr;

    const QString RADIO_BROWSER = "radiobrowser.txt";
    const QString ICE_CAST = "icecast.txt";
};

#endif // MENU_H
