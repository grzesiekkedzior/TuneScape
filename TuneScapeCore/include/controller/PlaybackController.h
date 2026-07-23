#ifndef PLAYBACKCONTROLLER_H
#define PLAYBACKCONTROLLER_H
#include <QObject>
#include "include/PlaybackState.h"
#include "include/container.h"
#include "include/radioaudiomanager.h"

class PlaybackController : public QObject
{
    Q_OBJECT
public:
    explicit PlaybackController(QObject *parent = nullptr);
    void play(const QUrl &streamUrl);
    void pause();
    void stop();
    void resume();

    bool isPlaying() const;
    bool isPaused() const;

    void setPlaying(bool playing);
    void setPaused(bool paused);

    void setVolume(int volume);
    bool isAvailable() const;
    QMediaPlayer *mediaPlayer() const;

private:
    PlaybackState &state = SingletonContainer::getSingleton().getInstance<PlaybackState>();

    RadioAudioManager &audioManager = SingletonContainer::getSingleton()
                                          .getInstance<RadioAudioManager>();
};

#endif // PLAYBACKCONTROLLER_H
