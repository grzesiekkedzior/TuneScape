#include "include/radioaudiomanager.h"

RadioAudioManager::RadioAudioManager() {
    player = new QMediaPlayer;
    audioOutput = new QAudioOutput;
    player->setAudioOutput(audioOutput);

#ifdef __unix__
    qputenv("QT_MEDIA_BACKEND", "gstreamer");
    // setenv("QT_MEDIA_BACKEND", "gstreamer", 1);
#elif defined(_WIN32) || defined(WIN32)
    qputenv("QT_MEDIA_BACKEND", "windows");
    qputenv("QT_MULTIMEDIA_PREFERRED_PLUGINS", "directshow");
    // setenv("QT_MEDIA_BACKEND", "windows", 1);
    // setenv("QT_MULTIMEDIA_PREFERRED_PLUGINS", "directshow", 1);

#endif
}

RadioAudioManager::~RadioAudioManager() {
    if (player)
        delete player;
    if (audioOutput)
        delete audioOutput;
}

void RadioAudioManager::loadStream(const QUrl &url) {
    streamReader.cleanupReplies();
    // player->stop();
    // player->deleteLater();
    // audioOutput->deleteLater();

    // player = new QMediaPlayer;
    // audioOutput = new QAudioOutput;
    // player->setAudioOutput(audioOutput);

    player->setSource(url);
}

void RadioAudioManager::playStream() { player->play(); }

void RadioAudioManager::stopStream() { player->stop(); }

QMediaPlayer *RadioAudioManager::getMediaPlayer() const { return this->player; }

QAudioOutput *RadioAudioManager::getAudioAutput() const {
    return this->audioOutput;
}

void RadioAudioManager::setVolume(int value) {
    float volume = (value * 0.01);
    this->audioOutput->setVolume(volume);
}
