#ifndef RADIOBROWSERPLAYLISTEDITOR_H
#define RADIOBROWSERPLAYLISTEDITOR_H

#include "PlaylistEditor.h"
#include "radiolist.h"

class RadioBrowserPlaylistEditor : public PlaylistEditor {
    // PlaylistEditor interface
public:
    explicit RadioBrowserPlaylistEditor(RadioList& radiolist);
    bool remove(const QModelIndex &index) override;
    bool updateFile() override;
    void refreshUI() override;

private:
    RadioList& radioList;
};

#endif // RADIOBROWSERPLAYLISTEDITOR_H
