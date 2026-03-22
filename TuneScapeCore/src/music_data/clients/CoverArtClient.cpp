#include "include/music_data/clients/CoverArtClient.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>

music_data::CoverArtClient::CoverArtClient(QObject *parent)
    : QObject(parent)
    , m_man(new QNetworkAccessManager(this))
{}

void music_data::CoverArtClient::getCover(const QString &releaseMbid)
{
    QNetworkRequest request;

    QUrl url(QString("https://coverartarchive.org/release/%1").arg(releaseMbid));

    request.setUrl(url);

    auto reply = m_man->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        emit coverFinished(doc.object());
        reply->deleteLater();
    });
}
