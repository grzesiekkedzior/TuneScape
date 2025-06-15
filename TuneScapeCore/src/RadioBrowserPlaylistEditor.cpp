#include "include/RadioBrowserPlaylistEditor.h"

RadioBrowserPlaylistEditor::RadioBrowserPlaylistEditor(RadioList &radiolist) : radioList {radiolist}
{

}

void RadioBrowserPlaylistEditor::checkIfStationIsPlaying(const QModelIndex &index)
{
    if (radioList.getRadioIndexNumber() == index.row())
        radioList.onStopButtonClicked();
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
    if (radioPosition < 0 || radioPosition >= radioList.getAllTableRows().at(FAVORITE_VECTOR).size()) {
        return false;
    }
    const auto& favoriteVector = radioList.getAllTableRows().at(FAVORITE_VECTOR);
    QString stationName = favoriteVector.at(radioPosition).station;
    if (radioIndex > radioPosition) {
        radioIndex -= 1;
    }

    radioList.setRadioIndexNumber(radioIndex);
    radioList.removeRadio(stationName, RADIO_BROWSER);
    return true;
}

void RadioBrowserPlaylistEditor::refreshUI()
{
    radioList.setFavoriteStatons();
    radioList.getModel()->clear();
    radioList.getJsonListProcessor()->setTableRows(radioList.getAllTableRows().at(FAVORITE_VECTOR));
    radioList.getJsonListProcessor()->setStreamAddresses(radioList.getAllStreamAddresses().at(FAVORITE_VECTOR));
    radioList.getJsonListProcessor()->setIconAddresses(radioList.getAllIconsAddresses().at(FAVORITE_VECTOR));

    radioList.loadRadioList();
    radioList.loadRadioIconList();
    radioList.updateStationColoring();

}
