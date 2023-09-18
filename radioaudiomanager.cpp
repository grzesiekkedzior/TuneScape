#include "radioaudiomanager.h"


RadioAudioManager::RadioAudioManager()
{
    player = new QMediaPlayer;
    audioOutput = new QAudioOutput;
    player->setAudioOutput(audioOutput);
    putenv("QT_MEDIA_BACKEND=windows");
    putenv("QT_MULTIMEDIA_PREFERRED_PLUGINS=directshow");
}

RadioAudioManager::~RadioAudioManager()
{
    if (player)
        delete player;
    if (audioOutput)
        delete audioOutput;
}

void RadioAudioManager::loadStream(const QUrl &url)
{
    player->setSource(url);
}

void RadioAudioManager::playStream()
{
    player->play();
}

void RadioAudioManager::stopStream()
{
    player->stop();
}

QMediaPlayer *RadioAudioManager::getMediaPlayer() const
{
    return this->player;
}

QAudioOutput *RadioAudioManager::getAudioAutput() const
{
    return this->audioOutput;
}

void RadioAudioManager::setVolume(int value)
{
    float volume = value * 0.01;
    this->audioOutput->setVolume(volume);
}
