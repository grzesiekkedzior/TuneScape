#include "include/controller/PlaybackController.h"

PlaybackController::PlaybackController(QObject *parent)
    : QObject{parent}
{}

void PlaybackController::play(const QUrl &streamUrl)
{
    audioManager.play(streamUrl);

    state.currentStreamUrl = streamUrl.toString();
    state.isPlaying = true;
    state.isPause = false;
}

void PlaybackController::pause()
{
    audioManager.stopStream();
    state.isPlaying = false;
    state.isPause = true;
}

void PlaybackController::stop()
{
    audioManager.stopStream();

    state.isPlaying = false;
    state.isPause = false;
    state.currentStreamUrl.clear();
}

void PlaybackController::resume()
{
    if (state.currentStreamUrl.isEmpty())
        return;

    audioManager.playStream();

    state.isPlaying = true;
    state.isPause = false;
}

bool PlaybackController::isPlaying() const
{
    return state.isPlaying;
}

bool PlaybackController::isPaused() const
{
    return state.isPause;
}

void PlaybackController::setPlaying(bool playing)
{
    state.isPlaying = playing;
}

void PlaybackController::setPaused(bool paused)
{
    state.isPause = paused;
}

void PlaybackController::setVolume(int volume)
{
    audioManager.setVolume(volume);
}

bool PlaybackController::isAvailable() const
{
    return audioManager.getMediaPlayer()->isAvailable();
}

QMediaPlayer *PlaybackController::mediaPlayer() const
{
    return audioManager.getMediaPlayer();
}
