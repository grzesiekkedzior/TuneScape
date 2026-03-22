#ifndef RESOLVERSERVICE_H
#define RESOLVERSERVICE_H
#include <QObject>
#include "include/music_data/clients/CoverArtClient.h"
#include "include/music_data/clients/MusicBrainzClient.h"
#include "include/music_data/clients/WikipediaClient.h"
#include "include/music_data/models/Track.h"

namespace music_data {
    class ResolverService : public QObject
    {
    public:
        explicit ResolverService(QObject* parent = nullptr);
        ResolverService(MusicBrainzClient *mbc,
                        CoverArtClient *cac,
                        //WikipediaClient *wpc,
                        QObject *parent = nullptr);
        void resolveTrack(const QString &query);

    signals:
        void trackReady(const Track& track);
        void error(const QString& errorMessage);

    private slots:
        void handleSearchResult(const QJsonObject &json);
        void handleLookupResult(const QJsonObject &json);
        void handleCoverResult(const QJsonObject &json);

    private:
        MusicBrainzClient *m_musicBrainzClient;
        CoverArtClient* m_coverArtClient;
        WikipediaClient* m_wikiClient;
        Track m_track;
    };
} // namespace music_data
#endif // RESOLVERSERVICE_H
