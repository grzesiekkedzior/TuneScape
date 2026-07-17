#ifndef FAVORITEMANAGER_H
#define FAVORITEMANAGER_H
#include <QObject>
#include <QVector>
#include "data/RadioStation.h"

class FavoriteManager : public QObject
{
    Q_OBJECT
public:
    explicit FavoriteManager(QObject *parent = nullptr);

    bool isRadioAdded(const QString data, const QString playlist);
    bool toggleFavorite(const QString &stationName, const QString &data, const QString &playlist);
    void readFavoriteStationsFromFile(QVector<RadioStation> &stations);
    void removeRadio(const QString data, const QString playlist);
    bool isAddressExists(const QString station, const QString playlist);

private:
    const QString RADIO_BROWSER_PLAYLIST = "radiobrowser.txt";
};

#endif // FAVORITEMANAGER_H
