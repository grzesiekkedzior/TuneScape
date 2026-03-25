#ifndef TRACK_H
#define TRACK_H
#include <QList>
#include <QMap>
#include <QString>
#include <QUrl>

namespace music_data {

struct Track
{
    QString artist;
    QString title;
    QString album;
    QString releaseDate;
    QString coverUrl;
    QMap<QString, QList<QUrl>> links;

    bool isValid() const { return !artist.isEmpty() && !title.isEmpty(); }
};
} // namespace music_data
#endif // TRACK_H
