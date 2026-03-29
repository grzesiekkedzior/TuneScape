#include "include/music_data/clients/MusicBrainzClient.h"
#include <QNetworkReply>
#include <QUrlQuery>

music_data::MusicBrainzClient::MusicBrainzClient(QObject *parent)
    : QObject(parent)
    , m_man(new QNetworkAccessManager(this))
    , m_lastRequestTime(0)
{
}

void music_data::MusicBrainzClient::searchRecording(const QString &rawquery)
{
    QueryData data = extractData(rawquery);

    if (!isValidTrack(data)) {
        qDebug() << "Skipping non-music content:" << rawquery;
        return;
    }

    qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (now - m_lastRequestTime < 1000) {
        qDebug() << "Rate limited, skipping request";
        return;
    }
    m_lastRequestTime = now;

    QUrl url(RECORDING_API);
    QUrlQuery query;

    QString queryStr;
    if (!data.artist.isEmpty()) {
        queryStr = QString("recording:%1 AND artist:%2").arg(data.title, data.artist);
    } else {
        queryStr = QString("recording:%1").arg(data.title);
    }

    query.addQueryItem("query", queryStr);
    query.addQueryItem("fmt", "json");
    query.addQueryItem("limit", "5");
    url.setQuery(query);

    QNetworkRequest request;
    request.setUrl(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, USER_AGENT_HEADER);

    auto reply = m_man->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() != QNetworkReply::NoError) {
            qDebug() << "Network error:" << reply->errorString();
            reply->deleteLater();
            return;
        }

        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (status != 200) {
            qDebug() << "HTTP error:" << status;
            reply->deleteLater();
            return;
        }

        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll(), &err);

        if (err.error != QJsonParseError::NoError) {
            qDebug() << "JSON parse error:" << err.errorString();
            reply->deleteLater();
            return;
        }

        emit searchFinished(doc.object());
        reply->deleteLater();
    });
}

void music_data::MusicBrainzClient::getRecordingByID(const QString &mbid)
{
    QNetworkRequest request;

    QUrl url(QString("https://musicbrainz.org/ws/2/recording/%1").arg(mbid));

    QUrlQuery query;
    query.addQueryItem("inc", "artists+releases");
    query.addQueryItem("fmt", "json");

    url.setQuery(query);
    request.setUrl(url);

    request.setHeader(QNetworkRequest::UserAgentHeader, USER_AGENT_HEADER.toUtf8());

    auto reply = m_man->get(request);

    connect(reply, &QNetworkReply::finished, this, [reply, this]() {
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        //qDebug() << "LOOKUP JSON:" << doc.object();
        emit lookupFinished(doc.object());
        reply->deleteLater();
    });
}

QueryData music_data::MusicBrainzClient::extractData(const QString &rawData)
{
    QueryData result;

    if (rawData.trimmed().isEmpty())
        return result;

    QString data = cleanRawData(rawData);

    if (data.isEmpty())
        return result;

    data.remove(QRegularExpression("<[^>]*>"));

    if (result.artist.contains(";") || result.artist.contains(",")) {
        result.artist = result.artist.split(QRegularExpression("[;,]")).first().trimmed();
    }

    struct Separator
    {
        QString sep;
        bool reversed;
    };

    const QList<Separator> separators = {{" - ", false},
                                         {" – ", false},
                                         {" — ", false},
                                         {" / ", false},
                                         {": ", false},
                                         {" by ", true}};

    for (const auto &s : separators) {
        if (data.contains(s.sep)) {
            int idx = data.indexOf(s.sep);
            QString left = data.left(idx).trimmed();
            QString right = data.mid(idx + s.sep.length()).trimmed();

            result.artist = s.reversed ? right : left;
            result.title = s.reversed ? left : right;

            if (result.artist.contains(";"))
                result.artist = result.artist.split(";").first().trimmed();

            static QRegularExpression featRe(R"(\s+(feat\.?|ft\.?|featuring)\s+.+$)",
                                             QRegularExpression::CaseInsensitiveOption);
            result.artist.remove(featRe);

            if (!result.artist.isEmpty()
                && result.title.startsWith(result.artist, Qt::CaseInsensitive)) {
                result.title = result.title.mid(result.artist.length()).trimmed();
                if (result.title.startsWith("-") || result.title.startsWith("/"))
                    result.title = result.title.mid(1).trimmed();
            }

            static QRegularExpression suffixRe(R"(\s*[\(\[](radio edit|extended|remix|remaster|)"
                                               R"(remastered|live|acoustic|instrumental|edit|)"
                                               R"(original mix|club mix)[\s\S]*?[\)\]])",
                                               QRegularExpression::CaseInsensitiveOption);
            result.title.remove(suffixRe);

            result.artist = result.artist.trimmed();
            result.title = result.title.trimmed();

            return result;
        }
    }

    result.title = data;
    result.artist = "";
    return result;
}

QString music_data::MusicBrainzClient::cleanRawData(const QString &raw)
{
    QString data = raw.trimmed();
    if (data.isEmpty())
        return data;

    static QRegularExpression urlRe(R"(\s*https?://\S+)");
    data.remove(urlRe);

    static QRegularExpression andRe(R"(\s+AND\s+\w+:[^&]*)",
                                    QRegularExpression::CaseInsensitiveOption);
    data.remove(andRe);

    static QRegularExpression domainRe(R"(\b\S+\.(fm|com|pl|net|org|radio|de|uk)\S*\b)");
    data.remove(domainRe);

    static QRegularExpression bitrateRe(R"(\s*\(\d+\s*kbit[_\s]\w+\)|\s*\(\d+\s*kbps\w*\))",
                                        QRegularExpression::CaseInsensitiveOption);
    data.remove(bitrateRe);

    static QRegularExpression incompleteFeatRe(R"(\s+(ft\.?|feat\.?)\s+\w+$)",
                                               QRegularExpression::CaseInsensitiveOption);
    data.remove(incompleteFeatRe);

    data = data.simplified();

    return data.trimmed();
}

bool music_data::MusicBrainzClient::isValidTrack(const QueryData &data)
{
    if (data.title.length() < 2)
        return false;

    if (data.title.startsWith("..."))
        return false;

    if (data.title.startsWith("-") || data.title.startsWith("/"))
        return false;

    static QRegularExpression onlyDotsRe(R"(^[\s\.]+$)");
    if (onlyDotsRe.match(data.title).hasMatch())
        return false;

    static QRegularExpression domainArtistRe(R"(\S+\.(pl|com|fm|net|org|de|uk)$)",
                                             QRegularExpression::CaseInsensitiveOption);
    if (domainArtistRe.match(data.artist).hasMatch())
        return false;

    static QRegularExpression adRe(R"(verbraucherinformation|werbung|commercial|)"
                                   R"(jingle|station\s*id|now\s*playing|gleich\s*geht|)"
                                   R"(halbzeit|geht.s\s*los|auf\s*laut)",
                                   QRegularExpression::CaseInsensitiveOption);
    if (adRe.match(data.title).hasMatch())
        return false;

    return true;
}