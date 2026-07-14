#ifndef RADIOSTATIONSMODEL_H
#define RADIOSTATIONSMODEL_H
#include <QAbstractTableModel>
#include <QObject>
#include "include/data/RadioStation.h"

enum class Column { StationColumn, GenreColumn, CountryColumn, HomepageColumn, ColumnCount };

class RadioStationsModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    RadioStationsModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    void setStations(const QVector<RadioStation> &stations);
    const RadioStation &station(int row) const;
    int size() const;
    bool isEmpty() const;

    const QVector<RadioStation> &stations() const;

private:
    QVector<RadioStation> m_stations;
    Column column = Column::ColumnCount;
};

#endif // RADIOSTATIONSMODEL_H
