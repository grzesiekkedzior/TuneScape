#ifndef MUSICBRAINZCLIENT_H
#define MUSICBRAINZCLIENT_H
#include "JsonRequestQueue.h"
#include "include/music_data/models/QueryData.h"
namespace music_data {
class MusicBrainzClient : public QObject
{
    Q_OBJECT
public:
    explicit MusicBrainzClient(QObject *parent = nullptr, QUrl base = QUrl("https://musicbrainz.org/ws/2/"), int timeoutMs = 8000);
    void searchRecording(const QString &query, quint64 token = 0);
    void getRecordingByID(const QString &mbid, quint64 token = 0);
    void cancel() { m_requests.cancel(); }
    JsonRequestQueue &requests() { return m_requests; }
    QUrl baseUrl() const { return m_base; }
    static QueryData extractData(const QString &raw);
    static bool isValidTrack(const QueryData &data);
signals:
    void searchFinished(quint64 token, const QJsonObject &result);
    void lookupFinished(quint64 token, const QJsonObject &result);
    void requestFailed(quint64 token, const QString &message);
private:
    QUrl m_base;
    JsonRequestQueue m_requests;
};
}
#endif
