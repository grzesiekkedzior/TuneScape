#include "include/RadioImageManager.h"
#include <QEventLoop>
#include <QNetworkAccessManager>
#include <QNetworkReply>
namespace {
constexpr auto RadioIconPath = ":/images/img/radio96x96.png";
}
RadioImageManager::RadioImageManager(QObject *parent)
    : QObject(parent)
{}

QPixmap RadioImageManager::downloadImageSync(const QUrl &url)
{
    QNetworkAccessManager manager;
    QNetworkRequest request(url);
    QNetworkReply *reply = manager.get(request);

    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    QPixmap pixmap;

    if (reply->error() == QNetworkReply::NoError) {
        QString contentType = reply->header(QNetworkRequest::ContentTypeHeader).toString();
        if (contentType.startsWith("image/")) {
            QByteArray imageData = reply->readAll();
            pixmap.loadFromData(imageData);
        }
    } else {
        qDebug() << "Image download error:" << reply->errorString();
    }

    reply->deleteLater();
    return pixmap;
}

QPixmap RadioImageManager::prepareImage(const QPixmap &pixmap) const
{
    if (pixmap.isNull())
        return QPixmap(RadioIconPath);

    const QSize imageSize(120, 120);

    return pixmap.scaled(imageSize, Qt::KeepAspectRatio, Qt::FastTransformation);
}
