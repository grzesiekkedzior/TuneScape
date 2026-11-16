#ifndef RADIOBROWSERAPI_H
#define RADIOBROWSERAPI_H

#include <QDeadlineTimer>
#include <QDnsLookup>
#include <QHash>
#include <QJsonArray>
#include <QNetworkAccessManager>
#include <QPointer>
#include <QTimer>
#include <QUrl>
#include <functional>

class QNetworkReply;

class RadioBrowserApi : public QObject
{
public:
    enum class Status { Success, EmptyResult, NetworkError, InvalidResponse, Timeout, Cancelled };
    struct Result {
        Status status;
        QJsonArray data;
        QString detail;
    };

    using RequestId = quint64;
    using Completion = std::function<void(RequestId, const Result &)>;

    explicit RadioBrowserApi(QObject *parent = nullptr);
    RadioBrowserApi(QList<QUrl> mirrors, int timeoutMs, QObject *parent = nullptr);
    ~RadioBrowserApi() override;

    RequestId request(const QUrl &endpoint, QObject *receiver, Completion completion);
    void cancel(RequestId id);

    static QUrl baseUrlForTarget(QString target, quint16 port = 443);
    static QUrl endpoint(const QString &path, const QString &key, const QString &value);

private:
    struct Operation {
        QUrl endpoint;
        QPointer<QObject> receiver;
        Completion completion;
        QMetaObject::Connection receiverDestroyed;
        QList<QUrl> mirrors;
        QPointer<QNetworkReply> reply;
        int nextMirror = 0;
        bool waitingForDns = false;
    };

    void resolve(RequestId id);
    void discoveryFinished();
    void finishDiscovery(const QString &error, Status failureStatus);
    void startAttempt(RequestId id);
    void attemptFinished(RequestId id, QNetworkReply *reply);
    void attemptFailed(RequestId id, Result result);
    void releaseReply(Operation &operation);
    void complete(RequestId id, Result result);

    QNetworkAccessManager m_network;
    QDnsLookup m_dns;
    QTimer m_dnsTimeout;
    QDeadlineTimer m_mirrorsExpiry;
    QList<QUrl> m_mirrors;
    QHash<RequestId, Operation> m_operations;
    RequestId m_nextId = 0;
    int m_timeoutMs = 5000;
    bool m_fixedMirrors = false;
    bool m_discovering = false;
};

#endif // RADIOBROWSERAPI_H
