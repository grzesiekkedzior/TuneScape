#ifndef RADIOAUDIOMANAGER_H
#define RADIOAUDIOMANAGER_H
#include "include/streamreader.h"

#include <QAudioOutput>
#include <QMediaPlayer>
#include <QObject>

class RadioAudioManager : public QObject
{
    Q_OBJECT
public:
    RadioAudioManager();
    ~RadioAudioManager();
    void loadStream(const QUrl &url);
    void playStream();
    void stopStream();
    QMediaPlayer *getMediaPlayer() const;
    QAudioOutput *getAudioAutput() const;
    void setVolume(int value);

signals:

private:
    QMediaPlayer *player;
    QAudioOutput *audioOutput;
    StreamReader streamReader;
};

#endif // RADIOAUDIOMANAGER_H
