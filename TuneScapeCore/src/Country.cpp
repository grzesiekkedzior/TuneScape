#include "include/Country.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QPersistentModelIndex>
#include <utility>
#include <QTableView>
#include "include/StationDetailsDialog.h"
#include "include/radiolist.h"

namespace {

constexpr auto WorldImagePath = ":/images/img/word.png";

} // namespace

void Country::setData(Ui::MainWindow *ui, RadioList *radioList)
{
    Q_ASSERT(ui);
    Q_ASSERT(radioList);

    this->ui = ui;
    this->radioList = radioList;
    connect(radioList, &QObject::destroyed, this, [this] {
        iconLoader.cancelPending();
        delete countryStationsModel;
        countryStationsModel = nullptr;
        customColor.clear();
        countriesRequest = stationsRequest = 0;
        countriesLoaded = false;
        isPlaying = false;
        currentIndexPlaying = -1;
        this->ui = nullptr;
        this->radioList = nullptr;
        theme = nullptr;
    });
    connect(ui->tabRadioListWidget, &QTabWidget::currentChanged, radioList, [this] {
        if (this->ui->tabRadioListWidget->currentWidget() == this->ui->listCountries) {
            load();
        } else {
            cancelStationSearch();
            this->radioList->hideCatalogProgress(countriesRequest);
        }
    });

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
    if (!radioList || countriesLoaded)
        return;
    if (!countriesRequest) {
        countriesRequest = radioList->catalog().request(QUrl("json/countries"), radioList,
            [this](auto id, const auto &result) {
                if (id != countriesRequest)
                    return;
                countriesRequest = 0;
                radioList->showCatalogResult(id, result);
                using Status = RadioBrowserApi::Status;
                if (result.status == Status::Success || result.status == Status::EmptyResult) {
                    countriesLoaded = true;
                    loadCountriesToComboBox(result.data);
                }
            });
    }
    if (ui->tabRadioListWidget->currentWidget() == ui->listCountries)
        radioList->showCatalogProgress(countriesRequest, tr("Loading Radio Browser countries…"));
}

void Country::loadCountriesToComboBox(const QJsonArray &countries)
{
    ui->comboBox->clear();
    for (const QJsonValue &entry : countries) {
        const auto object = entry.toObject();
        const auto name = object["name"].toString();
        if (!name.isEmpty())
            ui->comboBox->addItem(name, object["iso_3166_1"].toString());
    }
}

void Country::cancelStationSearch()
{
    const auto id = std::exchange(stationsRequest, 0);
    radioList->hideCatalogProgress(id);
    radioList->catalog().cancel(id);
}

void Country::searchCountry(const QString &country)
{
    if (!radioList)
        return;
    cancelStationSearch();
    ui->tableOfCoutries->show();
    ui->worldImage->hide();
    iconLoader.cancelPending();
    countryStationsModel->setStations({});
    clearTableColor();

    const auto countryCode = ui->comboBox->currentData().toString();
    const QUrl endpoint = countryCode.isEmpty()
        ? RadioBrowserApi::endpoint("json/stations/search", "country", country)
        : RadioBrowserApi::endpoint("json/stations/search", "countrycode", countryCode);
    stationsRequest = radioList->catalog().request(endpoint, radioList,
        [this](auto id, const auto &result) {
            if (id != stationsRequest)
                return;
            stationsRequest = 0;
            radioList->showCatalogResult(id, result);
            if (result.status == RadioBrowserApi::Status::Success) {
                countryStationsModel->setStations(JsonListProcessor::stationsFromJson(result.data));
                loadStationIcons();
            }
        });
    radioList->showCatalogProgress(stationsRequest, tr("Loading stations for %1…").arg(country));
}

void Country::onDoubleListClicked(const QModelIndex &index)
{
    if (!index.isValid())
        return;

    if (!countryStationsModel)
        return;

    if (index.row() < 0 || index.row() >= countryStationsModel->size())
        return;

    currentStation = countryStationsModel->station(index.row());

    const QUrl streamUrl(currentStation.streamUrl);
    playerUIController.clearMetadata();
    radioList->prepareStationChange(streamUrl);
    playbackController.play(streamUrl);

    setIsPlaying(true);
    setCurrentIndexPlaying(index.row());

    setIndexColor(index);

    radioList->clearTableViewColor();
    radioList->loadPlayingStationDetails(currentStation);

    playerUIController.setFavorite(radioList->isFavorite(currentStation));
    playerUIController.setPauseIcon();

    radioList->setIsStopClicked(false);
}

int Country::getCurrentIndexPlaying() const
{
    return currentIndexPlaying;
}

void Country::setCurrentIndexPlaying(int index)
{
    currentIndexPlaying = index;
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

    if (ui)
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
    if (!countryStationsModel || !radioList)
        return;

    const QPersistentModelIndex index(ui->tableOfCoutries->indexAt(position));

    if (!index.isValid())
        return;

    if (index.row() < 0 || index.row() >= countryStationsModel->size())
        return;

    const RadioStation station = countryStationsModel->station(index.row());

    QMenu menu(ui->tableOfCoutries);

    const bool isCurrentStationPlaying = playbackController.isPlaying() && isPlaying
                                         && station.streamUrl == currentStation.streamUrl;

    QAction *playAction = menu.addAction(isCurrentStationPlaying ? tr("Stop") : tr("Play"));

    const bool isFavorite = radioList->isFavorite(station);

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
            radioList->onStopButtonClicked();
        } else {
            onDoubleListClicked(index);
        }

        return;
    }

    if (selectedAction == favoriteAction) {
        radioList->setFavorite(station, !isFavorite);
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
