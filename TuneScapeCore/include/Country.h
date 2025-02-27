#ifndef COUNTRY_H
#define COUNTRY_H

#include "audioprocessor.h"
#include "container.h"
#include "customcolordelegate.h"
#include "jsonlistprocessor.h"
#include "miniplayer.h"
#include "qnetworkreply.h"
#include "qobject.h"
#include "ui_mainwindow.h"

class RadioList;

class Country : public QObject
{
    struct TableRow
    {
        QString station;
        QString genre;
        QString country;
        QString stationUrl;
    };

    struct DtoFavourite
    {
        QString icon;
        QString stream;
        QString station;
        QString genre;
        QString country;
        QString stationUrl;
    };

    struct CountriesData
    {
        QString countryName;
        QString countryIso;
        QString countryCount;
    };

    Q_OBJECT
public:
    Country();
    ~Country();

    void setData(Ui::MainWindow *ui, RadioList *radioList);
    void load();
    void createHeaders();
    void loadCountriesToComboBox();
    QNetworkReply *setConnection(QString endpoint);
    bool createCountryArray(QNetworkReply *reply);
    bool createTable(QNetworkReply *reply);
    void searchCountry(QString country);
    void addRowToTable(const TableRow &row);
    void setIndexColor(const QModelIndex &index);

    // const -> JSON
    const QString NAME = "name";
    const QString GENRE = "tags";
    const QString COUNTRY = "country";
    const QString URL = "homepage";
    const QString URL_RESOLVED = "url_resolved";
    const QString URL_STREAM = "url";
    const QString FAVICON = "favicon";
    // const -> array
    const QString COUNTRY_NAME = "name";
    const QString COUNTRY_ISO = "iso_3166_1";
    const QString COUNTRY_COUNT = "stationcount";
    const QString RADIO_BROWSER_PLAYLIST = "radiobrowser.txt";

    bool getIsPlaying() const;
    void setIsPlaying(bool newIsPlaying);
    void clearTableColor();
    void checkIsOnPlaylist(const QModelIndex &index, QString currentRadioPlayingAddress);

    QString getIconAddresses(int index) const;

    DtoFavourite dtoFavorite;

    void createDtoFavorites(const QModelIndex &index, QString url);

    QVector<QString> getStreamAddresses() const;

    int getCurrentIndexPlaying() const;
    void setCurrentIndexPlaying(int newCourrentIndexPlaying);

private slots:
    void onDoubleListClicked(const QModelIndex &index);

private:
    Ui::MainWindow *ui = nullptr;
    RadioList *radioList = nullptr;

    QNetworkReply *reply = nullptr;
    QNetworkAccessManager manager;

    JsonListProcessor jsonListProcessor;
    QJsonDocument doc;

    QVector<TableRow> tableRows;
    QVector<CountriesData> countriesData;
    QVector<QString> streamAddresses;
    QVector<QString> iconAddresses;

    QPixmap pixmap;

    QSharedPointer<CustomColorDelegate> customColor{nullptr};

    bool isPlaying;
    int courrentIndexPlaying = -1;

    const QString COUNTRY_ENDPOINT_SEARCH = "json/stations/search?country=";
    const QString COUNTRY_ENDPOINT_NAME = "json/countries";

    void playPauseIcon();
    void setRadioImage(const QModelIndex &index);
    AudioProcessor &audioProcessor = SingletonContainer::getSingleton().getInstance<AudioProcessor>();
    miniplayer &miniPlayer = SingletonContainer::getSingleton().getInstance<miniplayer>();
};

#endif // COUNTRY_H
