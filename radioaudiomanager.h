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

signals:

private:
    QMediaPlayer *player;
    QAudioOutput *audioOutput;


};

#endif // RADIOAUDIOMANAGER_H
