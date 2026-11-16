#include "include/RadioBrowserApi.h"

#include <QJsonDocument>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <algorithm>
#include <utility>

namespace {
constexpr int MaxMirrorAttempts = 3;
constexpr int DiscoveryTimeoutMs = 4000;
}

RadioBrowserApi::RadioBrowserApi(QObject *parent)
    : QObject(parent)
    , m_dns(QDnsLookup::SRV, QStringLiteral("_api._tcp.radio-browser.info"))
{
    m_dnsTimeout.setSingleShot(true);
    connect(&m_dns, &QDnsLookup::finished, this, &RadioBrowserApi::discoveryFinished);
    connect(&m_dnsTimeout, &QTimer::timeout, this, [this] {
        finishDiscovery(tr("Radio Browser server discovery timed out."), Status::Timeout);
    });
}

RadioBrowserApi::RadioBrowserApi(QList<QUrl> mirrors, int timeoutMs, QObject *parent)
    : RadioBrowserApi(parent)
{
    m_mirrors = std::move(mirrors);
    m_fixedMirrors = true;
    m_timeoutMs = qMax(1, timeoutMs);
}

RadioBrowserApi::~RadioBrowserApi()
{
    for (auto &operation : m_operations) {
        disconnect(operation.receiverDestroyed);
        releaseReply(operation);
    }
}

QUrl RadioBrowserApi::baseUrlForTarget(QString target, quint16 port)
{
    if (target.endsWith('.'))
        target.chop(1);
    QUrl url;
    url.setScheme(QStringLiteral("https"));
    url.setHost(target);
    if (port != 443)
        url.setPort(port);
    url.setPath(QStringLiteral("/"));
    return url;
}

QUrl RadioBrowserApi::endpoint(const QString &path, const QString &key, const QString &value)
{
    QUrl url(path);
    url.setQuery(key + '=' + QString::fromLatin1(QUrl::toPercentEncoding(value)), QUrl::StrictMode);
    return url;
}

RadioBrowserApi::RequestId RadioBrowserApi::request(const QUrl &endpoint, QObject *receiver,
                                                    Completion completion)
{
    Q_ASSERT(receiver);
    const RequestId id = ++m_nextId;
    auto &operation = m_operations[id];
    operation.endpoint = endpoint;
    operation.receiver = receiver;
    operation.completion = std::move(completion);
    operation.receiverDestroyed = connect(receiver, &QObject::destroyed, this, [this, id] {
        cancel(id);
    });
    QTimer::singleShot(0, this, [this, id] { resolve(id); });
    return id;
}

void RadioBrowserApi::cancel(RequestId id)
{
    complete(id, {Status::Cancelled, {}, {}});
}

void RadioBrowserApi::resolve(RequestId id)
{
    auto it = m_operations.find(id);
    if (it == m_operations.end())
        return;
    if (m_fixedMirrors || (!m_mirrors.isEmpty() && !m_mirrorsExpiry.hasExpired())) {
        it->mirrors = m_mirrors;
        startAttempt(id);
        return;
    }

    it->waitingForDns = true;
    if (!m_discovering) {
        m_discovering = true;
        m_dnsTimeout.start(DiscoveryTimeoutMs);
        m_dns.lookup();
    }
}

void RadioBrowserApi::discoveryFinished()
{
    if (!m_discovering)
        return;
    if (m_dns.error() != QDnsLookup::NoError) {
        finishDiscovery(m_dns.errorString(), Status::NetworkError);
        return;
    }

    auto records = m_dns.serviceRecords();
    std::stable_sort(records.begin(), records.end(), [](const auto &a, const auto &b) {
        if (a.priority() != b.priority())
            return a.priority() < b.priority();
        return a.weight() > b.weight();
    });

    m_mirrors.clear();
    quint32 ttl = 3600;
    for (const auto &record : records) {
        const QUrl url = baseUrlForTarget(record.target(), record.port());
        if (!url.host().isEmpty() && url.isValid() && !m_mirrors.contains(url)) {
            m_mirrors.append(url);
            ttl = qMin(ttl, record.timeToLive());
        }
    }
    m_mirrorsExpiry.setRemainingTime(qint64(ttl) * 1000);
    finishDiscovery(m_mirrors.isEmpty() ? tr("No Radio Browser servers were found.") : QString(),
                    Status::NetworkError);
}

