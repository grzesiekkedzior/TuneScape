#ifndef RADIOLIST_H
#define RADIOLIST_H

#include "include/FavoriteManager.h"
#include "include/IconLoader.h"
#include "include/RadioBrowserApi.h"
#include "include/controller/PlaybackController.h"
#include "include/controller/PlayerUIController.h"
#include "include/customcolordelegate.h"
#include "include/miniplayer.h"
#include "include/model/RadioStationsModel.h"
#include "include/radioinfo.h"
#include "include/streamreader.h"
#include "include/streamrecorder.h"
#include "include/theme.h"
#include <QPointer>
#include <array>

enum Stations { TOP, POPULAR, NEW, FAVORITE, SEARCH };

class QProgressDialog;
class QTableView;
class QMainWindow;
namespace Ui { class MainWindow; }

class RadioList : public QObject
{
    Q_OBJECT
public:
    RadioList(Ui::MainWindow *ui, FavoriteManager *favoriteManager, RadioBrowserApi *catalog = nullptr,
              PlaybackController *playback = nullptr);
    ~RadioList() override;

    void loadAllData();
    PlaybackController &playback() const;
    void togglePlayback();
    void selectPlaylist(Stations station);
    miniplayer &miniPlayerWindow();

    RadioBrowserApi &catalog() const;
    IconLoader &stationImages();
    void showCatalogProgress(RadioBrowserApi::RequestId id, const QString &message);
    void showCatalogResult(RadioBrowserApi::RequestId id, const RadioBrowserApi::Result &result);
    void hideCatalogProgress(RadioBrowserApi::RequestId id);

    bool isFavorite(const RadioStation &station) const;
    void setFavorite(const RadioStation &station, bool favorite);
    StreamRecorder &getStreamRecorder();
    void setMainWindow(QMainWindow *window);
    void setFavoriteLibrary();
    const QVector<RadioStation> &stations(Stations station) const;
    void updateThemeAppearance();
    void setTheme(Theme *theme);
    void applyFavoritePlaylist(QVector<RadioStation> stations);

    RadioStationsModel &stationModel() const;
    void showStationMenu(QTableView *view, const QPoint &position);

signals:
    void trackTitleReceived(const QString &title);
    void stationImageChanged();
    void catalogPlaylistLoaded(Stations station);

private slots:
    void showStationContextMenu(const QPoint &position);
    void onTreeViewItemClicked(const QModelIndex &index);
    void playIndex(const QModelIndex &index);
    void addRadioToFavorite();

private:
    void detachView();
    void restoreSelection();
    std::array<QVector<RadioStation>, Stations::SEARCH + 1> allStations;
    std::array<std::optional<RadioStation>, Stations::SEARCH + 1> m_selections;
    RadioStationsModel *radioStationsModel;
    void onSessionChanged();
    void updateStationColoring();
    QTableView *currentStationView() const;
    void playAdjacentStation(int offset);
    void sliderMoved(int volume);
    void handleDataReceived(quint64 generation, const QString &data);
    void startStopRecord();
    void updateRecordButton(StreamRecorder::State state);
    void stopStreamServices();
    void searchStations();
    void requestPlaylist(Stations station, bool showProgress);
    void cancelSearch();
    void loadPlayingStationDetails(const RadioStation &station);
    void clearPlayingStationDetails();
    void displayPlaylist();
    void setTopListOnStart();
    void showMiniplayer();
    void maximizeWindow();
    void switchToPlaylist(Stations station);

    int currentPlaylistIndex = -1;
    Ui::MainWindow *ui;
    QPointer<RadioBrowserApi> m_catalog;
    QPointer<PlaybackController> m_playback;
    std::array<RadioBrowserApi::RequestId, 3> m_playlistRequests{};
    std::array<bool, 3> m_playlistLoaded{};
    RadioBrowserApi::RequestId m_searchRequest = 0;
    RadioBrowserApi::RequestId m_detailsRequest = 0;
    RadioBrowserApi::RequestId m_progressRequest = 0;
    QPointer<QProgressDialog> m_catalogProgress;
    QString m_progressMessage;
    QString m_detailsCodec;
    QString m_playbackStatus;
    QString m_trackTitle;
    bool m_playlistDisplayPending = false;

    StreamReader streamReader;
    StreamRecorder streamRecorder;
    RadioInfo radioInfo;
    QPointer<QMainWindow> mainWindow;
    FavoriteManager *favoriteManager;
    miniplayer miniPlayer;
    PlayerUIController playerUIController;
    IconLoader iconLoader;
    IconLoader::RequestId m_imageRequest = 0;
    QPointer<Theme> theme;
    CustomColorDelegate *customColor = nullptr;
};

#endif // RADIOLIST_H
