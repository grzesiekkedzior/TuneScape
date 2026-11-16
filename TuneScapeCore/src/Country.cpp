#include "include/Country.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QPersistentModelIndex>
#include <utility>
#include <QTableView>
#include "include/StationDetailsDialog.h"
#include "include/radiolist.h"
#include "include/jsonlistprocessor.h"

namespace {

constexpr auto WorldImagePath = ":/images/img/word.png";

} // namespace

void Country::setData(Ui::MainWindow *ui, RadioList *radioList)
{
    Q_ASSERT(ui);
    Q_ASSERT(radioList);

    this->ui = ui;
    this->radioList = radioList;
    catalog = &radioList->catalog();
    connect(radioList, &QObject::destroyed, this, [this] {
        delete countryStationsModel;
        countryStationsModel = nullptr;
        delete customColor.data();
        if (catalog) {
            catalog->cancel(std::exchange(countriesRequest, 0));
            catalog->cancel(std::exchange(stationsRequest, 0));
        }
        countriesLoaded = false;
        this->ui = nullptr;
        this->radioList = nullptr;
        theme = nullptr;
    });
    connect(ui->tabRadioListWidget, &QTabWidget::currentChanged, this, [this] {
        if (!this->ui || !this->radioList)
            return;
        if (this->ui->tabRadioListWidget->currentWidget() == this->ui->listCountries) {
            load();
        } else {
            cancelStationSearch();
            this->radioList->hideCatalogProgress(countriesRequest);
        }
    });

    countryStationsModel = new RadioStationsModel(this);
    ui->tableOfCoutries->setModel(countryStationsModel);
    ui->tableOfCoutries->setIconSize(QSize(20, 20));
    countryStationsModel->bindIcons(ui->tableOfCoutries, &radioList->stationImages());

    connect(ui->comboBox, &QComboBox::textActivated, this, &Country::searchCountry);

    connect(ui->tableOfCoutries, &QTableView::doubleClicked, this, &Country::playIndex);
    connect(ui->tableOfCoutries, &QTableView::activated, this, &Country::playIndex);
    connect(countryStationsModel, &RadioStationsModel::modelReset, this, &Country::updateThemeAppearance);
    connect(&radioList->playback(), &PlaybackController::sessionChanged, this, &Country::updateThemeAppearance);

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
        countriesRequest = catalog->request(QUrl("json/countries"), this,
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
    countryStationsModel->setStations({});

    const auto countryCode = ui->comboBox->currentData().toString();
    const QUrl endpoint = countryCode.isEmpty()
        ? RadioBrowserApi::endpoint("json/stations/search", "country", country)
        : RadioBrowserApi::endpoint("json/stations/search", "countrycode", countryCode);
    stationsRequest = catalog->request(endpoint, this,
        [this](auto id, const auto &result) {
            if (id != stationsRequest)
                return;
            stationsRequest = 0;
            radioList->showCatalogResult(id, result);
            if (result.status == RadioBrowserApi::Status::Success) {
                countryStationsModel->setStations(JsonListProcessor::stationsFromJson(result.data));
            }
        });
    radioList->showCatalogProgress(stationsRequest, tr("Loading stations for %1…").arg(country));
}

void Country::playIndex(const QModelIndex &index)
{
    if (radioList && countryStationsModel && index.isValid()
        && index.model() == countryStationsModel
        && index.row() >= 0 && index.row() < countryStationsModel->size()) {
        radioList->playback().play(countryStationsModel->station(index.row()));
    }
}

void Country::setTheme(Theme *newTheme)
{
    theme = newTheme;
    updateThemeAppearance();
}

void Country::updateThemeAppearance()
{
    if (!theme || !countryStationsModel || !radioList)
        return;
    if (!customColor) {
        customColor = new CustomColorDelegate(-1, theme->playingRowColor(), ui->tableOfCoutries);
        ui->tableOfCoutries->setItemDelegate(customColor);
    }
    const auto &playback = radioList->playback();
    const auto &station = playback.station();
    const int row = playback.hasActiveSession() && station
        ? countryStationsModel->rowForStation(*station) : -1;
    customColor->setPlayingRow(row, theme->playingRowColor());
    ui->tableOfCoutries->viewport()->update();
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

    auto &playback = radioList->playback();
    const bool activeStation = playback.hasActiveSession() && playback.matches(station);
    const auto generation = playback.generation();

    QAction *playAction = menu.addAction(activeStation ? tr("Stop") : tr("Play"));

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
        if (activeStation) {
            if (playback.generation() == generation && playback.matches(station))
                playback.stop();
        } else if (index.isValid()) {
            playback.play(station);
        }

        return;
    }

    if (selectedAction == favoriteAction) {
        radioList->setFavorite(station, !isFavorite);
        return;
    }

    if (selectedAction == detailsAction) {
        StationDetailsDialog dialog(station, radioList->stationImages(), ui->tableOfCoutries);

        dialog.exec();
        return;
    }

    if (selectedAction == openHomepageAction) {
        QDesktopServices::openUrl(QUrl(station.homepage));
    }
}
