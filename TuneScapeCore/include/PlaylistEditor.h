#ifndef PLAYLISTEDITOR_H
#define PLAYLISTEDITOR_H

#include <QAbstractItemModel>
class PlaylistEditor {
public:
    virtual ~PlaylistEditor() = default;

    virtual bool remove(const QModelIndex &index) = 0;
    virtual bool updateFile() = 0;
    virtual void refreshUI() = 0;
};

#endif // PLAYLISTEDITOR_H
