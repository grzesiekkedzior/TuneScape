#ifndef EXTERNALLINKSCLIENT_H
#define EXTERNALLINKSCLIENT_H
#include <QJsonObject>
#include <QMap>
#include <QNetworkAccessManager>
#include <QObject>

namespace music_data {
class ExternalLinksClient : public QObject
{
    Q_OBJECT
public:
    explicit ExternalLinksClient(QObject *parent = nullptr);
    void getInfoByMbid(const QString &mbid);
    QMap<QString, QList<QUrl>> extractGroupedLinks(const QJsonObject &json);
    void fetchRelations(const QString &mbid);
signals:
    void linksReady(const QMap<QString, QList<QUrl>> &links);

private:
    QNetworkAccessManager *m_man;
};
} // namespace music_data

#endif // EXTERNALLINKSCLIENT_H
