#ifndef JSONREQUESTQUEUE_H
#define JSONREQUESTQUEUE_H

#include <QCache>
#include <QElapsedTimer>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QPointer>
#include <QQueue>
#include <QTimer>
#include <functional>

namespace music_data {
// Bounded JSON transport. One instance serves all requests to MusicBrainz.
class JsonRequestQueue : public QObject
{
public:
    struct Result { QJsonObject data; QString error; int status = 0; };
    using Completion = std::function<void(quint64, const Result &)>;
    explicit JsonRequestQueue(int spacingMs, int timeoutMs, QObject *parent = nullptr);
    ~JsonRequestQueue() override;
    void request(const QUrl &url, QObject *receiver, quint64 token, Completion completion);
    void cancel();
    static QUrl withQuery(QUrl url, const QList<QPair<QString, QString>> &items);

private:
    struct Subscriber { QPointer<QObject> receiver; quint64 token; Completion completion; };
    struct Cached { Result result; qint64 expires; };
    void pump();
    QNetworkAccessManager m_network;
    QTimer m_timer;
    QElapsedTimer m_clock;
    QCache<QUrl, Cached> m_cache{2 * 1024 * 1024};
    QHash<QUrl, QList<Subscriber>> m_jobs;
    QQueue<QUrl> m_waiting;
    QPointer<QNetworkReply> m_reply;
    QByteArray m_bytes;
    quint64 m_epoch = 0;
    qint64 m_lastStart = -1000;
    qint64 m_blockedUntil = 0;
    int m_spacing;
    int m_timeout;
};
}
#endif
