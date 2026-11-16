#include "include/RadioImageManager.h"

#include <QNetworkReply>
#include <QNetworkRequest>

RadioImageManager::RadioImageManager(QObject *parent)
    : QObject(parent)
{}

RadioImageManager::~RadioImageManager()
{
    cancel();
}

void RadioImageManager::load(const QUrl &url)
{
    cancel();
    if (!url.isValid() || url.isEmpty())
        return;

    QNetworkRequest request(url);
    request.setTransferTimeout(5000);
    auto *reply = m_network.get(request);
    m_reply = reply;
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        if (m_reply != reply)
            return;
        m_reply.clear();
        reply->deleteLater();
        QPixmap pixmap;
        if (reply->error() == QNetworkReply::NoError
            && pixmap.loadFromData(reply->readAll())) {
            emit imageLoaded(pixmap.scaled(QSize(120, 120), Qt::KeepAspectRatio, Qt::FastTransformation));
        }
    });
}

void RadioImageManager::cancel()
{
    auto *reply = m_reply.data();
    m_reply.clear();
    if (reply) {
        disconnect(reply, nullptr, this, nullptr);
        reply->abort();
        reply->deleteLater();
    }
}
