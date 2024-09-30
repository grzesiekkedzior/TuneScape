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
    exportPls = new QAction("export pls");
    exportPls->setIcon(QIcon(":/images/img/upload-20-32.png"));
    mainMenu->addAction(importPls);
    mainMenu->addAction(exportPls);
    ui->menuButton->setMenu(mainMenu);
    ui->menuButton->setPopupMode(QToolButton::InstantPopup);

    connect(importPls, &QAction::triggered, this, &Menu::importPlaylists);
    connect(exportPls, &QAction::triggered, this, &Menu::exportPlaylists);
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
        qDebug() << "???????????????????????????????";
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

void Menu::exportPlaylists() {}
