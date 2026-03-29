#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <QDialog>
#include <QMainWindow>
#include <QResizeEvent>
#include "Country.h"
#include "include/Menu.h"
#include "include/music_data/clients/MusicBrainzClient.h"
#include "include/music_data/services/ResolverService.h"
#include "include/radioexplorer.h"
#include "include/radioinfo.h"
#include "include/radiolist.h"
#include "include/trayicon.h"
#include "view/TrackView.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    Ui::MainWindow *getUi() const;

private slots:
    void info();

private:
    Ui::MainWindow *ui;
    RadioExplorer *radioExplorer;
    RadioList *radioList;
    RadioInfo *radioInfo;
    Country &country = SingletonContainer::getSingleton().getInstance<Country>();
    QDialog *licenseDialog;
    QLabel *licenseLabel;
    QLabel *aboutTuneScape;
    QScrollArea *scrollArea;
    TrayIcon *trayIcon;
    Menu *appMenu;
    TrackView *trackView;

    music_data::MusicBrainzClient *m_musicBrainzCient;
    music_data::ResolverService *m_resolverService;
    music_data::CoverArtClient *m_coverArtClient;
    music_data::ExternalLinksClient *m_externalLinksClient;

    void set_icon_btn();
    void start();
    void resizeEvent(QResizeEvent *event);

    void infoDialogApp();
};
#endif // MAINWINDOW_H
