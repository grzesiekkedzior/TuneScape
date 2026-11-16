#ifndef FAVORITEMANAGER_H
#define FAVORITEMANAGER_H

#include <QObject>
#include <QVector>
#include "data/RadioStation.h"

class FavoriteManager : public QObject
{
    Q_OBJECT
public:
    enum class Format { Native, M3U };

    explicit FavoriteManager(QObject *parent = nullptr);
    explicit FavoriteManager(const QString &filePath, QObject *parent = nullptr);

    // An empty error string means success. Output playlists are replaced only on success.
    QString load(QVector<RadioStation> &stations) const;
    QString setFavorite(const RadioStation &station, bool favorite,
                        QVector<RadioStation> &stations) const;
    QString importPlaylist(const QString &source, QVector<RadioStation> &stations) const;
    QString exportPlaylist(const QString &destination, Format format) const;

    static bool sameStation(const RadioStation &first, const RadioStation &second);

private:
    QString m_filePath;
};

#endif // FAVORITEMANAGER_H
