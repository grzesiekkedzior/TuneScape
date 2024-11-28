#ifndef ICECASTXMLDATA_H
#define ICECASTXMLDATA_H
#include <QSharedPointer>
#include <QUrl>
#include "audioprocessor.h"
#include "container.h"
#include "customcolordelegate.h"
#include "jsonlistprocessor.h"
#include "miniplayer.h"
#include "radioaudiomanager.h"
#include "radioinfo.h"
#include "streamrecorder.h"
#include "ui_mainwindow.h"

class TrayIcon;

struct IceCastTableRow
{
    QString station = "";
    QString genre = "";
    QString codec = "";
    QString bitrate = "";
    QString sample = "";
    QString listen_url = "";
};

struct DOM
{
    const QString STATION = "server_name";
    const QString GENRE = "genre";
    const QString CODEC = "server_type";
    const QString BITRATE = "bitrate";
    const QString SAMPLE = "samplerate";
    const QString LISTEN = "listen_url";
};

class IceCastXmlData : public QObject
{
    Q_OBJECT
public:
    IceCastXmlData();
    IceCastXmlData(Ui::MainWindow *ui);
    ~IceCastXmlData();

    void setUi();
    void setPlaying(bool b);
    void loadXmlData();
    void loadXmlToTable();
    void addRowToTable(const IceCastTableRow &row);
    void loadXmlAsync();
    void setJsonListProcessor(JsonListProcessor &jsonListProcesor);
    void setRadioAudioManager(RadioAudioManager &radioAudioManager);
    void setRadioList(RadioList *radioList);
    void setRadioInfo(RadioInfo *radioInfo);
    void setTableProperty(Ui::MainWindow *ui);
    void clearTableViewColor();

    void clearInfoData();
    bool getPlaying();
    void playStreamOnStart(const QModelIndex &index);

    bool getIsStationsLoaded() const;
    void setIsStationsLoaded(bool newIsStationsLoaded);

    int getCurrentPlayingStation() const;
    void setCurrentPlayingStation(int newCurrentPlayingStation);

    QVector<IceCastTableRow> getIceCastStationTableRows() const;
    void setIceCastStationTableRows(const QVector<IceCastTableRow> &newIceCastStationTableRows);

    IceCastTableRow getIceCastTableRow(int index);
    void addToFavoriteStations();
    void loadFavoriteIceCastStations();
    void loadDiscoveryStations();
    void setFavoriteStations();

    bool getIsFavoriteOnTreeCliced() const;
    void setIsFavoriteOnTreeCliced(bool newIsFavoriteOnTreeCliced);

    void setDiscoveryList();
    void setFavoriteList();

    bool getIsFavoritePlaying() const;
    void setIsFavoritePlaying(bool newIsFavoritePlaying);
    void setIndexColor(const QModelIndex &index);

    QModelIndex getIndexPlayingStation() const;
    void setIndexPlayingStation(const QModelIndex &newIndexPlayingStation);

    void makeShareStreamRecorder(QSharedPointer<StreamRecorder> streamRecorder);
    void playPauseIcon();
    bool getIsDownloadFinish() const;
    void setIsDownloadFinish(bool newIsDownloadFinish);

    TrayIcon *getTrayIcon() const;
    void setTryIcon(TrayIcon *newTrayIcon);

private slots:
    void onDoubleListClicked(const QModelIndex &index);
    void updateProgressBar(int progress);
    void showErrorMessageBox(const QString &errorMessage);
    void tableViewActivated(const QModelIndex &index);

private:
    Ui::MainWindow *ui;
    DOM dom;
    RadioAudioManager *radioAudioManager;
    JsonListProcessor *jsonListProcesor;
    RadioList *radioList = nullptr;
    RadioInfo *radioInfo = nullptr;
    QSharedPointer<StreamRecorder> streamRecorder;
    bool isDownloadFinish = false;
    bool isPlaying = false;
    bool isStationsLoaded = false;
    bool isFavoriteOnTreeCliced = false;
    bool isFavoritePlaying = false;
    int currentPlayingStation = -1;
    QModelIndex indexPlayingStation;
    const QString iceCastUrl = "http://dir.xiph.org/yp.xml";
    QVector<IceCastTableRow> iceCastStationTableRows;
    QVector<IceCastTableRow> favoriteStations;
    QVector<IceCastTableRow> discoveryStations;

    void checkIsRadioOnPlaylist();
    void setIceCastInfo(int index);

    QSharedPointer<CustomColorDelegate> customColor{nullptr};

    TrayIcon *trayIcon = nullptr;
    AudioProcessor &audioProcessor = SingletonContainer::getSingleton().getInstance<AudioProcessor>();
    miniplayer &miniPlayer = SingletonContainer::getSingleton().getInstance<miniplayer>();
};

#endif // ICECASTXMLDATA_H
