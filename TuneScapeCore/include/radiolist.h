#ifndef RADIOLIST_H
#define RADIOLIST_H

#include <QHeaderView>
#include <QMenu>
#include <QMessageBox>
#include <QObject>
#include <QPixmap>
#include <QStandardItemModel>
#include <QStringList>
#include <array>
#include "ui_mainwindow.h"
#include "Country.h"
#include "FavoriteManager.h"
#include "container.h"
#include "controller/PlaybackController.h"
#include "controller/PlayerUIController.h"
#include "include/IconLoader.h"
#include "include/RadioBrowserApi.h"
#include "include/customcolordelegate.h"
#include "include/jsonlistprocessor.h"
#include "include/radioaudiomanager.h"
#include "include/radioinfo.h"
#include "include/theme.h"
#include "miniplayer.h"
#include "model/RadioStationsModel.h"
#include "streamreader.h"
#include "streamrecorder.h"

enum Stations { TOP, POPULAR, NEW, FAVORITE, SEARCH, DISCOVERY };

class QProgressDialog;

class RadioList : public QObject
{
    Q_OBJECT
public:
    RadioList(Ui::MainWindow *ui, FavoriteManager *favoriteManager, RadioBrowserApi *catalog = nullptr);
    ~RadioList() override;

    void loadAllData();
    void prepareStationChange(const QUrl &url);
    void loadPlayingStationDetails(const RadioStation &station);

    RadioBrowserApi &catalog() const;
    IconLoader &stationImages();
    void showCatalogProgress(RadioBrowserApi::RequestId id, const QString &message);
    void showCatalogResult(RadioBrowserApi::RequestId id, const RadioBrowserApi::Result &result);
    void hideCatalogProgress(RadioBrowserApi::RequestId id);

    void clearTableViewColor();
    bool isFavorite(const RadioStation &station) const;
    void setFavorite(const RadioStation &station, bool favorite);

    void onPlayPauseButtonCliced();
    RadioAudioManager &getRadioManager();

    void addEmptyIconButton(int row);

    StreamRecorder &getStreamRecorder();

    bool getIsBrowseStationLoaded() const;
    void setIsBrowseStationLoaded(bool newIsBrowseStationLoaded);

    void onTrayViewItemClicked(const QModelIndex &index);
    void onTrayClickedandPlay(const QModelIndex &index);

    void setIsTreeClicked(bool newIsTreeClicked);

    QString getItem() const;

    int getRadioIndexNumber() const;

    QString getTreeItem() const;

    bool getIsSearchTablelDoubleCliced() const;
    void setIsSearchTablelDoubleCliced(bool newIsSearchTablelDoubleCliced);

    QMainWindow *getMainWindow() const;
    void setMainWindow(QMainWindow *newMainWindow);
    void setFavoriteLibrary();


    bool getIsStopClicked() const;
    void setIsStopClicked(bool newIsStopClicked);

    void resetTreeItemIfSearch();

    // Add and remove the playlists!!!
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

    void updateThemeAppearance();
    void setTheme(Theme *theme);
    void applyFavoritePlaylist(QVector<RadioStation> stations);

signals:
    void playIconButtonDoubleClicked(int radioNumber);
    void allIconsLoaded();
    void sendTitleToTray(QString title);
    void trackTitleReceived(const QString &title);
    void catalogPlaylistLoaded(Stations station);
public slots:
    void onStopButtonClicked();
    void showStationContextMenu(const QPoint &position);

private slots:
    void onTreeViewItemClicked(const QModelIndex &index);
    void onTableViewDoubleClicked(const QModelIndex &index);
    void onNextButtonClicked();
    void onPrevButtonClicked();
    void onTableViewClicked(const QModelIndex &index);
    void tableViewActivated(const QModelIndex &index);
    void addRadioToFavorite();

private:
    int currentPlaylistIndex = -1;
    int currentStationIndex = -1;
    int currentPlayListPlaying = -1;
    Ui::MainWindow *ui = nullptr;
    QHeaderView *header = nullptr;
    QStringList headers;
    QString treeItem;
    RadioBrowserApi *m_catalog;
    std::array<RadioBrowserApi::RequestId, 3> m_playlistRequests{};
    std::array<bool, 3> m_playlistLoaded{};
    RadioBrowserApi::RequestId m_searchRequest = 0;
    RadioBrowserApi::RequestId m_detailsRequest = 0;
    RadioBrowserApi::RequestId m_progressRequest = 0;
    QProgressDialog *m_catalogProgress;
    QString m_progressMessage;
    QString m_detailsCodec;
    bool m_playlistDisplayPending = false;
    bool m_hasStationImage = false;

    StreamReader streamReader;
    StreamRecorder streamRecorder;
    RadioInfo radioInfo;

    QString currentRadioPlayingAddress = "";
    QString item = "";

    QMainWindow *mainWindow = nullptr;
    FavoriteManager *favoriteManager = nullptr;

    //Singletons
    miniplayer &miniPlayer = SingletonContainer::getSingleton().getInstance<miniplayer>();
    Country &country = SingletonContainer::getSingleton().getInstance<Country>();
    PlaybackController &playbackController = SingletonContainer::getSingleton()
                                                 .getInstance<PlaybackController>();
    PlayerUIController &playerUIController = SingletonContainer::getSingleton()
                                                 .getInstance<PlayerUIController>();

    RadioStation currentPlayingStation;
    IconLoader iconLoader;
    IconLoader::RequestId m_imageRequest = 0;
    Theme *theme = nullptr;

    int radioIndexNumber = 0;
    int radioEnterIndexNumber = 0;
    int radioIndexCurrentPlaying = 0;
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

    const QString LIBRARY_TREE = "Library";
    const QString FAVORITE_TREE = "Favorite";
    const QString FAVORITE_ICECAST = "IceCastFavorite";
    const QString ICECAST_PLAYLIST = "icecast.txt";

    const QString RADIO_ICON = ":/images/img/radio96x96.png";
    const QString TUNESCAPE_ICON = ":/images/img/TuneScape.ico";

    void playSelectedStation(int radioNumber);
    void setIndexColor(int row);
    void sliderMoved(int move);
    void handleDataReceived(const QString &data);
    void startStopRecord();
    void updateRecordButton(StreamRecorder::State state);
    void stopStreamServices();
    void searchStations();

    void clearRadioInfoLabel();
    void requestPlaylist(Stations station, bool showProgress);
    void cancelSearch();
    void clearPlayingStationDetails();
    void displayPlaylist();
    void setTopListOnStart();
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

    void clearInfoData(int stationIndex);
    void showMiniplayer();
    void maximizeWindow();

    QScopedPointer<CustomColorDelegate> customColor{nullptr};
    void switchToPlaylist(Stations station);
    void loadRadioIconsFromNetwork(int dataSize);
    void stopRadioBrowserStream();
    void returnRadioBrowserToPlay();
    void playCountryStream();
    void startRadioBrowserStream();
    void updatePlayPauseIcons();
    void switchToDefaultTabIfNoCountryStationPlaying();

    void updateFavoriteColumnLayout();
};

#endif // RADIOLIST_H
