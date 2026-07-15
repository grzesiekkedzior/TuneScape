#include "include/RadioImageManager.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>

RadioImageManager::RadioImageManager(Ui::MainWindow *ui, miniplayer *mp, QObject *parent)
    : QObject(parent)
    , m_ui(ui)
    , m_miniPlayer(mp)
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

void RadioImageManager::setImageToUI(const QPixmap &pixmap)
{
    QPixmap scaled = pixmap;
    if (!scaled.isNull()) {
        QSize imageSize(120, 120);
        scaled = scaled.scaled(imageSize, Qt::KeepAspectRatio, Qt::FastTransformation);
    } else {
        scaled = QPixmap(RADIO_ICON);
    }

    m_ui->infoLabel->setPixmap(scaled);
    m_miniPlayer->getMui()->radioImage->setPixmap(scaled);
    m_ui->infoLabel->show();
}
