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

    if (role != Qt::DisplayRole)
        return {};

    const RadioStation &station = m_stations.at(index.row());

    switch (static_cast<Column>(index.column())) {
    case Column::StationColumn:
        return station.station;

    case Column::GenreColumn:
        return station.genre;

    case Column::CountryColumn:
        return station.country;

    case Column::HomepageColumn:
        return station.homepage;

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
        return "Station";

    case Column::GenreColumn:
        return "Genre";

    case Column::CountryColumn:
        return "Country";

    case Column::HomepageColumn:
        return "Homepage";

    default:
        return {};
    }
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
