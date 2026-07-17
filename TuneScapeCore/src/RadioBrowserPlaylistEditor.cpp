#include "include/RadioBrowserPlaylistEditor.h"

RadioBrowserPlaylistEditor::RadioBrowserPlaylistEditor(RadioList &radiolist,
                                                       FavoriteManager *favoriteManager)
    : radioList{radiolist}
    , favoriteManager{favoriteManager}
{}

void RadioBrowserPlaylistEditor::checkIfStationIsPlaying(const QModelIndex &index)
{
    if (radioList.getRadioIndexNumber() == index.row()) {
        radioList.onStopButtonClicked();
        radioIndex = 0;
    }
}

bool RadioBrowserPlaylistEditor::remove(const QModelIndex &index)
{
    radioIndex = radioList.getRadioIndexNumber();
    checkIfStationIsPlaying(index);
    updateFile(index.row());
    refreshUI();

    return true;
}

bool RadioBrowserPlaylistEditor::updateFile(int radioPosition)
{
    const auto &favoriteStations = radioList.stations(Stations::FAVORITE);

    if (radioPosition < 0 || radioPosition >= favoriteStations.size()) {
        return false;
    }

    QString stationName = favoriteStations.at(radioPosition).station;

    if (radioIndex > radioPosition) {
        --radioIndex;
    }

    radioList.setRadioIndexNumber(radioIndex);
    favoriteManager->removeRadio(stationName, RADIO_BROWSER);

    return true;
}

void RadioBrowserPlaylistEditor::refreshUI()
{
    radioList.setFavoriteStatons();
    radioList.loadRadioIconList();
    radioList.updateStationColoring();
}
