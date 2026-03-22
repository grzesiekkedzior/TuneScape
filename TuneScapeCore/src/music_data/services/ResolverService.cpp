#include "include/music_data/services/ResolverService.h"
#include <QJsonArray>

music_data::ResolverService::ResolverService(QObject *parent) {}

music_data::ResolverService::ResolverService(MusicBrainzClient *mbc,
                                             CoverArtClient *cac,
                                             //WikipediaClient *wpc,
                                             QObject *parent)
    : QObject(parent)
    , m_musicBrainzClient(mbc)
    , m_coverArtClient(cac)
//, m_wikiClient(wpc)
{
    mbc->setParent(this);
    cac->setParent(this);
    //wpc->setParent(this);

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
}

void music_data::ResolverService::resolveTrack(const QString &query)
{
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
    //TITLE
    QString title = json.value("title").toString();

    //ARTIST
    QString artist;
    auto artistCredit = json.value("artist-credit").toArray();

    for (const auto &item : artistCredit) {
        auto obj = item.toObject();

        QString name = obj.value("name").toString();

        if (name.isEmpty()) {
            name = obj.value("artist").toObject().value("name").toString();
        }

        if (!name.isEmpty()) {
            artist = name;
            break;
        }
    }

    //RELEASE (album + date + mbid)
    auto releases = json.value("releases").toArray();

    if (releases.isEmpty()) {
        qDebug() << "No releases found";
        return;
    }

    auto first = releases[0].toObject();

    QString releaseId = first.value("id").toString();
    QString album = first.value("title").toString();
    QString date = first.value("date").toString();

    qDebug() << "Artist:" << artist;
    qDebug() << "Title:" << title;
    qDebug() << "Album:" << album;
    qDebug() << "Date:" << date;
    qDebug() << "Release MBID:" << releaseId;
    m_coverArtClient->getCover(releaseId);
}

void music_data::ResolverService::handleCoverResult(const QJsonObject &json)
{
    auto images = json.value("images").toArray();

    if (images.isEmpty()) {
        qDebug() << "No cover found";
        return;
    }

    auto first = images[0].toObject();

    QString coverUrl = first.value("image").toString();

    qDebug() << "Cover URL:" << coverUrl;
}
