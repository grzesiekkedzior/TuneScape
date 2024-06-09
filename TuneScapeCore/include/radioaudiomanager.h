#ifndef RADIOAUDIOMANAGER_H
#define RADIOAUDIOMANAGER_H
#include "include/streamreader.h"

#include <QAudioOutput>
#include <QFile>
#include <QMediaPlayer>
#include <QObject>

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
    void setVolume(int value);

signals:

private slots:
    void logError(QMediaPlayer::Error error);

private:
    QMediaPlayer *player;
    QAudioOutput *audioOutput;
    StreamReader streamReader;
    int logLineCount;
    const int MAX_LOG_LINES = 10000;

    QFile logFile;

    QString getErrorString(QMediaPlayer::Error error);
    void resetLogFile();
};

#endif // RADIOAUDIOMANAGER_H
