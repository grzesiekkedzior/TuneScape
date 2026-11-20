#ifndef EXTERNALLINKSCLIENT_H
#define EXTERNALLINKSCLIENT_H
#include "JsonRequestQueue.h"
namespace music_data {
class ExternalLinksClient : public QObject
{
    Q_OBJECT
public:
    explicit ExternalLinksClient(QObject *parent = nullptr);
    void useQueue(JsonRequestQueue *queue, const QUrl &base);
    void getInfoByMbid(const QString &mbid, quint64 token = 0);
    void fetchRelations(const QString &mbid) { getInfoByMbid(mbid); }
    QMap<QString, QList<QUrl>> extractGroupedLinks(const QJsonObject &json);
signals:
    void linksReady(quint64 token, const QMap<QString, QList<QUrl>> &links);
private:
    QPointer<JsonRequestQueue> m_requests;
    QUrl m_base = QUrl("https://musicbrainz.org/ws/2/");
};
}
#endif
