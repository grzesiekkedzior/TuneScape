#include "include/model/RadioStationsModel.h"

RadioStationsModel::RadioStationsModel(QObject *parent)
    : QAbstractTableModel(parent)
{}

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
            if (!station.icon.isNull())
                return station.icon;

            return QIcon(QStringLiteral(":/images/img/radio96x96.png"));
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
        return station.homepage;

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
    beginResetModel();

    m_stations = stations;

    endResetModel();
}

const RadioStation &RadioStationsModel::station(int row) const
{
    return m_stations.at(row);
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

void RadioStationsModel::setStationIcon(int row, const QIcon &icon)
{
    qDebug() << "setStationIcon row:" << row << "isNull:" << icon.isNull();

    if (row < 0 || row >= m_stations.size()) {
        qDebug() << "Invalid row, model size:" << m_stations.size();
        return;
    }

    m_stations[row].icon = icon;

    const QModelIndex changedIndex = index(row, static_cast<int>(Column::StationColumn));

    emit dataChanged(changedIndex, changedIndex, {Qt::DecorationRole});
}
