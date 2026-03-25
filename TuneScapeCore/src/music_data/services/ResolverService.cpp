#include "include/music_data/services/ResolverService.h"
#include <QJsonArray>

music_data::ResolverService::ResolverService(QObject *parent) {}

music_data::ResolverService::ResolverService(MusicBrainzClient *mbc,
                                             CoverArtClient *cac,
                                             ExternalLinksClient *elc,
                                             QObject *parent)
    : QObject(parent)
    , m_musicBrainzClient(mbc)
    , m_coverArtClient(cac)
    , m_externalLinksClient(elc)
{
    mbc->setParent(this);
    cac->setParent(this);
    elc->setParent(this);

    connect(m_musicBrainzClient,
            &MusicBrainzClient::searchFinished,
            this,
            &ResolverService::handleSearchResult);

    connect(m_musicBrainzClient,
            &MusicBrainzClient::lookupFinished,
            this,
            &ResolverService::handleLookupResult);

    connect(m_coverArtClient,
            &CoverArtClient::coverFinished,
            this,
            &ResolverService::handleCoverResult);

    connect(m_externalLinksClient,
            &ExternalLinksClient::linksReady,
            this,
            &ResolverService::handleLinksResult);
}

void music_data::ResolverService::resolveTrack(const QString &query)
{
    m_pending = PendingTrack{};
    m_musicBrainzClient->searchRecording(query);
}

void music_data::ResolverService::handleSearchResult(const QJsonObject &json)
{
    auto recordings = json.value("recordings").toArray();

    if (recordings.isEmpty()) {
        qDebug() << "No recordings found";
        return;
    }

    auto first = recordings[0].toObject();

    QString mbid = first.value("id").toString();

    if (mbid.isEmpty()) {
        qDebug() << "No MBID found";
        return;
    }

    qDebug() << "Recording MBID:" << mbid;

    m_musicBrainzClient->getRecordingByID(mbid);
}

void music_data::ResolverService::handleLookupResult(const QJsonObject &json)
{
    QString title = json.value("title").toString();

    QString artist;
    QString artistMbid;

    auto artistCredit = json.value("artist-credit").toArray();

    for (const auto &item : artistCredit) {
        auto obj = item.toObject();
        auto artistObj = obj.value("artist").toObject();

        QString name = obj.value("name").toString();
        if (name.isEmpty())
            name = artistObj.value("name").toString();

        QString mbid = artistObj.value("id").toString();

        if (!name.isEmpty()) {
            artist = name;
            artistMbid = mbid;
            break;
        }
    }

    QString releaseId;
    QString album;
    QString date;

    auto releases = json.value("releases").toArray();

    if (!releases.isEmpty()) {
        auto first = releases[0].toObject();

        releaseId = first.value("id").toString();
        album = first.value("title").toString();
        date = first.value("date").toString();
    }

    m_pending.track.title = title;
    m_pending.track.artist = artist;
    m_pending.track.album = album;
    m_pending.track.releaseDate = date;

    m_pending.lookupReady = true;

    // ===== COVER =====
    if (!releaseId.isEmpty()) {
        m_coverArtClient->getCover(releaseId);
    } else {
        m_pending.coverReady = true;
    }

    // ===== LINKS =====
    if (!artistMbid.isEmpty()) {
        m_externalLinksClient->getInfoByMbid(artistMbid);
    } else {
        m_pending.linksReady = true;
    }

    checkReady();
}

void music_data::ResolverService::handleCoverResult(const QJsonObject &json)
{
    auto images = json.value("images").toArray();

    if (!images.isEmpty()) {
        auto first = images[0].toObject();
        QString coverUrl = first.value("image").toString();

        m_pending.track.coverUrl = coverUrl;
    } else {
        qDebug() << "No cover found";
    }

    m_pending.coverReady = true;
    checkReady();
}

void music_data::ResolverService::handleLinksResult(const QMap<QString, QList<QUrl> > &links)
{
    qDebug() << "LINKS";

    for (auto it = links.begin(); it != links.end(); ++it) {
        const QString &type = it.key();
        const QList<QUrl> &urls = it.value();

        qDebug() << "Type:" << type;

        for (const QUrl &url : urls) {
            qDebug() << "  -" << url.toString();
        }
    }

    m_pending.track.links = links;
    m_pending.linksReady = true;

    checkReady();
}

void music_data::ResolverService::checkReady()
{
    qDebug() << "\n=== PENDING STATE ===";
    qDebug() << "lookupReady:" << m_pending.lookupReady;
    qDebug() << "coverReady:" << m_pending.coverReady;
    qDebug() << "linksReady:" << m_pending.linksReady;

    qDebug() << "\n--- TRACK DATA ---";
    qDebug() << "Artist:" << m_pending.track.artist;
    qDebug() << "Title:" << m_pending.track.title;
    qDebug() << "Album:" << m_pending.track.album;
    qDebug() << "ReleaseDate:" << m_pending.track.releaseDate;
    qDebug() << "Cover URL:" << m_pending.track.coverUrl.toStdString();

    qDebug() << "\n--- LINKS ---";
    if (m_pending.track.links.isEmpty()) {
        qDebug() << "No links";
    } else {
        for (auto it = m_pending.track.links.begin(); it != m_pending.track.links.end(); ++it) {
            const QString &type = it.key();
            const QList<QUrl> &urls = it.value();

            qDebug() << "Type:" << type;

            for (const QUrl &url : urls) {
                qDebug() << "  -" << url.toString();
            }
        }
    }

    qDebug() << "====================\n";

    if (m_pending.lookupReady && m_pending.coverReady && m_pending.linksReady) {
        qDebug() << "=== TRACK READY ===";

        emit trackReady(m_pending.track);

        m_pending = PendingTrack{};
    }
}