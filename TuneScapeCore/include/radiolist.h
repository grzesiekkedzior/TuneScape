#ifndef RADIOLIST_H
#define RADIOLIST_H

#include <QMessageBox>
#include "../ui_mainwindow.h"
#include "Country.h"
#include "FavoriteManager.h"
#include "PlaylistEditor.h"
#include "audioprocessor.h"
#include "container.h"
#include "controller/PlaybackController.h"
#include "include/IconLoader.h"
#include "include/RadioImageManager.h"
#include "include/customcolordelegate.h"
#include "include/flowlayout.h"
#include "include/jsonlistprocessor.h"
#include "include/radioaudiomanager.h"
#include "include/radioinfo.h"
#include "model/RadioStationsModel.h"
#include "streamreader.h"
#include "streamrecorder.h"

#include <QHeaderView>
#include <QObject>
#include <QPixmap>
#include <QStandardItemModel>
#include <QStringList>

#include "miniplayer.h"

enum Stations { TOP, POPULAR, NEW, FAVORITE, SEARCH, DISCOVERY };

class RadioList : public QObject
{
    Q_OBJECT
public:
    explicit RadioList(QObject *parent = nullptr);
    RadioList(Ui::MainWindow *ui, FavoriteManager *favoriteManager);

    void loadAllData();
    void setLoadedStationsCount(int num);
    void getSongTitle(const QString &url);

    void setTypeMenu();
    void startSearchTimer();
    void onInternetConnectionRestored();

    void clearIconLabelColor();
    void clearTableViewColor();
    void checkIsRadioOnPlaylist(const QString &station);
    bool isAddressExists(const QString address, const QString playlist);
    void setFavoriteStatons();

    bool getIsDarkMode() const;
    void setIsDarkMode(bool newIsDarkMode);
    void onPlayPauseButtonCliced();
    RadioAudioManager &getRadioManager();

    void addEmptyIconButton(int row);

    QSharedPointer<StreamRecorder> getStreamRecorder() const;

    bool getIsBrowseStationLoaded() const;
    void setIsBrowseStationLoaded(bool newIsBrowseStationLoaded);

    JsonListProcessor *getJsonListProcessor();

    void onTrayViewItemClicked(const QModelIndex &index);
    void onTrayClickedandPlay(const QModelIndex &index);
    void setTrayRadioImage(const QModelIndex &index);

    void setIsTreeClicked(bool newIsTreeClicked);

    QString getItem() const;

    int getRadioIndexNumber() const;

    QString getTreeItem() const;

    bool getIsSearchTablelDoubleCliced() const;
    void setIsSearchTablelDoubleCliced(bool newIsSearchTablelDoubleCliced);

    QMainWindow *getMainWindow() const;
    void setMainWindow(QMainWindow *newMainWindow);
    void setFavoriteLibrary();

    RadioInfo *getRadioInfo() const;

    bool getIsStopClicked() const;
    void setIsStopClicked(bool newIsStopClicked);

    void resetTreeItemIfSearch();

    QScopedPointer<PlaylistEditor> playlistEditor;

    // Add and remove the playlists!!!
    void loadRadioIconList();
    void updateStationColoring();

    Ui::MainWindow *getUi() const;

    void setRadioIndexNumber(int newRadioIndexNumber);

    int getCurrentStationIndex() const;
    void setCurrentStationIndex(int newCurrentStationIndex);
    //new model
    QVector<QVector<RadioStation>> allStations;
    RadioStationsModel *radioStationsModel;

    const QVector<RadioStation> &stations(Stations station) const;
    bool isIceCastFavoriteMode() const;
    void handleIceCastFavorite();
    void handleRadioBrowserFavorite();
    void handleCountryFavorite();

    void updateFavoriteIcon(bool isFavorite);

signals:
    void playIconButtonDoubleClicked(int radioNumber);
    void allIconsLoaded();
    void sendTitleToTray(QString title);
    void trackTitleReceived(const QString &title);
public slots:
    void onStopButtonClicked();

private slots:
    void onTreeViewItemClicked(const QModelIndex &index);
    void onTableViewDoubleClicked(const QModelIndex &index);
    void onNextButtonClicked();
    void onPrevButtonClicked();
    void onTableViewClicked(const QModelIndex &index);
    void tableViewActivated(const QModelIndex &index);
    void addRadioToFavorite();
    void onAllIconsLoaded();

private:
    int loadedStationsCount = -1;
    int currentPlaylistIndex = -1;
    int currentStationIndex = -1;
    int currentPlayListPlaying = -1;
    Ui::MainWindow *ui = nullptr;
    QHeaderView *header = nullptr;
    QStringList headers;
    QString treeItem;
    JsonListProcessor jsonListProcesor;

