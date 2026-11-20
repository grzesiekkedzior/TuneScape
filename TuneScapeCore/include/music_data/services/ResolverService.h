#ifndef RESOLVERSERVICE_H
#define RESOLVERSERVICE_H
#include "include/music_data/clients/CoverArtClient.h"
#include "include/music_data/clients/MusicBrainzClient.h"
#include "include/music_data/clients/ExternalLinksClient.h"
#include "include/music_data/models/Track.h"
namespace music_data {
class ResolverService : public QObject
{
    Q_OBJECT
public:
    ResolverService(MusicBrainzClient *mbc, CoverArtClient *cac, ExternalLinksClient *elc, QObject *parent = nullptr);
    ~ResolverService() override;
    void resolveTrack(const QString &query);
    void cancel();
signals:
    void trackReady(const Track &track);
    void cleared();
    void error(const QString &message);
private:
    void handleSearchResult(quint64 token, const QJsonObject &json);
    void handleLookupResult(quint64 token, const QJsonObject &json);
    void handleCoverResult(quint64 token, const QJsonObject &json);
    void handleLinksResult(quint64 token, const QMap<QString, QList<QUrl>> &links);
    void checkReady();
    MusicBrainzClient *m_musicBrainzClient;
    CoverArtClient *m_coverArtClient;
    ExternalLinksClient *m_externalLinksClient;
    enum class Stage { Idle, Search, Lookup, Enrich };
    Stage m_stage = Stage::Idle;
    quint64 m_generation = 0;
    QString m_key;
    QString m_recording;
    QueryData m_query;
    Track m_track;
    bool m_coverReady = false;
    bool m_linksReady = false;
};
}
#endif
