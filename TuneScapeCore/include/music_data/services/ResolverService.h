#ifndef RESOLVERSERVICE_H
#define RESOLVERSERVICE_H
#include <QObject>
#include "include/music_data/clients/CoverArtClient.h"
#include "include/music_data/clients/MusicBrainzClient.h"
#include "include/music_data/clients/ExternalLinksClient.h"
#include "include/music_data/models/Track.h"

namespace music_data {
class ResolverService : public QObject
{
    Q_OBJECT

public:
    explicit ResolverService(QObject *parent = nullptr);
    ResolverService(MusicBrainzClient *mbc,
                    CoverArtClient *cac,
                    ExternalLinksClient *elc,
                    QObject *parent = nullptr);

    void resolveTrack(const QString &query);

signals:
    void trackReady(const Track &track);
    void error(const QString &errorMessage);

private slots:
    void handleSearchResult(const QJsonObject &json);
    void handleLookupResult(const QJsonObject &json);
    void handleCoverResult(const QJsonObject &json);
    void handleLinksResult(const QMap<QString, QList<QUrl>> &links);

private:
    MusicBrainzClient *m_musicBrainzClient;
    CoverArtClient *m_coverArtClient;
    ExternalLinksClient *m_externalLinksClient;

    struct PendingTrack
    {
        Track track;
        bool lookupReady = false;
        bool coverReady = false;
        bool linksReady = false;
    };

    PendingTrack m_pending;

    void checkReady();
};
} // namespace music_data
#endif // RESOLVERSERVICE_H
