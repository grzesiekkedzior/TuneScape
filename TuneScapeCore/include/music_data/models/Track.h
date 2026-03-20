#ifndef TRACK_H
#define TRACK_H
#include <QString>

namespace music_data {

struct Track
{
    QString artist;
    QString title;
    QString album;
    QString releaseDate;
    QString coverUrl;
    QString description;

    bool isValid() const { return !artist.isEmpty() && !title.isEmpty(); }
};
} // namespace music_data
#endif // TRACK_H
