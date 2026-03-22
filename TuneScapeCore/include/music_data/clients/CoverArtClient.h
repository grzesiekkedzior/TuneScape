#ifndef COVERARTCLIENT_H
#define COVERARTCLIENT_H
#include <QNetworkAccessManager>
#include <QObject>

namespace music_data {
class CoverArtClient : public QObject
{
    Q_OBJECT
public:
    explicit CoverArtClient(QObject *parent = nullptr);

    void getCover(const QString &releaseMbid);
signals:
    void coverFinished(const QJsonObject &json);

private:
    QNetworkAccessManager *m_man;
    QString coverUrl{};
};
} // namespace music_data

#endif // COVERARTCLIENT_H