    StreamReader streamReader;
    QSharedPointer<StreamRecorder> streamRecorder = QSharedPointer<StreamRecorder>::create(
        new StreamRecorder);
    QTimer searchTimer;
    RadioInfo *radioInfo;
    FlowLayout *flowLayout = nullptr;

    QString currentRadioPlayingAddress = "";
    QString item = "";

    QMessageBox message;
    QMainWindow *mainWindow = nullptr;

    IconLoader *iconLoader;
    RadioImageManager *imageManager = nullptr;
    FavoriteManager *favoriteManager = nullptr;

    //Singletons
    AudioProcessor &audioProcessor = SingletonContainer::getSingleton().getInstance<AudioProcessor>();
    miniplayer &miniPlayer = SingletonContainer::getSingleton().getInstance<miniplayer>();
    Country &country = SingletonContainer::getSingleton().getInstance<Country>();
    PlaybackController &playbackController = SingletonContainer::getSingleton()
                                                 .getInstance<PlaybackController>();

    RadioStation currentPlayingStation;

    int radioIndexNumber = 0;
    int radioEnterIndexNumber = 0;
    int radioIndexCurrentPlaying = 0;
    int radioPlaylistCurrentPlaying = 0;
    int progressLoading = 1;
    bool isStopClicked = true;
    bool isTreeClicked = false;
    bool isSearching = false;
    bool isIconFlowlayoutFull = false;
    bool isDarkMode = false;
    bool isBrowseStationLoaded = false;
    bool isSearchTablelDoubleCliced = false;

    const QString STATION = "Station";
    const QString GENRE = "Genre";
    const QString COUNTRY = "Country";
    const QString HOMEPAGE = "Homepage";
    const QString LIBRARY = "Library";
    const QString FAVORITE = "Favorite";
    const QString SEARCH = "Search";

    const QString JSON_ENDPOINT_TOP = "json/stations/topvote/20";
    const QString JSON_ENDPOINT_POPULAR = "json/stations/topclick/20";
    const QString JSON_ENDPOINT_NEW = "json/stations/lastchange/20";
    const QString JSON_ENDPOINT_SEARCH = "json/stations/search?name=";
    const QString JSON_ENDPOINT_EMPTY = "empty";
    const QString LIBRARY_TREE = "Library";
    const QString FAVORITE_TREE = "Favorite";
    const QString FAVORITE_ICECAST = "IceCastFavorite";
    const QString ICECAST_PLAYLIST = "icecast.txt";
    const QString RADIO_BROWSER_PLAYLIST = "radiobrowser.txt";

    const QString RADIO_ICON = ":/images/img/radio96x96.png";
    const QString TUNESCAPE_ICON = ":/images/img/TuneScape.ico";

    QVector<QVector<TableRow>> allTableRows;
    QVector<QVector<QString>> allStreamAddresses;
    QVector<QVector<QString>> allIconsAddresses;

    void playSelectedStation(int radioNumber);
    void setIndexColor();
    void sliderMoved(int move);
    void setRadioImage(const QModelIndex &index);
    void handleDataReceived(const QString &data);
    void startStopRecord();
    void setMp3FileName();
    void searchStations();

    void clearRadioInfoLabel();
    void setVectorsOfStation(const QString &endpoint, Stations station);
    void setTopListOnStart();

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

    void clearAll();
    void handleIconPlayButtonDoubleClick(int radioNumber);
    void markIconPlayingStation(int radioNumber);
    void clearInfoData(int stationIndex);
    void setDarkMode();
    void isDark();
    void setRawRadioImage();
    void setRawDarkRadioImage();
    void showMiniplayer();
    void maximizeWindow();
    QString getCurrentStreamUrl() const;
    bool shouldUpdateIcon() const;
    void handleIconUpdate();
    void updateThemeAppearance(bool darkMode);

    QScopedPointer<CustomColorDelegate> customColor{nullptr};
    void handleIconClick(int row);
    void updateLayoutOrProgress();
    void readFavoriteStationsFromFile(QVector<RadioStation> &stations);
    void switchToPlaylist(Stations station);
    void loadRadioIconsFromNetwork(int dataSize);
    void clearRadioDataVectors();
    void prepareRestoredConnectionMessage();
    void stopRadioBrowserStream();
    void returnRadioBrowserToPlay();
    void playCountryStream();
    void startRadioBrowserStream();
    void updatePlayPauseIcons();
    void switchToDefaultTabIfNoCountryStationPlaying();
    void resetImageIfStopped();

    void updateFavoriteColumnLayout();
    void onTrashIconCliced(const QModelIndex &index);
    void refreshFavoritePlaylist();
};

#endif // RADIOLIST_H
