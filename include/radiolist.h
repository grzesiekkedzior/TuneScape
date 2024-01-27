#ifndef RADIOLIST_H
#define RADIOLIST_H

#include "../ui_mainwindow.h"
#include "IceCastXmlData.h"
#include "StreamReader.h"
#include "StreamRecorder.h"
#include "include/flowlayout.h"
#include "include/jsonlistprocessor.h"
#include "include/radioaudiomanager.h"
#include "include/radioinfo.h"

#include <QHeaderView>
#include <QObject>
#include <QStandardItemModel>
#include <QStringList>

enum Stations { TOP, POPULAR, NEW, FAVORITE, SEARCH, DISCOVERY };

class RadioList : public QObject
{
    Q_OBJECT
public:
    explicit RadioList(QObject *parent = nullptr);
    RadioList(Ui::MainWindow *ui);

    void loadRadioList();
    void loadAllData();
    void setLoadedStationsCount(int num);
    void getSongTitle(const QString &url);

    void setTypeMenu();
    void startSearchTimer();
    void onInternetConnectionRestored();

    void handleNetworkReply(
        QNetworkReply *reply, QPushButton *button, QWidget *itemContainer, int dataSize, int row);

    void clearIconLabelColor();
    void clearTableViewColor();
    void checkIsRadioOnPlaylist();
    bool isAddressExists(const QString address, const QString playlist);

    bool getIsPlaying() const;
    void setIsPlaying(bool newIsPlaying);

    bool getIsDarkMode() const;
    void setIsDarkMode(bool newIsDarkMode);

signals:
    void playIconButtonClicked(int radioNumber);
    void allIconsLoaded();
private slots:
    void onTreeViewItemClicked(const QModelIndex &index);
    void loadMoreStationsIfNeeded();
    void onTableViewDoubleClicked(const QModelIndex &index);
    void onPlayPauseButtonCliced();
    void onNextButtonClicked();
    void onPrevButtonClicked();
    void onStopButtonClicked();
    void onTableViewClicked(const QModelIndex &index);
    void tableViewActivated(const QModelIndex &index);
    void addRadioToFavorite();
    void onAllIconsLoaded();

private:
    int loadedStationsCount = -1;
    int currentPlaylistIndex = -1;
    int currentStationIndex = -1;
    int currentPlayListPlaying = -1;
    bool isPlaying = false;
    Ui::MainWindow *ui = nullptr;
    QStandardItemModel *model = nullptr;
    QHeaderView *header = nullptr;
    QStringList headers;
    QString treeItem;
    JsonListProcessor jsonListProcesor;
    RadioAudioManager radioManager;
    StreamReader streamReader;
    QSharedPointer<StreamRecorder> streamRecorder = QSharedPointer<StreamRecorder>::create(
        new StreamRecorder);
    IceCastXmlData *iceCastXmlData = nullptr;
    QTimer searchTimer;
    RadioInfo *radioInfo;
    FlowLayout *flowLayout = nullptr;
    QVector<QWidget *> buttonCache;

    QString currentRadioPlayingAddress = "";
    QString item = "";

    int radioIndexNumber = 0;
    int radioEnterIndexNumber = 0;
    int radioIndexCurrentPlaying = 0;
    int radioPlaylistCurrentPlaying = 0;
    int progressLoading = 1;
    bool isStopClicked = false;
    bool isTreeClicked = false;
    bool isSearching = false;
    bool isIconFlowlayoutFull = false;
    bool isDarkMode = false;

    const QString STATION = "Station";
    const QString GENRE = "Genre";
    const QString COUNTRY = "Country";
    const QString HOMEPAGE = "Homepage";
    const QString LIBRARY = "Library";
    const QString FAVORITE = "Favorite";

    const QString JSON_ENDPOINT_TOP = "json/stations/topvote/20";
    const QString JSON_ENDPOINT_POPULAR = "json/stations/topclick/20";
    const QString JSON_ENDPOINT_NEW = "json/stations/lastchange/20";
    const QString JSON_ENDPOINT_SEARCH = "json/stations/search?name=";
    const QString JSON_ENDPOINT_EMPTY = "empty";
    const QString LIBRARY_TREE = "Library";
    const QString FAVORITE_TREE = "Favorite";
    const QString FAVORITE_ICECAST = "IceCastFavorite";
    const QString ICECAST_PLAYLIST = "icecast.txt";

    QVector<QVector<TableRow>> allTableRows;
    QVector<QVector<QString>> allStreamAddresses;
    QVector<QVector<QString>> allIconsAddresses;

    void playStream(int radioNumber);
    void setIndexColor();
    void sliderMoved(int move);
    void setRadioImage(const QModelIndex &index);
    void handleDataReceived(const QString &data);
    void startStopRecord();
    void setMp3FileName();
    void searchStations();

    void clearRadioInfoLabel();
    void setVectorsOfStation(const QString endpoint);
    void setFavoriteStatons();
    void setTopListOnStart();

    // Add and remove the playlists!!!
    bool isRadioAdded(const QString data, const QString playlist);
    void removeRadio(const QString data, const QString playlist);

    void setRadioListVectors(Stations s);
    void clearFlowLayout();
    void setImageButton(int row);
    void loadAndSetImageForItem(const QString &imageUrl,
                                QList<QStandardItem *> &rowItems,
                                const QModelIndex &nameIndex,
                                const QModelIndex &countryIndex);
    void addIconButton(int row);
    void populateRowItems(QList<QStandardItem *> rowItems, int row);
    void createButton(int row,
                      QWidget *itemContainer,
                      QList<QStandardItem *> rowItems,
                      QVBoxLayout *itemLayout);
    QWidget *createItemContainer(QPushButton *button, int row);
    QPushButton *createButton(int row);
    void addToButtonCache();
    void loadRadioIconList();
    void clearAll();
    void handleIconPlayButtonDoubleClick(int radioNumber);
    QList<QNetworkReply *> networkReplies;
    QNetworkAccessManager *networkManager = nullptr;
    void markIconPlayingStation(int radioNumber);
    void clearInfoData(int stationIndex);
    void setDarkMode();
    void isDark();
};

#endif // RADIOLIST_H