void RadioBrowserApi::finishDiscovery(const QString &error, Status failureStatus)
{
    m_discovering = false;
    m_dnsTimeout.stop();
    if (!m_dns.isFinished())
        m_dns.abort();

    const auto ids = m_operations.keys();
    for (RequestId id : ids) {
        auto it = m_operations.find(id);
        if (it == m_operations.end() || !it->waitingForDns)
            continue;
        it->waitingForDns = false;
        if (error.isEmpty()) {
            it->mirrors = m_mirrors;
            startAttempt(id);
        } else {
            complete(id, {failureStatus, {}, error});
        }
    }
}

void RadioBrowserApi::startAttempt(RequestId id)
{
    auto it = m_operations.find(id);
    if (it == m_operations.end())
        return;
    if (it->nextMirror >= it->mirrors.size()) {
        complete(id, {Status::NetworkError, {}, tr("No Radio Browser servers are available.")});
        return;
    }

    const QUrl url = it->mirrors.at(it->nextMirror++).resolved(it->endpoint);
    QNetworkRequest request(url);
    request.setRawHeader("Accept", "application/json");
    request.setRawHeader("User-Agent", "TuneScape/6.0");
    auto *reply = m_network.get(request);
    it->reply = reply;

    QTimer::singleShot(m_timeoutMs, reply, [this, id, reply] {
        const auto current = m_operations.constFind(id);
        if (current != m_operations.cend() && current->reply == reply)
            attemptFailed(id, {Status::Timeout, {}, tr("The Radio Browser request timed out.")});
    });
    connect(reply, &QNetworkReply::finished, this, [this, id, reply] {
        attemptFinished(id, reply);
    });
    if (reply->isFinished())
        attemptFinished(id, reply);
}

void RadioBrowserApi::attemptFinished(RequestId id, QNetworkReply *reply)
{
    const auto it = m_operations.constFind(id);
    if (it == m_operations.cend() || it->reply != reply)
        return;

    const int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (reply->error() != QNetworkReply::NoError || httpStatus < 200 || httpStatus >= 300) {
        const QString detail = httpStatus >= 300
            ? tr("HTTP %1").arg(httpStatus) : reply->errorString();
        const auto status = reply->error() == QNetworkReply::TimeoutError
            ? Status::Timeout : Status::NetworkError;
        attemptFailed(id, {status, {}, detail});
        return;
    }

    QJsonParseError error;
    const auto document = QJsonDocument::fromJson(reply->readAll(), &error);
    const auto data = document.array();
    if (!document.isArray() || std::any_of(data.begin(), data.end(),
                                          [](const QJsonValue &value) { return !value.isObject(); })) {
        const QString detail = error.error == QJsonParseError::NoError
            ? tr("Expected a JSON array of objects.") : error.errorString();
        attemptFailed(id, {Status::InvalidResponse, {}, detail});
        return;
    }

    complete(id, {data.isEmpty() ? Status::EmptyResult : Status::Success, data, {}});
}

void RadioBrowserApi::attemptFailed(RequestId id, Result result)
{
    auto it = m_operations.find(id);
    if (it == m_operations.end())
        return;
    releaseReply(*it);
    if (it->nextMirror >= qMin<qsizetype>(it->mirrors.size(), MaxMirrorAttempts)) {
        complete(id, std::move(result));
    } else {
        QTimer::singleShot(0, this, [this, id] { startAttempt(id); });
    }
}

void RadioBrowserApi::releaseReply(Operation &operation)
{
    auto *reply = operation.reply.data();
    operation.reply.clear();
    if (!reply)
        return;

    disconnect(reply, nullptr, this, nullptr);
    if (!reply->isFinished())
        reply->abort();
    reply->deleteLater();
}

void RadioBrowserApi::complete(RequestId id, Result result)
{
    auto it = m_operations.find(id);
    if (it == m_operations.end())
        return;
    auto operation = std::move(*it);
    m_operations.erase(it);
    disconnect(operation.receiverDestroyed);
    releaseReply(operation);
    if (operation.waitingForDns && m_discovering
        && std::none_of(m_operations.cbegin(), m_operations.cend(),
                        [](const Operation &op) { return op.waitingForDns; })) {
        m_discovering = false;
        m_dnsTimeout.stop();
        m_dns.abort();
    }
    if (operation.receiver)
        operation.completion(id, result);
}
