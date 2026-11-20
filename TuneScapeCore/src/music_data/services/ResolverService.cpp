#include "include/music_data/services/ResolverService.h"
#include <QJsonArray>
#include <QUuid>

using namespace music_data;
namespace {
QString key(const QString &value) { return value.normalized(QString::NormalizationForm_C).simplified().toCaseFolded(); }
bool mbid(const QString &value) { return !QUuid(value).isNull(); }
QString artist(const QJsonObject &json)
{
    QString name;
    for (const auto &credit : json["artist-credit"].toArray()) {
        const auto object = credit.toObject();
        name += object["name"].toString(object["artist"].toObject()["name"].toString()) + object["joinphrase"].toString();
    }
    return name.trimmed();
}
}

ResolverService::ResolverService(MusicBrainzClient *mbc, CoverArtClient *cac, ExternalLinksClient *elc, QObject *parent)
    : QObject(parent), m_musicBrainzClient(mbc), m_coverArtClient(cac), m_externalLinksClient(elc)
{
    mbc->setParent(this);
    cac->setParent(this);
    elc->setParent(this);
    elc->useQueue(&mbc->requests(), mbc->baseUrl());
    connect(mbc, &MusicBrainzClient::searchFinished, this, &ResolverService::handleSearchResult);
    connect(mbc, &MusicBrainzClient::lookupFinished, this, &ResolverService::handleLookupResult);
    connect(cac, &CoverArtClient::coverFinished, this, &ResolverService::handleCoverResult);
    connect(elc, &ExternalLinksClient::linksReady, this, &ResolverService::handleLinksResult);
    connect(mbc, &MusicBrainzClient::requestFailed, this, [this](auto token, const auto &message) {
        if (token == m_generation) emit error(message);
    });
}
ResolverService::~ResolverService()
{
    m_musicBrainzClient->cancel();
    m_coverArtClient->cancel();
}
void ResolverService::cancel()
{
    ++m_generation;
    m_stage = Stage::Idle;
    m_key.clear();
    m_track = {};
    m_musicBrainzClient->cancel();
    m_coverArtClient->cancel();
    emit cleared();
}
void ResolverService::resolveTrack(const QString &query)
{
    const auto data = MusicBrainzClient::extractData(query);
    const auto next = key(data.artist) + QChar(0x1f) + key(data.title);
    if (!m_key.isEmpty() && m_key == next) return;
    cancel();
    if (!MusicBrainzClient::isValidTrack(data)) return;
    m_query = data;
    m_key = next;
    m_track.artist = data.artist;
    m_track.title = data.title;
    emit trackReady(m_track); // Keep trustworthy stream text even if enrichment fails.
    m_stage = Stage::Search;
    m_musicBrainzClient->searchRecording(query, m_generation);
}
void ResolverService::handleSearchResult(quint64 token, const QJsonObject &json)
{
    if (token != m_generation || m_stage != Stage::Search) return;
    m_stage = Stage::Idle;
    QString selected;
    for (const auto &value : json["recordings"].toArray()) {
        const auto recording = value.toObject();
        const auto id = recording["id"].toString();
        if (mbid(id) && key(recording["title"].toString()) == key(m_query.title)
            && (m_query.artist.isEmpty() || key(artist(recording)) == key(m_query.artist))) {
            if (!selected.isEmpty() && selected != id) return; // Ambiguous: retain ICY instead of guessing an album.
            selected = id;
        }
    }
    if (!selected.isEmpty()) {
        m_recording = selected;
        m_stage = Stage::Lookup;
        m_musicBrainzClient->getRecordingByID(selected, token);
    }
}
void ResolverService::handleLookupResult(quint64 token, const QJsonObject &json)
{
    if (token != m_generation || m_stage != Stage::Lookup) return;
    m_stage = Stage::Idle;
    if (json["id"].toString() != m_recording || key(json["title"].toString()) != key(m_query.title)) return;
    const auto name = artist(json);
    if (!m_query.artist.isEmpty() && key(name) != key(m_query.artist)) return;
    m_track.title = json["title"].toString();
    m_track.artist = name;
    const auto releases = json["releases"].toArray();
    const auto release = releases.isEmpty() ? QJsonObject{} : releases.first().toObject();
    m_track.album = release["title"].toString();
    m_track.releaseDate = release["date"].toString();
    const auto releaseId = release["id"].toString();
    const auto credits = json["artist-credit"].toArray();
    const auto artistId = credits.isEmpty() ? QString() : credits.first().toObject()["artist"].toObject()["id"].toString();
    m_coverReady = !mbid(releaseId);
    m_linksReady = !mbid(artistId);
    m_stage = Stage::Enrich;
    if (!m_coverReady) m_coverArtClient->getCover(releaseId, token);
    if (!m_linksReady) m_externalLinksClient->getInfoByMbid(artistId, token);
    checkReady();
}
void ResolverService::handleCoverResult(quint64 token, const QJsonObject &json)
{
    if (token != m_generation || m_stage != Stage::Enrich || m_coverReady) return;
    const auto images = json["images"].toArray();
    QJsonObject chosen;
    for (const auto &value : images) {
        const auto image = value.toObject();
        if (chosen.isEmpty() || image["front"].toBool()) chosen = image;
        if (image["front"].toBool()) break;
    }
    const auto thumbs = chosen["thumbnails"].toObject();
    const QUrl url(thumbs["250"].toString(chosen["image"].toString()));
    if (url.isValid() && (url.scheme() == "http" || url.scheme() == "https")) m_track.coverUrl = url.toString();
    m_coverReady = true;
    checkReady();
}
void ResolverService::handleLinksResult(quint64 token, const QMap<QString, QList<QUrl>> &links)
{
    if (token != m_generation || m_stage != Stage::Enrich || m_linksReady) return;
    m_track.links = links;
    m_linksReady = true;
    checkReady();
}
void ResolverService::checkReady()
{
    if (m_stage == Stage::Enrich && m_coverReady && m_linksReady) {
        m_stage = Stage::Idle;
        emit trackReady(m_track);
    }
}
