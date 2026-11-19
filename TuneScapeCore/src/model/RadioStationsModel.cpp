#include "include/model/RadioStationsModel.h"
#include "include/FavoriteManager.h"
#include <algorithm>
#include <QEvent>
#include <QHeaderView>
#include <QScrollBar>
#include <QScreen>
#include <QTableView>
#include <QAbstractProxyModel>
#include <QSet>
#include <QUrl>
#include <QWindow>

RadioStationsModel::RadioStationsModel(QObject *parent)
    : QAbstractTableModel(parent)
{
    m_iconUpdate.setSingleShot(true);
    connect(&m_iconUpdate, &QTimer::timeout, this, &RadioStationsModel::updateVisibleIcons);
}

RadioStationsModel::~RadioStationsModel()
{
    clearIcons();
}

std::optional<RadioStation> RadioStationsModel::stationAt(QModelIndex index)
{
    while (index.isValid()) {
        if (const auto *proxy = qobject_cast<const QAbstractProxyModel *>(index.model())) {
            index = proxy->mapToSource(index);
            continue;
        }
        const auto *model = qobject_cast<const RadioStationsModel *>(index.model());
        if (model && index.row() >= 0 && index.row() < model->size())
            return model->station(index.row());
        break;
    }
    return {};
}

int RadioStationsModel::viewRowForStation(const QAbstractItemModel *model, const RadioStation &station)
{
    if (model) {
        for (int row = 0; row < model->rowCount(); ++row) {
            const auto candidate = stationAt(model->index(row, 0));
            if (candidate && FavoriteManager::sameStation(*candidate, station))
                return row;
        }
    }
    return -1;
}

int RadioStationsModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return m_stations.size();
}

int RadioStationsModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return static_cast<int>(Column::ColumnCount);
}

QVariant RadioStationsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};

    if (index.row() < 0 || index.row() >= m_stations.size())
        return {};

    const RadioStation &station = m_stations.at(index.row());

    if (index.column() == static_cast<int>(Column::StationColumn)) {
        if (role == Qt::DecorationRole) {
            const auto visible = m_icons.constFind(index.row());
            if (visible != m_icons.cend() && !visible->icon.isNull())
                return visible->icon;
            if (!station.icon.isNull())
                return station.icon;

            static const QIcon fallback(QStringLiteral(":/images/img/radio96x96.png"));
            return fallback;
        }

        if (role == Qt::DisplayRole)
            return station.station;
    }

    if (role != Qt::DisplayRole)
        return {};

    switch (static_cast<Column>(index.column())) {
    case Column::GenreColumn:
        return station.genre;

    case Column::CountryColumn:
        return station.country;

    case Column::HomepageColumn:
        return QUrl(station.homepage).host();

    case Column::StationColumn:
    case Column::ColumnCount:
        break;
    }

    return {};
}

QVariant RadioStationsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return {};

    if (orientation != Qt::Horizontal)
        return {};

    switch (static_cast<Column>(section)) {
    case Column::StationColumn:
        return QStringLiteral("Station");

    case Column::GenreColumn:
        return QStringLiteral("Genre");

    case Column::CountryColumn:
        return QStringLiteral("Country");

    case Column::HomepageColumn:
        return QStringLiteral("Homepage");

    case Column::ColumnCount:
        break;
    }

    return {};
}

void RadioStationsModel::setStations(const QVector<RadioStation> &stations)
{
    ++m_generation;
    clearIcons();
    beginResetModel();

    m_stations = stations;

    endResetModel();
    scheduleIcons();
}

const RadioStation &RadioStationsModel::station(int row) const
{
    return m_stations.at(row);
}

int RadioStationsModel::rowForStation(const RadioStation &station) const
{
    const auto it = std::find_if(m_stations.cbegin(), m_stations.cend(), [&station](const auto &entry) {
        return FavoriteManager::sameStation(station, entry);
    });
    return it != m_stations.cend() ? int(it - m_stations.cbegin()) : -1;
}

int RadioStationsModel::size() const
{
    return m_stations.size();
}

bool RadioStationsModel::isEmpty() const
{
    return m_stations.isEmpty();
}

const QVector<RadioStation> &RadioStationsModel::stations() const
{
    return m_stations;
}

