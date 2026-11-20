#include "include/music_data/clients/ExternalLinksClient.h"
#include <QJsonArray>
using namespace music_data;
ExternalLinksClient::ExternalLinksClient(QObject *parent)
    : QObject(parent), m_requests(new JsonRequestQueue(1000, 8000, this)) {}
void ExternalLinksClient::useQueue(JsonRequestQueue *queue, const QUrl &base)
{
    if (m_requests && m_requests->parent() == this) delete m_requests.data();
    m_requests = queue;
    m_base = base;
}
QMap<QString, QList<QUrl>> ExternalLinksClient::extractGroupedLinks(const QJsonObject &json)
{
    QMap<QString, QList<QUrl>> links;
    for (const auto &value : json["relations"].toArray()) {
        const auto relation = value.toObject();
        const QUrl url(relation["url"].toObject()["resource"].toString());
        if (url.isValid() && (url.scheme() == "http" || url.scheme() == "https"))
            links[relation["type"].toString().trimmed()].append(url);
    }
    return links;
}
void ExternalLinksClient::getInfoByMbid(const QString &mbid, quint64 token)
{
    if (!m_requests) return;
    const auto url = JsonRequestQueue::withQuery(m_base.resolved(QUrl("artist/" + QString::fromLatin1(QUrl::toPercentEncoding(mbid)))), {{"inc", "url-rels"}, {"fmt", "json"}});
    m_requests->request(url, this, token, [this](auto id, const auto &result) { emit linksReady(id, extractGroupedLinks(result.data)); });
}
