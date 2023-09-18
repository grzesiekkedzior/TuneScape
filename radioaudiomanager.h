#ifndef RADIOAUDIOMANAGER_H
#define RADIOAUDIOMANAGER_H

#include <QObject>
#include <QMediaPlayer>
#include <QAudioOutput>

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

signals:

private:
    QMediaPlayer *player;
    QAudioOutput *audioOutput;


};

#endif // RADIOAUDIOMANAGER_H
