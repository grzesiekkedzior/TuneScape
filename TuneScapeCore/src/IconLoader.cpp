#include "include/IconLoader.h"

#include <QNetworkReply>
#include <QPixmap>
#include <utility>

IconLoader::IconLoader(QObject *parent)
    : QObject(parent)
{}

IconLoader::~IconLoader()
{
    cancelPending();
}

void IconLoader::cancelPending()
{
    ++m_generation;
    const auto pending = std::exchange(m_pending, {});
    for (const auto &entry : pending) {
        if (auto *reply = entry.reply.data()) {
            disconnect(reply, nullptr, this, nullptr);
            reply->abort();
            reply->deleteLater();
        }
    }
}

void IconLoader::loadIcon(int row, const QUrl &url, const QSize &size)
{
    if (!url.isValid() || url.isEmpty()) {
        emit iconLoadFailed(row);
        return;
    }

    const QString cacheKey = url.toString()
                             + QStringLiteral("_%1x%2").arg(size.width()).arg(size.height());
    const auto cacheIt = m_cache.constFind(cacheKey);
    if (cacheIt != m_cache.constEnd()) {
        emit iconLoaded(row, cacheIt.value());
        return;
    }

    auto pending = m_pending.find(cacheKey);
    if (pending != m_pending.end()) {
        pending->rows.append(row);
        return;
    }

    QNetworkRequest request(url);
    request.setTransferTimeout(5000);
    auto *reply = m_networkManager.get(request);
    m_pending.insert(cacheKey, {reply, {row}});
    const auto generation = m_generation;
    connect(reply, &QNetworkReply::finished, this, [this, reply, size, cacheKey, generation] {
        if (generation != m_generation)
            return;
        const auto entry = m_pending.take(cacheKey);
        reply->deleteLater();

        QPixmap pixmap;
        QIcon icon;
        if (reply->error() == QNetworkReply::NoError && pixmap.loadFromData(reply->readAll())) {
            icon = QIcon(pixmap.scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            m_cache.insert(cacheKey, icon);
        }

        for (int row : entry.rows) {
            if (generation != m_generation)
                break;
            if (icon.isNull())
                emit iconLoadFailed(row);
            else
                emit iconLoaded(row, icon);
        }
    });
}
