#ifndef RADIOBROWSERPLAYLISTEDITOR_H
#define RADIOBROWSERPLAYLISTEDITOR_H

#include "PlaylistEditor.h"
#include "radiolist.h"

class RadioBrowserPlaylistEditor : public PlaylistEditor {
    // PlaylistEditor interface
public:
    explicit RadioBrowserPlaylistEditor(RadioList& radiolist);
    bool remove(const QModelIndex &index) override;
    bool updateFile(int radioPosition) override;
    void refreshUI() override;

private:
    RadioList& radioList;
    const QString RADIO_BROWSER = "radiobrowser.txt";
    const int FAVORITE_VECTOR = 3;
    int radioIndex = -1;
};

#endif // RADIOBROWSERPLAYLISTEDITOR_H
