#ifndef COUNTRY_H
#define COUNTRY_H

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QPixmap>
#include <QSharedPointer>
#include <QString>
#include <QVector>

#include "FavoriteManager.h"
#include "RadioImageManager.h"
#include "audioprocessor.h"
#include "container.h"
#include "controller/PlaybackController.h"
#include "controller/PlayerUIController.h"
#include "customcolordelegate.h"
#include "jsonlistprocessor.h"
#include "model/RadioStationsModel.h"

namespace Ui {
class MainWindow;
}

class RadioList;

class Country : public QObject
{
    Q_OBJECT

    struct CountriesData
    {
        QString countryName;
        QString countryIso;
        QString countryCount;
    };

public:
    Country() = default;
    ~Country() override = default;

    void setData(Ui::MainWindow *ui, RadioList *radioList);
    void load();

    bool getIsPlaying() const;
    void setIsPlaying(bool isPlaying);

    int getCurrentIndexPlaying() const;
    void setCurrentIndexPlaying(int index);

    const RadioStation &getCurrentStation() const;

    void clearTableColor();

    FavoriteManager *getFavoriteManager() const;
    void setFavoriteManager(FavoriteManager *favoriteManager);

private slots:
    void searchCountry(const QString &country);
    void onDoubleListClicked(const QModelIndex &index);

private:
    void loadCountriesToComboBox();

    QNetworkReply *setConnection(const QString &endpoint);
    bool createCountryArray(QNetworkReply *reply);
    bool createTable(QNetworkReply *reply);

    void setIndexColor(const QModelIndex &index);

    Ui::MainWindow *ui = nullptr;
    RadioList *radioList = nullptr;
    FavoriteManager *favoriteManager = nullptr;

    QNetworkReply *reply = nullptr;
    QNetworkAccessManager manager;

    JsonListProcessor jsonListProcessor;

    QVector<CountriesData> countriesData;

    QPixmap pixmap;
    QSharedPointer<CustomColorDelegate> customColor;

    bool isPlaying = false;
    int currentIndexPlaying = -1;

    RadioStationsModel *countryStationsModel = nullptr;
    RadioStation currentStation;
    RadioImageManager imageManager;

    PlaybackController &playbackController = SingletonContainer::getSingleton()
                                                 .getInstance<PlaybackController>();

    AudioProcessor &audioProcessor = SingletonContainer::getSingleton().getInstance<AudioProcessor>();

    PlayerUIController &playerUIController = SingletonContainer::getSingleton()
                                                 .getInstance<PlayerUIController>();

    inline static const QString NameKey = QStringLiteral("name");
    inline static const QString GenreKey = QStringLiteral("tags");
    inline static const QString CountryKey = QStringLiteral("country");
    inline static const QString HomepageKey = QStringLiteral("homepage");
    inline static const QString ResolvedUrlKey = QStringLiteral("url_resolved");
    inline static const QString FaviconKey = QStringLiteral("favicon");

    inline static const QString CountryNameKey = QStringLiteral("name");
    inline static const QString CountryIsoKey = QStringLiteral("iso_3166_1");
    inline static const QString CountryCountKey = QStringLiteral("stationcount");

    inline static const QString CountrySearchEndpoint = QStringLiteral(
        "json/stations/search?country=");

    inline static const QString CountriesEndpoint = QStringLiteral("json/countries");

    inline static const QString RadioBrowserPlaylist = QStringLiteral("radiobrowser.txt");
};

#endif // COUNTRY_H