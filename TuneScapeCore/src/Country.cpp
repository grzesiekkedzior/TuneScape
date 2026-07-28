#include "include/Country.h"
#include "include/radiolist.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QRegularExpression>
#include <QTableView>

namespace {

constexpr auto WorldImagePath = ":/images/img/word.png";

} // namespace

void Country::setData(Ui::MainWindow *ui, RadioList *radioList)
{
    Q_ASSERT(ui);
    Q_ASSERT(radioList);

    this->ui = ui;
    this->radioList = radioList;

    countryStationsModel = new RadioStationsModel(this);
    ui->tableOfCoutries->setModel(countryStationsModel);

    connect(ui->comboBox, &QComboBox::textActivated, this, &Country::searchCountry);

    connect(ui->tableOfCoutries, &QTableView::doubleClicked, this, &Country::onDoubleListClicked);

    pixmap.load(WorldImagePath);
    ui->worldImage->setPixmap(pixmap);
    ui->worldImage->setAlignment(Qt::AlignCenter);

    ui->tableOfCoutries->setVisible(false);
}

void Country::load()
{
    QNetworkReply *countriesReply = setConnection(CountriesEndpoint);

    if (!createCountryArray(countriesReply))
        return;

    loadCountriesToComboBox();
    ui->tableOfCoutries->verticalHeader()->setDefaultSectionSize(18);
}

void Country::loadCountriesToComboBox()
{
    ui->comboBox->clear();

    for (const CountriesData &country : countriesData)
        ui->comboBox->addItem(country.countryName);
}

QNetworkReply *Country::setConnection(const QString &endpoint)
{
    RadioStations radioStations(endpoint);

    return jsonListProcessor.checkAvailability(radioStations.getAddresses());
}

bool Country::createCountryArray(QNetworkReply *reply)
{
    if (!reply)
        return false;

    const QJsonDocument document = jsonListProcessor.createJasonDocument(reply);

    reply->deleteLater();

    if (!document.isArray())
        return false;

    countriesData.clear();

    for (const QJsonValue &value : document.array()) {
        const QJsonObject object = value.toObject();

        CountriesData country;
        country.countryName = object[CountryNameKey].toString();
        country.countryIso = object[CountryIsoKey].toString();
        country.countryCount = QString::number(object[CountryCountKey].toInt());

        countriesData.append(std::move(country));
    }

    return true;
}

bool Country::createTable(QNetworkReply *reply)
{
    if (!reply || !countryStationsModel)
        return false;

    const QJsonDocument document = jsonListProcessor.createJasonDocument(reply);

    reply->deleteLater();

    if (!document.isArray())
        return false;

    QVector<RadioStation> stations;
    stations.reserve(document.array().size());

    for (const QJsonValue &value : document.array()) {
        const QJsonObject object = value.toObject();

        RadioStation station;

        station.station = object[NameKey].toString().trimmed().replace(QRegularExpression(
                                                                           QStringLiteral(
                                                                               "^[\\s?_.-]+")),
                                                                       QString());

        station.genre = object[GenreKey].toString();

        const qsizetype commaIndex = station.genre.indexOf(',');

        if (commaIndex >= 0)
            station.genre = station.genre.left(commaIndex);

        station.genre = station.genre.trimmed();
        station.country = object[CountryKey].toString().trimmed();
        station.homepage = object[HomepageKey].toString().trimmed();
        station.streamUrl = object[ResolvedUrlKey].toString().trimmed();
        station.iconUrl = object[FaviconKey].toString().trimmed();

        stations.append(std::move(station));
    }

    countryStationsModel->setStations(stations);
    return true;
}

void Country::searchCountry(const QString &country)
{
    if (!ui->tableOfCoutries->isVisible()) {
        ui->tableOfCoutries->show();
        ui->worldImage->hide();
    }

    const QString endpoint = CountrySearchEndpoint + country;
    QNetworkReply *stationsReply = setConnection(endpoint);

    createTable(stationsReply);
}

void Country::onDoubleListClicked(const QModelIndex &index)
{
    if (!index.isValid())
        return;

    if (!radioList->getJsonListProcessor()->isConnected)
        return;

    if (!countryStationsModel)
        return;

    if (index.row() < 0 || index.row() >= countryStationsModel->size())
        return;

    if (!favoriteManager)
        return;

    currentStation = countryStationsModel->station(index.row());

    playbackController.play(QUrl(currentStation.streamUrl));
    audioProcessor.start(currentStation.streamUrl);

    setIsPlaying(true);
    setCurrentIndexPlaying(index.row());

    setIndexColor(index);

    playerUIController.clearMetadata();
    radioList->getRadioInfo()->clearInfo();
    radioList->clearTableViewColor();

    radioList->getRadioInfo()->loadEndpoint(currentStation.station);
    radioList->getRadioInfo()->processInfoJsonQuery();
    radioList->getRadioInfo()->setDataOnTable();

    const QPixmap downloadedPixmap = imageManager.downloadImageSync(QUrl(currentStation.iconUrl));

    const QPixmap preparedPixmap = imageManager.prepareImage(downloadedPixmap);

    playerUIController.setStationImage(preparedPixmap);

    const bool isFavorite = favoriteManager->isAddressExists(currentStation.streamUrl,
                                                             RadioBrowserPlaylist);

    playerUIController.setFavorite(isFavorite);
    playerUIController.setPauseIcon();

    radioList->setIsStopClicked(false);
    radioList->getSongTitle(currentStation.streamUrl);

    const auto recorder = radioList->getStreamRecorder();

    if (recorder->getIsRecording()) {
        recorder->stopRecording();
        recorder->setIsRecording(false);
    }
}

int Country::getCurrentIndexPlaying() const
{
    return currentIndexPlaying;
}

void Country::setCurrentIndexPlaying(int index)
{
    currentIndexPlaying = index;
}

FavoriteManager *Country::getFavoriteManager() const
{
    return favoriteManager;
}

void Country::setFavoriteManager(FavoriteManager *manager)
{
    favoriteManager = manager;
}

bool Country::getIsPlaying() const
{
    return isPlaying;
}

void Country::setIsPlaying(bool playing)
{
    isPlaying = playing;
}

void Country::clearTableColor()
{
    if (customColor)
        customColor->clearRowColor();

    ui->tableOfCoutries->update();
}

const RadioStation &Country::getCurrentStation() const
{
    return currentStation;
}

void Country::setIndexColor(const QModelIndex &index)
{
    if (!index.isValid() || !theme)
        return;

    customColor.reset(new CustomColorDelegate(index.row(), theme->playingRowColor(), this));

    ui->tableOfCoutries->setItemDelegate(customColor.get());
    ui->tableOfCoutries->viewport()->update();
}

void Country::setTheme(Theme *newTheme)
{
    theme = newTheme;
}

void Country::updateThemeAppearance()
{
    if (currentIndexPlaying < 0 || !countryStationsModel)
        return;

    const QModelIndex index = countryStationsModel->index(currentIndexPlaying, 0);

    if (!index.isValid())
        return;

    setIndexColor(index);
}