void RadioStationsModel::bindIcons(QTableView *view, IconLoader *loader)
{
    m_view = view;
    m_loader = loader;
    view->installEventFilter(this);
    view->viewport()->installEventFilter(this);
    connect(view->verticalScrollBar(), &QScrollBar::valueChanged, this, &RadioStationsModel::scheduleIcons);
    connect(view->verticalHeader(), &QHeaderView::sectionResized, this, &RadioStationsModel::scheduleIcons);
    connect(view, &QTableView::iconSizeChanged, this, &RadioStationsModel::scheduleIcons);
    connect(view, &QObject::destroyed, this, &RadioStationsModel::clearIcons);
    scheduleIcons();
}

bool RadioStationsModel::eventFilter(QObject *watched, QEvent *event)
{
    switch (event->type()) {
    case QEvent::Show:
        if (m_view) {
            if (auto *window = m_view->window()->windowHandle()) {
                connect(window, &QWindow::screenChanged, this, &RadioStationsModel::scheduleIcons,
                        Qt::UniqueConnection);
            }
        }
        scheduleIcons();
        break;
    case QEvent::Resize:
    case QEvent::Hide:
    case QEvent::Paint:
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
    case QEvent::DevicePixelRatioChange:
#endif
        scheduleIcons();
        break;
    default:
        break;
    }
    return QAbstractTableModel::eventFilter(watched, event);
}

void RadioStationsModel::scheduleIcons()
{
    if (m_view)
        m_iconUpdate.start(0);
}

void RadioStationsModel::clearIcons()
{
    if (m_loader) {
        for (const auto &entry : m_icons)
            m_loader->cancel(entry.request);
    }
    m_icons.clear();
}

void RadioStationsModel::updateVisibleIcons()
{
    if (!m_view || !m_loader || !m_view->model() || !m_view->isVisible() || m_stations.isEmpty()) {
        clearIcons();
        return;
    }
    const auto iconSize = m_view->iconSize();
    const auto dpr = m_view->devicePixelRatioF();
    if (m_dpr != dpr || m_iconSize != iconSize) {
        clearIcons();
        m_dpr = dpr;
        m_iconSize = iconSize;
    }
    const int first = m_view->rowAt(0);
    const int bottomRow = m_view->rowAt(m_view->viewport()->height() - 1);
    const int last = bottomRow < 0 ? m_view->model()->rowCount() - 1
                                  : qMin(bottomRow, m_view->model()->rowCount() - 1);
    QSet<int> visibleRows;
    for (int row = first; first >= 0 && row <= last; ++row) {
        if (m_view->isRowHidden(row))
            continue;
        auto index = m_view->model()->index(row, 0);
        while (auto *proxy = qobject_cast<const QAbstractProxyModel *>(index.model()))
            index = proxy->mapToSource(index);
        if (index.isValid() && index.model() == this)
            visibleRows.insert(index.row());
    }
    for (auto it = m_icons.begin(); it != m_icons.end();) {
        if (!visibleRows.contains(it.key())) {
            m_loader->cancel(it->request);
            it = m_icons.erase(it);
        } else {
            ++it;
        }
    }
    if (first < 0)
        return;
    for (int row : visibleRows) {
        const auto &station = m_stations.at(row);
        if (station.iconUrl.isEmpty() || m_icons.contains(row))
            continue;
        const QPersistentModelIndex target(index(row, static_cast<int>(Column::StationColumn)));
        const auto generation = m_generation;
        const auto request = m_loader->request(QUrl(station.iconUrl), iconSize, dpr, this,
            [this, target, generation, uuid = station.stationUuid, stream = station.streamUrl,
             url = station.iconUrl](auto id, const QPixmap &pixmap) {
                if (!target.isValid() || generation != m_generation)
                    return;
                auto entry = m_icons.find(target.row());
                if (entry == m_icons.end() || entry->request != id)
                    return;
                entry->request = 0;
                const auto &current = m_stations.at(target.row());
                if (pixmap.isNull() || current.stationUuid != uuid
                    || current.streamUrl != stream || current.iconUrl != url)
                    return;
                entry->icon = QIcon(pixmap);
                emit dataChanged(target, target, {Qt::DecorationRole});
            });
        m_icons.insert(row, {request, {}});
    }
}
