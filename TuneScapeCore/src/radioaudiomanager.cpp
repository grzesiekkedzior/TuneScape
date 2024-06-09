#include "include/radioaudiomanager.h"

RadioAudioManager::RadioAudioManager()
    : logLineCount(0)
    , logFile("error_log.txt")
{
    player = new QMediaPlayer;
    audioOutput = new QAudioOutput;
    player->setAudioOutput(audioOutput);
    if (logFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&logFile);
        while (!in.atEnd()) {
            in.readLine();
            logLineCount++;
        }
        logFile.close();
    }
    if (!logFile.open(QIODevice::Append | QIODevice::Text)) {
        qDebug() << "Cannot open log file!";
    }
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

RadioAudioManager::~RadioAudioManager()
{
    if (player)
        delete player;
    if (audioOutput)
        delete audioOutput;
    if (logFile.isOpen()) {
        logFile.close();
    }
}

void RadioAudioManager::loadStream(const QUrl &url)
{
    streamReader.cleanupReplies();

    connect(player, &QMediaPlayer::errorOccurred, this, &RadioAudioManager::logError);
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
    float volume = (value * 0.01);
    this->audioOutput->setVolume(volume);
}

QString RadioAudioManager::getErrorString(QMediaPlayer::Error error)
{
    switch (error) {
    case QMediaPlayer::NoError:
        return "No error";
    case QMediaPlayer::ResourceError:
        return "Resource error";
    case QMediaPlayer::FormatError:
        return "Format error";
    case QMediaPlayer::NetworkError:
        return "Network error";
    case QMediaPlayer::AccessDeniedError:
        return "Access denied";
    default:
        return "Unknown error";
    }
}

void RadioAudioManager::resetLogFile()
{
    if (logFile.isOpen()) {
        logFile.close();
    }

    if (logFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        logFile.close();
        logLineCount = 0;
    } else {
        qDebug() << "Cannot reset log file!";
    }

    if (!logFile.open(QIODevice::Append | QIODevice::Text)) {
        qDebug() << "Cannot open log file!";
    }
}

void RadioAudioManager::logError(QMediaPlayer::Error error)
{
    if (logLineCount >= MAX_LOG_LINES) {
        resetLogFile();
    }

    QString errorMsg = getErrorString(error);
    if (logFile.isOpen()) {
        QTextStream out(&logFile);
        out << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << " - " << errorMsg
            << "\n";
        logLineCount++;
    } else {
        qDebug() << "Log file is not open!";
    }
    qDebug() << "Player error occurred:" << errorMsg;
}
