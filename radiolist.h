#ifndef RADIOLIST_H
#define RADIOLIST_H

#include "jsonlistprocessor.h"
#include "radioaudiomanager.h"
#include "ui_mainwindow.h"

#include <QObject>
#include <QStandardItemModel>
#include <QHeaderView>
#include <QStringList>

class RadioList : public QObject
{
    Q_OBJECT
public:
    explicit RadioList(QObject *parent = nullptr);
    RadioList(Ui::MainWindow *ui);

    void loadRadioList();
    void setLoadedStationsCount(int num);

private slots:
    void onTreeViewItemClicked(const QModelIndex &index);
    void loadMoreStationsIfNeeded();

    void onTableViewDoubleClicked(const QModelIndex &index);

private:
    int loadedStationsCount = 0;
    Ui::MainWindow     *ui      = nullptr;
    QStandardItemModel *model   = nullptr;
    QHeaderView        *header  = nullptr;
    QStringList         headers;
    QString             treeItem;
    JsonListProcessor   jsonListProcesor;
    RadioAudioManager   radioManager;

    const QString STATION   = "Station";
    const QString GENRE     = "Genre";
    const QString COUNTRY   = "Country";
    const QString HOMEPAGE  = "Homepage";
    const QString LIBRARY   = "Library";
    const QString FAVORITE  = "Favorite";

    const QString JSON_ENDPOINT_TOP         = "json/stations/topvote/5";
    const QString JSON_ENDPOINT_DISCOVER    = "json/stations";
    const QString JSON_ENDPOINT_POPULAR     = "json/stations/topclick/5";
    const QString JSON_ENDPOINT_NEW         = "json/stations/lastchange/5";
    const QString JSON_ENDPOINT_EMPTY       = "empty";
    const QString LIBRARY_TREE              = "Library";
    const QString FAVORITE_TREE             = "Favorite";


};

#endif // RADIOLIST_H
