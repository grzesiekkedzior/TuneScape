#include "include/RadioBrowserPlaylistEditor.h"

RadioBrowserPlaylistEditor::RadioBrowserPlaylistEditor(RadioList &radiolist) : radioList {radiolist}
{

}

bool RadioBrowserPlaylistEditor::remove(const QModelIndex &index)
{
    radioIndex = index.row();
    updateFile(index.row());
    refreshUI();
    return true;
}


bool RadioBrowserPlaylistEditor::updateFile(int radioPosition)
{
    if (radioPosition < 0 || radioPosition >= radioList.getAllTableRows().at(FAVORITE_VECTOR).size()) {
        return false;
    }
    auto favoriteVector = radioList.getAllTableRows().at(FAVORITE_VECTOR);
    QString stationName = favoriteVector.at(radioPosition).station;
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
}
