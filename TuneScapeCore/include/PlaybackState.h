#ifndef PLAYBACKSTATE_H
#define PLAYBACKSTATE_H

#include <QObject>

class PlaybackState
{
public:
    bool isPlaying = false;
    bool isPause = false;
    bool isStopClicked = true;

    QString currentStreamUrl;
};

#endif // PLAYBACKSTATE_H
