#ifndef RADIOAUDIOMANAGER_H
#define RADIOAUDIOMANAGER_H
#include <QAbstractNativeEventFilter>
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
    void handleSystemSleep();
    void handleSystemWake();

    bool getIsPlaying() const;
    void setIsPlaying(bool newIsPlaying);

signals:

private slots:
    void logError(QMediaPlayer::Error error);

private:
    QMediaPlayer *player;
    QAudioOutput *audioOutput;
    StreamReader streamReader;
    int logLineCount;
    const int MAX_LOG_LINES = 10000;
    bool isPlaying = false;

    QFile logFile;

    QString getErrorString(QMediaPlayer::Error error);
    void resetLogFile();
};

class SystemEventFilter : public QAbstractNativeEventFilter
{
public:
    SystemEventFilter(RadioAudioManager *manager)
        : manager(manager)
    {}
    bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result) override;

private:
    RadioAudioManager *manager;
};

#endif // RADIOAUDIOMANAGER_H
