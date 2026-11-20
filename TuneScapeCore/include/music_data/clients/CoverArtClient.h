#ifndef COVERARTCLIENT_H
#define COVERARTCLIENT_H
#include "JsonRequestQueue.h"
namespace music_data {
class CoverArtClient : public QObject
{
    Q_OBJECT
public:
    explicit CoverArtClient(QObject *parent = nullptr, QUrl base = QUrl("https://coverartarchive.org/release/"), int timeoutMs = 8000);
    void getCover(const QString &mbid, quint64 token = 0);
    void cancel() { m_requests.cancel(); }
signals:
    void coverFinished(quint64 token, const QJsonObject &json);
private:
    QUrl m_base;
    JsonRequestQueue m_requests;
};
}
#endif
