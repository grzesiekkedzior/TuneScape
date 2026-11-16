#ifndef ICONLOADER_H
#define ICONLOADER_H

#include <QCache>
#include <QDeadlineTimer>
#include <QHash>
#include <QImage>
#include <QNetworkAccessManager>
#include <QPixmap>
#include <QPointer>
#include <QQueue>
#include <QThreadPool>
#include <QTimer>
#include <QUrl>
#include <functional>

class QNetworkReply;

class IconLoader : public QObject
{
public:
    using RequestId = quint64;
    using Completion = std::function<void(RequestId, const QPixmap &)>;
    enum class Priority { Visible, Background };

    explicit IconLoader(QObject *parent = nullptr, qsizetype cacheBytes = 16 * 1024 * 1024,
                        int timeoutMs = 5000);
    ~IconLoader() override;

    RequestId request(const QUrl &url, const QSize &size, qreal dpr,
                      QObject *receiver, Completion completion, Priority priority = Priority::Visible);
    void cancel(RequestId id);

private:
    struct Subscriber {
        QSize size;
        qreal dpr;
        QPointer<QObject> receiver;
        QMetaObject::Connection destroyed;
        Completion completion;
    };

    struct Pending {
        QHash<RequestId, Subscriber> subscribers;
        QPointer<QNetworkReply> reply;
        QByteArray bytes;
        bool active = false;
    };

    void pump();
    void start(const QUrl &url, Pending &job);
    bool read(const QUrl &url, QNetworkReply *reply);
    void downloaded(const QUrl &url, QNetworkReply *reply);
    void finish(const QUrl &url, const QImage &image, bool remember);
    void releaseReply(Pending &job);

    QNetworkAccessManager m_network;
    QThreadPool m_decoder;
    QTimer m_pump;
    QCache<QUrl, QImage> m_cache;
    QCache<QUrl, QDeadlineTimer> m_failures{128 * 1024};
    QHash<QUrl, Pending> m_pending;
    QHash<RequestId, QUrl> m_requests;
    QQueue<QUrl> m_waiting;
    QQueue<QUrl> m_background;
    RequestId m_nextId = 0;
    int m_active = 0;
    int m_timeoutMs;
};

#endif // ICONLOADER_H
