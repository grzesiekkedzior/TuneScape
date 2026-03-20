#ifndef RESOLVERSERVICE_H
#define RESOLVERSERVICE_H
#include <QObject>
#include "music_data/clients/CoverArtClient.h"
#include "music_data/clients/MusicBrainzClient.h"
#include "music_data/clients/WikipediaClient.h"
#include "music_data/models/Track.h"

namespace music_data {
    class ResolverService : public QObject
    {
    public:
        explicit ResolverService(QObject* parent = nullptr);
        void resolveTrack(const QString& query);

    signals:
        void trackReady(const Track& track);
        void error(const QString& errorMessage);

    private:
        MusicBrainzClient* m_musicBrainzClient;
        CoverArtClient* m_coverArtClient;
        WikipediaClient* m_wikiClient;
        Track m_track;
    };
} // namespace music_data
#endif // RESOLVERSERVICE_H
