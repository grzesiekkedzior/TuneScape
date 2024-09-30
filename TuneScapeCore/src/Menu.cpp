#include "include/Menu.h"
#include <QFileDialog>
#include <QMenu>
#include <QMessageBox>

Menu::Menu(Ui::MainWindow *ui, RadioList *radioList, IceCastXmlData *iceCastXmlData)
    : ui(ui)
    , radioList(radioList)
    , iceCastXmlData(iceCastXmlData)

{
    mainMenu = new QMenu("Menu");
    importPls = new QAction("import pls");
    importPls->setIcon(QIcon(":/images/img/download-20-32.png"));
    exportPls = new QMenu("export pls");
    exportPls->setIcon(QIcon(":/images/img/upload-20-32.png"));
    exportRadioBrowser = new QAction("Radio-Browser");
    exportRadioBrowser->setIcon(QIcon(":/images/img/playlist-27-32.png"));
    exportIceCastRadio = new QAction("IceCast");
    exportIceCastRadio->setIcon(QIcon(":/images/img/playlist-27-32.png"));
    mainMenu->addAction(importPls);
    mainMenu->addMenu(exportPls);
    exportPls->addAction(exportRadioBrowser);
    exportPls->addAction(exportIceCastRadio);
    ui->menuButton->setMenu(mainMenu);
    ui->menuButton->setPopupMode(QToolButton::InstantPopup);

    connect(importPls, &QAction::triggered, this, &Menu::importPlaylists);
    connect(exportRadioBrowser, &QAction::triggered, this, &Menu::exportRadioBrowserPlaylist);
    connect(exportIceCastRadio, &QAction::triggered, this, &Menu::exortIceCastPlaylist);
}

void Menu::importPlaylists()
{
    QString fileName = QFileDialog::getOpenFileName(nullptr,
                                                    "Choose the playlist file.",
                                                    "",
                                                    "All Files (*)");

    QString playlist{};

    if (fileName.contains(RADIO_BROWSER)) {
        playlist = RADIO_BROWSER;
    } else if (fileName.contains(ICE_CAST)) {
        playlist = ICE_CAST;
    } else {
        QMessageBox::warning(nullptr, "Error", "This is not TuneScape playlist!");
        return;
    }

    if (fileName.isEmpty()) {
        QMessageBox::warning(nullptr, "Error", "You don't choose any file.");
        return;
    }

    QFile inputFile(fileName);
    if (!inputFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(nullptr, "Error", "Cannot open file.");
        return;
    }

    QFile outputFile(playlist);
    if (!outputFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(nullptr, "Error", "Cannot open file to save.");
        return;
    }

    QTextStream in(&inputFile);
    QTextStream out(&outputFile);

    out << in.readAll();

    inputFile.close();
    outputFile.close();

    QMessageBox::information(nullptr, "Sukccess", "Playlist is imported successfully.");

    if (playlist == RADIO_BROWSER) {
        radioList->setFavoriteStatons();
        radioList->setFavoriteLibrary();
    }
    if (playlist == ICE_CAST) {
        if (iceCastXmlData->getIsFavoriteOnTreeCliced())
            iceCastXmlData->loadFavoriteIceCastStations();
        iceCastXmlData->setIndexColor(iceCastXmlData->getIndexPlayingStation());
        iceCastXmlData->setFavoriteStations();
        iceCastXmlData->loadFavoriteIceCastStations();
    }
}

void Menu::exportRadioBrowserPlaylist()
{
    exportRadio(RADIO_BROWSER);
}

void Menu::exortIceCastPlaylist()
{
    exportRadio(ICE_CAST);
}

void Menu::exportRadio(const QString &playlist)
{
    QFile inputFile(playlist);
    if (!inputFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(nullptr, "Error", "Cannot open file.");
        return;
    }

    QTextStream in(&inputFile);
    QString fileContent = in.readAll();
    inputFile.close();

    QString saveFilePath = QFileDialog::getSaveFileName(nullptr,
                                                        "Save playlist",
                                                        playlist,
                                                        "Text file (*.txt);;All files (*.*)");
    if (saveFilePath.isEmpty()) {
        return;
    }

    QFile outputFile(saveFilePath);
    if (!outputFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(nullptr, "Error", "Cannot open file for writing.");
        return;
    }

    QTextStream out(&outputFile);
    out << fileContent;
    outputFile.close();

    QMessageBox::information(nullptr, "Sukccess", "Playlist is saved correctly.");
}
