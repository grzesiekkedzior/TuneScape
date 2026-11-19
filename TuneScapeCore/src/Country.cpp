#include "include/Country.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QTableView>
#include <utility>
#include "include/radiolist.h"
#include "include/jsonlistprocessor.h"
#include "ui_mainwindow.h"

Country::~Country()
{
    detach();
}

void Country::detach()
{
    for (const auto &connection : connections)
        disconnect(connection);
    connections.clear();
    const auto countryId = std::exchange(countriesRequest, 0);
    const auto stationId = std::exchange(stationsRequest, 0);
    if (radioList) {
        radioList->hideCatalogProgress(countryId);
        radioList->hideCatalogProgress(stationId);
    }
    if (catalog) {
        catalog->cancel(countryId);
        catalog->cancel(stationId);
    }
    if (stationView) {
        if (stationView->model() == countryStationsModel)
            stationView->setModel(nullptr);
        if (customColor && stationView->itemDelegate() == customColor)
            stationView->setItemDelegate(nullptr);
    }
    delete customColor.data();
    delete std::exchange(countryStationsModel, nullptr);
    selector.clear();
    stationView.clear();
    worldImage.clear();
    tabs.clear();
    countryTab.clear();
    radioList.clear();
    catalog.clear();
    countriesLoaded = false;
}

void Country::setData(Ui::MainWindow *ui, RadioList *list)
{
    Q_ASSERT(ui && list);
    if (stationView == ui->tableOfCoutries && radioList == list)
        return;
    detach();
    selector = ui->comboBox;
    stationView = ui->tableOfCoutries;
    worldImage = ui->worldImage;
    tabs = ui->tabRadioListWidget;
    countryTab = ui->listCountries;
    radioList = list;
    catalog = &list->catalog();
    connections.append(connect(list, &QObject::destroyed, this, [this] {
        radioList.clear(); // The owner's UI helpers have already been destroyed.
        detach();
    }));
    connections.append(connect(stationView, &QObject::destroyed, this, [this] {
        // QWidget emits destroyed while its derived table interface is already gone.
        stationView.clear();
        detach();
    }));
    connections.append(connect(tabs, &QTabWidget::currentChanged, this, [this] {
        if (!tabs || !radioList || !stationView)
            return;
        if (tabs->currentWidget() == countryTab) {
            load();
        } else {
            cancelStationSearch();
            radioList->hideCatalogProgress(countriesRequest);
        }
    }));
    countryStationsModel = new RadioStationsModel(this);
    stationView->setModel(countryStationsModel);
    stationView->setIconSize(QSize(20, 20));
    countryStationsModel->bindIcons(stationView, &list->stationImages());
    connections.append(connect(selector, &QComboBox::textActivated, this, &Country::searchCountry));
    connections.append(connect(stationView, &QTableView::doubleClicked, this, &Country::playIndex));
    connections.append(connect(stationView, &QTableView::activated, this, &Country::playIndex));
    connections.append(connect(countryStationsModel, &RadioStationsModel::modelReset, this, &Country::updateThemeAppearance));
    connections.append(connect(&list->playback(), &PlaybackController::sessionChanged, this, &Country::updateThemeAppearance));
    connections.append(connect(stationView, &QTableView::customContextMenuRequested, this, &Country::showStationContextMenu));
    worldImage->setPixmap(QPixmap(":/images/img/word.png"));
    worldImage->setAlignment(Qt::AlignCenter);
    stationView->setVisible(false);
    stationView->setContextMenuPolicy(Qt::CustomContextMenu);
    stationView->verticalHeader()->setDefaultSectionSize(30);
    auto *header = stationView->horizontalHeader();
    header->setSectionResizeMode(static_cast<int>(Column::StationColumn), QHeaderView::Stretch);
    header->setSectionResizeMode(static_cast<int>(Column::GenreColumn), QHeaderView::ResizeToContents);
    updateThemeAppearance();
}

void Country::load()
{
    if (!radioList || !catalog || !stationView || countriesLoaded)
        return;
    if (!countriesRequest) {
        countriesRequest = catalog->request(QUrl("json/countries"), this,
            [this](auto id, const auto &result) {
                if (id != countriesRequest || !radioList || !selector)
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
    if (tabs->currentWidget() == countryTab)
        radioList->showCatalogProgress(countriesRequest, tr("Loading Radio Browser countries…"));
}

void Country::loadCountriesToComboBox(const QJsonArray &countries)
{
    selector->clear();
    for (const auto &entry : countries) {
        const auto object = entry.toObject();
        const auto name = object["name"].toString();
        if (!name.isEmpty())
            selector->addItem(name, object["iso_3166_1"].toString());
    }
}

void Country::cancelStationSearch()
{
    const auto id = std::exchange(stationsRequest, 0);
    if (radioList)
        radioList->hideCatalogProgress(id);
    if (catalog)
        catalog->cancel(id);
}

void Country::searchCountry(const QString &country)
{
    if (!radioList || !catalog || !stationView)
        return;
    cancelStationSearch();
    stationView->show();
    worldImage->hide();
    countryStationsModel->setStations({});
    const auto countryCode = selector->currentData().toString();
    const auto endpoint = countryCode.isEmpty()
        ? RadioBrowserApi::endpoint("json/stations/search", "country", country)
        : RadioBrowserApi::endpoint("json/stations/search", "countrycode", countryCode);
    stationsRequest = catalog->request(endpoint, this,
        [this](auto id, const auto &result) {
            if (id != stationsRequest || !radioList || !countryStationsModel)
                return;
            stationsRequest = 0;
            radioList->showCatalogResult(id, result);
            if (result.status == RadioBrowserApi::Status::Success)
                countryStationsModel->setStations(JsonListProcessor::stationsFromJson(result.data));
        });
    radioList->showCatalogProgress(stationsRequest, tr("Loading stations for %1…").arg(country));
}

void Country::playIndex(const QModelIndex &index)
{
    if (radioList && stationView && index.model() == stationView->model()) {
        if (auto connection = connect(index.model(), &QAbstractItemModel::layoutChanged, this, &Country::updateThemeAppearance, Qt::UniqueConnection))
            connections.append(connection);
        if (auto connection = connect(index.model(), &QAbstractItemModel::modelReset, this, &Country::updateThemeAppearance, Qt::UniqueConnection))
            connections.append(connection);
        if (auto connection = connect(index.model(), &QAbstractItemModel::rowsRemoved, this, &Country::updateThemeAppearance, Qt::UniqueConnection))
            connections.append(connection);
        if (auto connection = connect(index.model(), &QAbstractItemModel::rowsInserted, this, &Country::updateThemeAppearance, Qt::UniqueConnection))
            connections.append(connection);
        if (const auto station = RadioStationsModel::stationAt(index))
            radioList->playback().play(*station);
    }
}

void Country::setTheme(Theme *newTheme)
{
    theme = newTheme;
    updateThemeAppearance();
}

void Country::updateThemeAppearance()
{
    if (!theme || !countryStationsModel || !radioList || !stationView)
        return;
    if (!customColor) {
        customColor = new CustomColorDelegate(-1, theme->playingRowColor(), stationView);
        stationView->setItemDelegate(customColor);
    }
    const auto &playback = radioList->playback();
    const auto &station = playback.station();
    const int row = playback.hasActiveSession() && station
        ? RadioStationsModel::viewRowForStation(stationView->model(), *station) : -1;
    customColor->setPlayingRow(row, theme->playingRowColor());
    stationView->viewport()->update();
}

void Country::showStationContextMenu(const QPoint &position)
{
    if (radioList && stationView)
        radioList->showStationMenu(stationView, position);
}
