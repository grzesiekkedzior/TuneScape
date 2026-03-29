#ifndef MUSICBRAINZCLIENT_H
#define MUSICBRAINZCLIENT_H

#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QObject>
#include "include/music_data/models/QueryData.h"

namespace music_data {
class MusicBrainzClient : public QObject
{
    Q_OBJECT
public:
    explicit MusicBrainzClient(QObject *parent = nullptr);
    void searchRecording(const QString &rawquery);
    void getRecordingByID(const QString &mbid);

signals:
    void searchFinished(const QJsonObject &result);
    void lookupFinished(const QJsonObject &result);

private:
    QNetworkAccessManager *m_man;
    const QString USER_AGENT_HEADER
        = "TuneScape/5.0.0 ( https://github.com/grzesiekkedzior/TuneScape; "
          "grzesiekkedzior@gmail.com )";
    const QString RECORDING_API = "https://musicbrainz.org/ws/2/recording";
    const QString ARTIST_API = "https://musicbrainz.org/ws/2/artist";

    int m_lastRequestTime;

    QueryData extractData(const QString &rawData);
    QString cleanRawData(const QString &raw);
    bool isValidTrack(const QueryData &data);
};
} // namespace music_data

#endif // MUSICBRAINZCLIENT_H
