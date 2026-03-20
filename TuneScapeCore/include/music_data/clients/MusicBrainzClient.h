#ifndef MUSICBRAINZCLIENT_H
#define MUSICBRAINZCLIENT_H

#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QObject>
namespace music_data {
class MusicBrainzClient : public QObject
{
    Q_OBJECT
public:
    //explicit MusicBrainzClient(QObject *parent = nullptr);
    void searchRecording(const QString &query);
    void getRecordingByID(const QString &mbid);

signals:
    void searchFinished(const QJsonObject &result);

private:
    QNetworkAccessManager *m_man;
};
} // namespace music_data

#endif // MUSICBRAINZCLIENT_H
