#include "include/music_data/clients/MusicBrainzClient.h"
#include <QRegularExpression>
using namespace music_data;
MusicBrainzClient::MusicBrainzClient(QObject *parent, QUrl base, int timeoutMs)
    : QObject(parent), m_base(base), m_requests(1000, timeoutMs, this) {}

QueryData MusicBrainzClient::extractData(const QString &raw)
{
    QueryData result;
    auto text = raw.normalized(QString::NormalizationForm_C).simplified();
    if (text.size() > 512 || text.contains(QRegularExpression("[\\x00-\\x08\\x0b-\\x1f]")))
        return result;
    text.remove(QRegularExpression("<[^>]*>"));
    text = text.trimmed();
    const QStringList separators = {QStringLiteral(" - "), QString::fromUtf8(" \xe2\x80\x93 "), QString::fromUtf8(" \xe2\x80\x94 ")};
    for (const auto &separator : separators) {
        const auto at = text.indexOf(separator);
        if (at >= 0) {
            result.artist = text.left(at).trimmed();
            result.title = text.mid(at + separator.size()).trimmed();
            return result;
        }
    }
    result.title = text;
    return result;
}
bool MusicBrainzClient::isValidTrack(const QueryData &data)
{
    if (data.title.size() < 2 || data.title.startsWith("http", Qt::CaseInsensitive))
        return false;
    static const QRegularExpression noise("^(werbung|reklama|advertisement|commercial|jingle|station id)$|https?://|www\\.|^[^\\s]+\\.(fm|com|pl|net|org|de|uk)$", QRegularExpression::CaseInsensitiveOption);
    return !noise.match(data.title).hasMatch() && data.title.contains(QRegularExpression("[\\p{L}\\p{N}]"));
}
void MusicBrainzClient::searchRecording(const QString &query, quint64 token)
{
    const auto data = extractData(query);
    if (!isValidTrack(data)) {
        QTimer::singleShot(0, this, [this, token] { emit searchFinished(token, {}); });
        return;
    }
    const auto literal = [](QString value) {
        value.replace("\\", "\\\\");
        value.replace("\"", "\\\"");
        return '"' + value + '"';
    };
    QString search = "recording:" + literal(data.title);
    if (!data.artist.isEmpty())
        search += " AND artist:" + literal(data.artist);
    const auto url = JsonRequestQueue::withQuery(m_base.resolved(QUrl("recording")), {{"query", search}, {"fmt", "json"}, {"limit", "5"}});
    m_requests.request(url, this, token, [this](auto id, const auto &result) {
        if (!result.error.isEmpty()) emit requestFailed(id, result.error);
        emit searchFinished(id, result.data);
    });
}
void MusicBrainzClient::getRecordingByID(const QString &mbid, quint64 token)
{
    const auto url = JsonRequestQueue::withQuery(m_base.resolved(QUrl("recording/" + QString::fromLatin1(QUrl::toPercentEncoding(mbid)))), {{"inc", "artists+releases"}, {"fmt", "json"}});
    m_requests.request(url, this, token, [this](auto id, const auto &result) {
        if (!result.error.isEmpty()) emit requestFailed(id, result.error);
        emit lookupFinished(id, result.data);
    });
}
