#include "include/Menu.h"

#include <QFileDialog>
#include <QMenu>
#include <QMessageBox>
#include <utility>

Menu::Menu(Ui::MainWindow *ui, RadioList *radioList, FavoriteManager *favoriteManager)
    : ui(ui)
    , radioList(radioList)
    , favoriteManager(favoriteManager)
{
    mainMenu = new QMenu("Menu");
    importPls = new QAction("import pls");
    importPls->setIcon(QIcon(":/images/img/download-20-32.png"));
    exportPls = new QMenu("export pls");
    exportPls->setIcon(QIcon(":/images/img/upload-20-32.png"));

    exportRadioBrowser = new QAction("Radio-Browser");
    exportRadioBrowserM3U = new QAction("Radio_Browser-M3U");
    exportRadioBrowser->setIcon(QIcon(":/images/img/playlist-27-32.png"));
    exportRadioBrowserM3U->setIcon(QIcon(":/images/img/playlist-27-32.png"));

    mainMenu->addAction(importPls);
    mainMenu->addMenu(exportPls);
    exportPls->addAction(exportRadioBrowser);
    exportPls->addAction(exportRadioBrowserM3U);

    ui->menuButton->setMenu(mainMenu);
    ui->menuButton->setPopupMode(QToolButton::InstantPopup);

    connect(importPls, &QAction::triggered, this, &Menu::importPlaylists);
    connect(exportRadioBrowser, &QAction::triggered, this, &Menu::exportRadioBrowserPlaylist);
    connect(exportRadioBrowserM3U, &QAction::triggered, this, &Menu::exportRadioBrowserPlaylistM3U);
}

void Menu::importPlaylists()
{
    auto *parent = ui->menuButton->window();
    const QString source = QFileDialog::getOpenFileName(
        parent, tr("Import Favorites"), {},
        tr("Playlists (*.txt *.m3u *.m3u8);;All files (*)"));
    if (source.isEmpty())
        return;

    QVector<RadioStation> stations;
    const QString error = favoriteManager->importPlaylist(source, stations);
    if (!error.isEmpty()) {
        QMessageBox::warning(parent, tr("Cannot import Favorites"), error);
        return;
    }

    radioList->applyFavoritePlaylist(std::move(stations));
    radioList->setFavoriteLibrary();
    QMessageBox::information(parent, tr("Import complete"), tr("The Favorites playlist was replaced."));
}

void Menu::exportRadioBrowserPlaylist()
{
    exportPlaylist(FavoriteManager::Format::Native);
}

void Menu::exportRadioBrowserPlaylistM3U()
{
    exportPlaylist(FavoriteManager::Format::M3U);
}

void Menu::exportPlaylist(FavoriteManager::Format format)
{
    auto *parent = ui->menuButton->window();
    const bool m3u = format == FavoriteManager::Format::M3U;
    const QString extension = m3u ? QStringLiteral(".m3u") : QStringLiteral(".txt");
    QFileDialog dialog(parent, tr("Export Favorites"), QStringLiteral("radiobrowser") + extension,
                       m3u ? tr("M3U playlists (*.m3u *.m3u8)") : tr("TuneScape playlists (*.txt)"));
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setDefaultSuffix(extension.mid(1));
    if (dialog.exec() != QDialog::Accepted)
        return;

    const QString error = favoriteManager->exportPlaylist(dialog.selectedFiles().value(0), format);
    if (!error.isEmpty()) {
        QMessageBox::warning(parent, tr("Cannot export Favorites"), error);
        return;
    }
    QMessageBox::information(parent, tr("Export complete"), tr("The Favorites playlist was saved."));
}
