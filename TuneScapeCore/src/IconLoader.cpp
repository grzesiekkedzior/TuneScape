#include "include/IconLoader.h"

#include <QNetworkReply>
#include <QPixmap>

IconLoader::IconLoader(QObject *parent)
    : QObject(parent)
{}

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

    QNetworkRequest request(url);

    QNetworkReply *reply = m_networkManager.get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply, row, size, cacheKey]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            emit iconLoadFailed(row);
            return;
        }

        QPixmap pixmap;

        if (!pixmap.loadFromData(reply->readAll())) {
            emit iconLoadFailed(row);
            return;
        }

        pixmap = pixmap.scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation);

        const QIcon icon(pixmap);

        m_cache.insert(cacheKey, icon);

        emit iconLoaded(row, icon);
    });
}