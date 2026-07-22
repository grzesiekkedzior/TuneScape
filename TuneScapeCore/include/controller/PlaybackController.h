#ifndef PLAYBACKCONTROLLER_H
#define PLAYBACKCONTROLLER_H
#include <QObject>
#include "PlaybackState.h"
#include "audioprocessor.h"
#include "container.h"
#include "radioaudiomanager.h"

class PlaybackController : public QObject
{
    Q_OBJECT
public:
    explicit PlaybackController(QObject *parent = nullptr);
    void play(const QUrl streamUrl);
    void pause();
    void stop();

private:
    PlaybackState &state = SingletonContainer::getSingleton().getInstance<PlaybackState>();

    RadioAudioManager &audioManager = SingletonContainer::getSingleton()
                                          .getInstance<RadioAudioManager>();

    AudioProcessor &audioProcessor = SingletonContainer::getSingleton().getInstance<AudioProcessor>();
};

#endif // PLAYBACKCONTROLLER_H
