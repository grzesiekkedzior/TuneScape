#ifndef RADIOSTATIONSMODEL_H
#define RADIOSTATIONSMODEL_H
#include <QAbstractTableModel>
#include <QObject>
#include "include/IconLoader.h"
#include "include/data/RadioStation.h"

class QTableView;

enum class Column { StationColumn, GenreColumn, CountryColumn, HomepageColumn, ColumnCount };

class RadioStationsModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    RadioStationsModel(QObject *parent = nullptr);
    ~RadioStationsModel() override;

    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    void setStations(const QVector<RadioStation> &stations);
    const RadioStation &station(int row) const;
    int rowForStation(const RadioStation &station) const;
    int size() const;
    bool isEmpty() const;
    void bindIcons(QTableView *view, IconLoader *loader);

    const QVector<RadioStation> &stations() const;

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    struct VisibleIcon {
        IconLoader::RequestId request = 0;
        QIcon icon;
    };

    void scheduleIcons();
    void updateVisibleIcons();
    void clearIcons();

    QVector<RadioStation> m_stations;
    QPointer<QTableView> m_view;
    QPointer<IconLoader> m_loader;
    QHash<int, VisibleIcon> m_icons;
    QTimer m_iconUpdate;
    quint64 m_generation = 0;
    qreal m_dpr = 0;
    QSize m_iconSize;
};

#endif // RADIOSTATIONSMODEL_H
