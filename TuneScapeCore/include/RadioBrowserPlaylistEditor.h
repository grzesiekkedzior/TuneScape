#ifndef RADIOBROWSERPLAYLISTEDITOR_H
#define RADIOBROWSERPLAYLISTEDITOR_H

#include "PlaylistEditor.h"
#include "include/FavoriteManager.h"
#include "radiolist.h"

class RadioBrowserPlaylistEditor : public PlaylistEditor {
    // PlaylistEditor interface
public:
    explicit RadioBrowserPlaylistEditor(RadioList &radiolist, FavoriteManager *favoriteManager);
    bool remove(const QModelIndex &index) override;
    bool updateFile(int radioPosition) override;
    void refreshUI() override;
    
    void checkIfStationIsPlaying(const QModelIndex &index);
    
private:
    RadioList& radioList;
    const QString RADIO_BROWSER = "radiobrowser.txt";
    const int FAVORITE_VECTOR = 3;
    int radioIndex = -1;
    FavoriteManager *favoriteManager = nullptr;
};

#endif // RADIOBROWSERPLAYLISTEDITOR_H
