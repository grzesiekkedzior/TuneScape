#include "include/controller/PlaybackController.h"

PlaybackController::PlaybackController(QObject *parent)
    : QObject{parent}
{}

void PlaybackController::play(const QUrl &streamUrl)
{
    state.currentStreamUrl = streamUrl.toString();
    state.isPlaying = true;
    state.isPause = false;

    audioManager.play(streamUrl);
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
