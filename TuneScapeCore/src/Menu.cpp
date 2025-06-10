#include "include/Menu.h"
#include "include/M3UHandler.h"
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
    exportRadioBrowserM3U = new QAction("Radio_Browser-M3U");
    exportRadioBrowser->setIcon(QIcon(":/images/img/playlist-27-32.png"));
    exportRadioBrowserM3U->setIcon(QIcon(":/images/img/playlist-27-32.png"));

    exportIceCastRadio = new QAction("IceCast");
    exportIceCastRadioM3U = new QAction("IceCast-M3U");
    exportIceCastRadio->setIcon(QIcon(":/images/img/playlist-27-32.png"));
    exportIceCastRadioM3U->setIcon(QIcon(":/images/img/playlist-27-32.png"));
    mainMenu->addAction(importPls);
    mainMenu->addMenu(exportPls);
    exportPls->addAction(exportRadioBrowser);
    exportPls->addAction(exportRadioBrowserM3U);
    exportPls->addAction(exportIceCastRadio);
    exportPls->addAction(exportIceCastRadioM3U);

    ui->menuButton->setMenu(mainMenu);
    ui->menuButton->setPopupMode(QToolButton::InstantPopup);

    connect(importPls, &QAction::triggered, this, &Menu::importPlaylists);
    connect(exportRadioBrowser, &QAction::triggered, this, &Menu::exportRadioBrowserPlaylist);
    connect(exportRadioBrowserM3U, &QAction::triggered, this, &Menu::exportRadioBrowserPlaylistM3U);
    connect(exportIceCastRadio, &QAction::triggered, this, &Menu::exortIceCastPlaylist);
    connect(exportIceCastRadioM3U, &QAction::triggered, this, &Menu::exportIceCastPlaylistM3U);
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
    } else if (fileName.contains(FORMAT_M3U)) {
        playlist = RADIO_BROWSER_M3U;
    }else {
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

    // For now only radiobrowser
    if (fileName.endsWith(".m3u", Qt::CaseInsensitive)) {
        inputFile.close();
        outputFile.close();
        M3UHandler m3uHandler;
        m3uHandler.importM3Ufile(fileName, RADIO_BROWSER);
    } else {
        out << in.readAll();
        inputFile.close();
        outputFile.close();
    }



    QMessageBox::information(nullptr, "Sukccess", "Playlist is imported successfully.");

    if (playlist == RADIO_BROWSER || playlist == RADIO_BROWSER_M3U) {
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
    exportRadio(RADIO_BROWSER, TUNSCAPE_FORMAT);
}

void Menu::exportRadioBrowserPlaylistM3U()
{
    generateM3Uplaylist(RADIO_BROWSER, RADIO_BROWSER_M3U);

}

void Menu::exportIceCastPlaylistM3U()
{
    generateM3Uplaylist(ICE_CAST, ICE_CAST_M3U);
}

void Menu::exortIceCastPlaylist()
{
    exportRadio(ICE_CAST, TUNSCAPE_FORMAT);
}

void Menu::exportRadio(const QString &playlist, QString format)
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
                                                        format == "m3u" ? "M3U Playlist (*.m3u)" : "Text file (*.txt);;All files (*.*)");
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

void Menu::generateM3Uplaylist(const QString &radio, const QString &radioM3u)
{
    M3UHandler m3uHandler;
    if (m3uHandler.exportM3Ufile(radio))
        exportRadio(radioM3u, M3U_FORMAT);
    else
        qDebug() << "M3U export failed for:" << radio;
}
