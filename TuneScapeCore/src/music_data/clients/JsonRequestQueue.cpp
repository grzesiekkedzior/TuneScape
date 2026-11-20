#include "include/music_data/clients/JsonRequestQueue.h"
#include <QDateTime>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrlQuery>
#include <climits>

using namespace music_data;

JsonRequestQueue::JsonRequestQueue(int spacingMs, int timeoutMs, QObject *parent)
    : QObject(parent), m_spacing(qMax(0, spacingMs)), m_timeout(qMax(1, timeoutMs))
{
    m_clock.start();
    m_timer.setSingleShot(true);
    connect(&m_timer, &QTimer::timeout, this, &JsonRequestQueue::pump);
}

JsonRequestQueue::~JsonRequestQueue() { cancel(); }

QUrl JsonRequestQueue::withQuery(QUrl url, const QList<QPair<QString, QString>> &items)
{
    QUrlQuery query;
    for (const auto &item : items)
        query.addQueryItem(item.first, item.second);
    // QUrlQuery deliberately leaves '+' literal; form-style API parsers do not.
    url.setQuery(query.query(QUrl::FullyEncoded).replace("+", "%2B"), QUrl::StrictMode);
    return url;
}

void JsonRequestQueue::request(const QUrl &url, QObject *receiver, quint64 token, Completion completion)
{
    if (!receiver)
        return;
    if (auto *cached = m_cache.object(url)) {
        if (cached->expires > m_clock.elapsed()) {
            const auto epoch = m_epoch;
            const auto result = cached->result;
            QTimer::singleShot(0, this, [this, epoch, target = QPointer<QObject>(receiver), token, completion, result] {
                if (epoch == m_epoch && target)
                    completion(token, result);
            });
            return;
        }
        m_cache.remove(url);
    }
    if (!url.isValid() || (url.scheme() != "http" && url.scheme() != "https") || m_jobs.size() >= 32) {
        const auto epoch = m_epoch;
        QTimer::singleShot(0, this, [this, epoch, target = QPointer<QObject>(receiver), token, completion] {
            if (epoch == m_epoch && target)
                completion(token, {{}, tr("Metadata request unavailable"), 0});
        });
        return;
    }
    if (!m_jobs.contains(url))
        m_waiting.enqueue(url);
    m_jobs[url].append({receiver, token, std::move(completion)});
    pump();
}

void JsonRequestQueue::cancel()
{
    ++m_epoch;
    m_timer.stop();
    m_jobs.clear();
    m_waiting.clear();
    auto *reply = m_reply.data();
    m_reply.clear();
    m_bytes.clear();
    if (reply) {
        disconnect(reply, nullptr, this, nullptr);
        reply->abort();
        reply->deleteLater();
    }
}

void JsonRequestQueue::pump()
{
    if (m_reply || m_waiting.isEmpty())
        return;
    const auto delay = qMax(m_lastStart + m_spacing, m_blockedUntil) - m_clock.elapsed();
    if (delay > 0) {
        m_timer.start(int(qMin(delay, qint64(INT_MAX))));
        return;
    }
    const auto url = m_waiting.dequeue();
    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", "TuneScape/6.0 (https://github.com/grzesiekkedzior/TuneScape; grzesiekkedzior@gmail.com)");
    request.setRawHeader("Accept", "application/json");
    m_lastStart = m_clock.elapsed();
    auto *reply = m_network.get(request);
    m_reply = reply;
    m_bytes.clear();
    const auto epoch = m_epoch;
    const auto read = [this, reply] {
        constexpr qint64 MaxBytes = 1024 * 1024;
        if (m_bytes.size() + reply->bytesAvailable() > MaxBytes) {
            reply->setProperty("metadataTooLarge", true);
            reply->abort();
        } else {
            m_bytes += reply->readAll();
        }
    };
    connect(reply, &QNetworkReply::readyRead, this, read);
    QTimer::singleShot(m_timeout, reply, [reply] {
        if (!reply->isFinished()) {
            reply->setProperty("metadataTimeout", true);
            reply->abort();
        }
    });
    connect(reply, &QNetworkReply::finished, this, [this, url, reply, epoch, read] {
        if (epoch != m_epoch || m_reply != reply)
            return;
        if (reply->isOpen() && !reply->property("metadataTooLarge").toBool())
            read();
        Result result;
        result.status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (reply->error() == QNetworkReply::NoError && result.status == 200) {
            QJsonParseError error;
            const auto doc = QJsonDocument::fromJson(m_bytes, &error);
            if (error.error == QJsonParseError::NoError && doc.isObject())
                result.data = doc.object();
            else
                result.error = tr("Invalid metadata JSON");
        } else {
            result.error = reply->property("metadataTimeout").toBool() ? tr("Metadata request timed out")
                         : reply->property("metadataTooLarge").toBool() ? tr("Metadata response too large")
                         : reply->errorString();
        }
        qint64 ttl = result.error.isEmpty() ? 600000 : result.status == 404 ? 300000 : 30000;
        if (result.status == 429 || result.status == 503) {
            const auto retry = reply->rawHeader("Retry-After");
            bool number = false;
            auto seconds = retry.toLongLong(&number);
            if (!number)
                seconds = QDateTime::currentDateTimeUtc().secsTo(QDateTime::fromString(QString::fromLatin1(retry), Qt::RFC2822Date));
            ttl = qMax(ttl, qBound(qint64(0), seconds, qint64(86400)) * 1000);
            m_blockedUntil = qMax(m_blockedUntil, m_clock.elapsed() + ttl);
        }
        const auto subscribers = m_jobs.take(url);
        m_cache.insert(url, new Cached{result, m_clock.elapsed() + ttl}, int(m_bytes.size() + url.toEncoded().size() + 512));
        m_reply.clear();
        m_bytes.clear();
        reply->deleteLater();
        for (const auto &subscriber : subscribers) {
            if (epoch != m_epoch)
                break;
            if (subscriber.receiver)
                subscriber.completion(subscriber.token, result);
        }
        pump();
    });
}
