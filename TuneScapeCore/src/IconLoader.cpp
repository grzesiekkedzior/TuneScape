#include "include/IconLoader.h"

#include <QBuffer>
#include <QFutureWatcher>
#include <QImageReader>
#include <QNetworkReply>
#include <QtConcurrentRun>
#include <algorithm>
#include <utility>

namespace {

constexpr int MaxActive = 4;
constexpr qsizetype MaxDownloadBytes = 2 * 1024 * 1024;
constexpr int MaxSourceSide = 4096;
constexpr int MaxCachedSide = 480;
constexpr int FailureTtlMs = 30000;

QImage decodeIcon(QByteArray bytes)
{
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::ReadOnly);
    QImageReader reader(&buffer);
    reader.setDecideFormatFromContent(true);
    const QSize source = reader.size();
    if (source.isEmpty() || source.width() > MaxSourceSide || source.height() > MaxSourceSide)
        return {};

    const QSize limit(MaxCachedSide, MaxCachedSide);
    if (source.width() > limit.width() || source.height() > limit.height())
        reader.setScaledSize(source.scaled(limit, Qt::KeepAspectRatio).expandedTo(QSize(1, 1)));

    auto image = reader.read();
    if (image.width() > limit.width() || image.height() > limit.height())
        image = image.scaled(limit, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    image.setDevicePixelRatio(1);
    return image.convertToFormat(QImage::Format_ARGB32_Premultiplied);
}

QPixmap renderIcon(const QImage &image, const QSize &size, qreal dpr)
{
    if (image.isNull())
        return {};
    const QSize pixels(qBound(1, qRound(size.width() * dpr), MaxCachedSide),
                       qBound(1, qRound(size.height() * dpr), MaxCachedSide));
    auto pixmap = QPixmap::fromImage(image.scaled(pixels, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    pixmap.setDevicePixelRatio(dpr);
    return pixmap;
}

} // namespace

IconLoader::IconLoader(QObject *parent, qsizetype cacheBytes, int timeoutMs)
    : QObject(parent)
    , m_cache(cacheBytes)
    , m_timeoutMs(timeoutMs)
{
    m_decoder.setMaxThreadCount(1);
    m_pump.setSingleShot(true);
    connect(&m_pump, &QTimer::timeout, this, &IconLoader::pump);
}

IconLoader::~IconLoader()
{
    m_pump.stop();
    for (auto &job : m_pending) {
        for (const auto &sub : job.subscribers)
            disconnect(sub.destroyed);
        releaseReply(job);
    }
}

IconLoader::RequestId IconLoader::request(const QUrl &url, const QSize &size, qreal dpr,
                                         QObject *receiver, Completion completion, Priority priority)
{
    const auto id = ++m_nextId;
    const QUrl key = url.adjusted(QUrl::RemoveFragment);
    if (!m_pending.contains(key)) {
        auto &queue = priority == Priority::Visible ? m_waiting : m_background;
        queue.enqueue(key);
    } else if (priority == Priority::Visible && m_background.removeAll(key)) {
        m_waiting.enqueue(key);
    }
    auto destroyed = connect(receiver, &QObject::destroyed, this, [this, id] { cancel(id); });
    m_pending[key].subscribers.insert(id, {size, qBound(qreal(1), dpr, qreal(4)), receiver,
                                          destroyed, std::move(completion)});
    m_requests.insert(id, key);
    m_pump.start(0);
    return id;
}

void IconLoader::cancel(RequestId id)
{
    const auto request = m_requests.find(id);
    if (request == m_requests.end())
        return;

    const auto url = request.value();
    m_requests.erase(request);
    auto pending = m_pending.find(url);
    if (pending == m_pending.end() || !pending->subscribers.contains(id))
        return;
    auto &job = pending.value();
    disconnect(job.subscribers.take(id).destroyed);
    if (!job.subscribers.isEmpty() || (job.active && !job.reply))
        return;

    if (job.active)
        --m_active;
    releaseReply(job);
    m_pending.remove(url);
    m_waiting.removeAll(url);
    m_background.removeAll(url);
    m_pump.start(0);
}

void IconLoader::pump()
{
    const auto process = [this](QQueue<QUrl> &queue) {
        for (auto remaining = queue.size(); remaining > 0 && !queue.isEmpty(); --remaining) {
            const QUrl url = queue.dequeue();
            if (const auto *cached = m_cache.object(url)) {
                finish(url, *cached, false);
            } else if (const auto *expiry = m_failures.object(url); expiry && !expiry->hasExpired()) {
                finish(url, {}, false);
            } else if (!url.isValid() || url.host().isEmpty()
                       || (url.scheme() != "http" && url.scheme() != "https")) {
                finish(url, {}, false);
            } else if (m_active < MaxActive) {
                start(url, m_pending[url]);
            } else {
                queue.enqueue(url);
            }
        }
    };
    process(m_waiting);
    process(m_background);
}

void IconLoader::start(const QUrl &url, Pending &job)
{
    job.active = true;
    ++m_active;
    auto *reply = m_network.get(QNetworkRequest(url));
    job.reply = reply;
    reply->setReadBufferSize(MaxDownloadBytes + 1);
    connect(reply, &QNetworkReply::readyRead, this, [this, url, reply] { read(url, reply); });
    connect(reply, &QNetworkReply::finished, this, [this, url, reply] { downloaded(url, reply); });
    QTimer::singleShot(m_timeoutMs, reply, [this, url, reply] {
        const auto it = m_pending.constFind(url);
        if (it != m_pending.cend() && it->reply == reply)
            finish(url, {}, true);
    });
    if (reply->isFinished())
        downloaded(url, reply);
}

bool IconLoader::read(const QUrl &url, QNetworkReply *reply)
{
    auto it = m_pending.find(url);
    if (it == m_pending.end() || it->reply != reply)
        return false;
    if (reply->bytesAvailable() > MaxDownloadBytes - it->bytes.size()) {
        finish(url, {}, true);
        return false;
    }
    it->bytes.append(reply->readAll());
    return true;
}

void IconLoader::downloaded(const QUrl &url, QNetworkReply *reply)
{
    if (!read(url, reply))
        return;
    if (reply->error() != QNetworkReply::NoError) {
        finish(url, {}, true);
        return;
    }

    auto &job = m_pending[url];
    releaseReply(job);
    auto *watcher = new QFutureWatcher<QImage>(this);
    connect(watcher, &QFutureWatcher<QImage>::finished, this, [this, watcher, url] {
        const auto image = watcher->result();
        watcher->deleteLater();
        finish(url, image, true);
    });
    watcher->setFuture(QtConcurrent::run(&m_decoder, decodeIcon, std::move(job.bytes)));
}

void IconLoader::finish(const QUrl &url, const QImage &image, bool remember)
{
    auto job = m_pending.take(url);
    if (job.active)
        --m_active;
    releaseReply(job);
    if (remember) {
        const auto keyBytes = url.toEncoded().size();
        if (image.isNull())
            m_failures.insert(url, new QDeadlineTimer(FailureTtlMs), qMax(qsizetype(512), keyBytes));
        else
            m_cache.insert(url, new QImage(image), qMax(qsizetype(1024), image.sizeInBytes() + keyBytes));
    }
    for (auto it = job.subscribers.cbegin(); it != job.subscribers.cend(); ++it) {
        disconnect(it->destroyed);
    }
    m_pump.start(0);
    // Keep the source alive if a completion triggers another request and evicts this cache entry.
    const QImage source = image;
    struct RenderedIcon {
        QSize size;
        qreal dpr;
        QPixmap pixmap;
    };
    QList<RenderedIcon> rendered;
    for (auto it = job.subscribers.cbegin(); it != job.subscribers.cend(); ++it) {
        if (!m_requests.remove(it.key()) || !it->receiver)
            continue;
        const auto sameSize = [&it](const auto &item) {
            return item.size == it->size && item.dpr == it->dpr;
        };
        const auto cached = std::find_if(rendered.cbegin(), rendered.cend(), sameSize);
        if (cached != rendered.cend()) {
            it->completion(it.key(), cached->pixmap);
        } else {
            const auto pixmap = renderIcon(source, it->size, it->dpr);
            rendered.append({it->size, it->dpr, pixmap});
            it->completion(it.key(), pixmap);
        }
    }
}

void IconLoader::releaseReply(Pending &job)
{
    auto *reply = job.reply.data();
    job.reply.clear();
    if (!reply)
        return;
    disconnect(reply, nullptr, this, nullptr);
    if (!reply->isFinished())
        reply->abort();
    reply->deleteLater();
}
