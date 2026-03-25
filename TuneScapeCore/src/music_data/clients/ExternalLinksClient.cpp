#include "include/music_data/clients/ExternalLinksClient.h"
#include <QJsonArray>
#include <QNetworkReply>

music_data::ExternalLinksClient::ExternalLinksClient(QObject *parent)
    : QObject(parent)
    , m_man(new QNetworkAccessManager(this))
{}

void music_data::ExternalLinksClient::getInfoByMbid(const QString &mbid)
{
    fetchRelations(mbid);
}

QMap<QString, QList<QUrl>> music_data::ExternalLinksClient::extractGroupedLinks(
    const QJsonObject &json)
{
    QMap<QString, QList<QUrl>> grouped;

    auto relations = json.value("relations").toArray();

    for (const auto &relVal : relations) {
        auto rel = relVal.toObject();

        QString type = rel.value("type").toString().toLower().trimmed();
        QString urlStr = rel["url"].toObject().value("resource").toString();

        if (urlStr.isEmpty())
            continue;

        QUrl url(urlStr);

        //QString group = normalizeType(type);

        grouped[type].append(url);
    }

    return grouped;
}

void music_data::ExternalLinksClient::fetchRelations(const QString &mbid)
{
    if (mbid.isEmpty()) {
        emit linksReady({});
        return;
    }

    QString url = QString("https://musicbrainz.org/ws/2/artist/%1?inc=url-rels&fmt=json").arg(mbid);

    QNetworkRequest req((QUrl(url)));
    req.setRawHeader("User-Agent", "TuneScape/5.0.0 grzesiekkedzior@gmail.com");

    auto reply = m_man->get(req);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            qDebug() << "ExternalLinks error:" << reply->errorString();
            emit linksReady({});
            return;
        }

        auto data = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);

        if (!doc.isObject()) {
            emit linksReady({});
            return;
        }

        auto links = extractGroupedLinks(doc.object());

        emit linksReady(links);
    });
}
