#include "include/Country.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QRegularExpression>
#include <QTableView>
#include "include/StationDetailsDialog.h"
#include "include/radiolist.h"

namespace {

constexpr auto WorldImagePath = ":/images/img/word.png";

} // namespace

Country::Country(const IconLoader &iconLoader, QObject *parent)
    : QObject(parent)
    , iconLoader(this)
{}

void Country::setData(Ui::MainWindow *ui, RadioList *radioList)
{
    Q_ASSERT(ui);
    Q_ASSERT(radioList);

    this->ui = ui;
    this->radioList = radioList;

    countryStationsModel = new RadioStationsModel(this);
    ui->tableOfCoutries->setModel(countryStationsModel);
    connect(&iconLoader,
            &IconLoader::iconLoaded,
            countryStationsModel,
            &RadioStationsModel::setStationIcon);

    ui->tableOfCoutries->setIconSize(QSize(20, 20));

    connect(ui->comboBox, &QComboBox::textActivated, this, &Country::searchCountry);

    connect(ui->tableOfCoutries, &QTableView::doubleClicked, this, &Country::onDoubleListClicked);

    pixmap.load(WorldImagePath);
    ui->worldImage->setPixmap(pixmap);
    ui->worldImage->setAlignment(Qt::AlignCenter);

    ui->tableOfCoutries->setVisible(false);
    ui->tableOfCoutries->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(ui->tableOfCoutries,
            &QTableView::customContextMenuRequested,
            this,
            &Country::showStationContextMenu);
    ui->tableOfCoutries->verticalHeader()->setDefaultSectionSize(30);

    QHeaderView *header = ui->tableOfCoutries->horizontalHeader();

    header->setSectionResizeMode(static_cast<int>(Column::StationColumn), QHeaderView::Stretch);

    header->setSectionResizeMode(static_cast<int>(Column::GenreColumn),
                                 QHeaderView::ResizeToContents);
}

void Country::load()
{
    QNetworkReply *countriesReply = setConnection(CountriesEndpoint);

    if (!createCountryArray(countriesReply))
        return;

    loadCountriesToComboBox();
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
        station.codec = object[QStringLiteral("codec")].toString().trimmed();
        station.language = object[QStringLiteral("language")].toString().trimmed();
        station.state = object[QStringLiteral("state")].toString().trimmed();
        station.countryCode = object[QStringLiteral("countrycode")].toString().trimmed();
        station.bitrate = object[QStringLiteral("bitrate")].toInt();
        station.votes = object[QStringLiteral("votes")].toInt();
        station.clickCount = object[QStringLiteral("clickcount")].toInt();
        stations.append(std::move(station));
    }

    countryStationsModel->setStations(stations);
    loadStationIcons();
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

void Country::loadStationIcons()
{
    if (!countryStationsModel)
        return;

    for (int row = 0; row < countryStationsModel->rowCount(QModelIndex()); ++row) {
        const RadioStation &station = countryStationsModel->station(row);

        if (station.iconUrl.isEmpty())
            continue;

        iconLoader.loadIcon(row, QUrl(station.iconUrl), QSize(128, 128));
    }
}

void Country::showStationContextMenu(const QPoint &position)
{
    if (!countryStationsModel || !favoriteManager || !radioList)
        return;

    const QModelIndex index = ui->tableOfCoutries->indexAt(position);

    if (!index.isValid())
        return;

    if (index.row() < 0 || index.row() >= countryStationsModel->size())
        return;

    const RadioStation station = countryStationsModel->station(index.row());

    QMenu menu(ui->tableOfCoutries);

    const bool isCurrentStationPlaying = playbackController.isPlaying() && isPlaying
                                         && station.streamUrl == currentStation.streamUrl;

    QAction *playAction = menu.addAction(isCurrentStationPlaying ? tr("Stop") : tr("Play"));

    const bool isFavorite = favoriteManager->isAddressExists(station.streamUrl,
                                                             RadioBrowserPlaylist);

    QAction *favoriteAction = menu.addAction(isFavorite ? tr("Remove from favorites")
                                                        : tr("Add to favorites"));

    QAction *detailsAction = menu.addAction(tr("Details"));

    menu.addSeparator();

    QAction *openHomepageAction = menu.addAction(tr("Open homepage"));
    openHomepageAction->setEnabled(!station.homepage.isEmpty());

    QAction *selectedAction = menu.exec(ui->tableOfCoutries->viewport()->mapToGlobal(position));

    if (!selectedAction)
        return;

    if (selectedAction == playAction) {
        if (isCurrentStationPlaying) {
            playbackController.stop();
            audioProcessor.stop();

            setIsPlaying(false);
            setCurrentIndexPlaying(-1);

            playerUIController.setPlayIcon();
            playerUIController.setDefaultImage();
            playerUIController.clearMetadata();

            clearTableColor();

            radioList->setIsBrowseStationLoaded(false);
            radioList->setIsStopClicked(true);
        } else {
            onDoubleListClicked(index);
        }

        return;
    }

    if (selectedAction == favoriteAction) {
        const QString data = station.iconUrl + "," + station.streamUrl + "," + station.station + ","
                             + station.country + "," + station.genre + "," + station.homepage;

        const bool isNowFavorite = favoriteManager->toggleFavorite(station.streamUrl,
                                                                   data,
                                                                   RadioBrowserPlaylist);

        if (station.streamUrl == currentStation.streamUrl)
            playerUIController.setFavorite(isNowFavorite);

        radioList->refreshFavoritePlaylist();

        return;
    }

    if (selectedAction == detailsAction) {
        StationDetailsDialog dialog(station, ui->tableOfCoutries);

        dialog.exec();
        return;
    }

    if (selectedAction == openHomepageAction) {
        QDesktopServices::openUrl(QUrl(station.homepage));
    }
}