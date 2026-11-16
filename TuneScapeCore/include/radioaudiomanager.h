#ifndef RADIOAUDIOMANAGER_H
#define RADIOAUDIOMANAGER_H

#include <QFile>
#include <QMediaPlayer>
#include <QPointer>
#include <memory>

class QAudioOutput;
class QAbstractNativeEventFilter;

class RadioAudioManager : public QObject
{
    Q_OBJECT
public:
    explicit RadioAudioManager(QObject *parent = nullptr);
    ~RadioAudioManager() override;

    virtual void start(const QUrl &url, quint64 attempt);
    virtual void stop();
    void setVolume(int value);

signals:
    void playbackStateChanged(quint64 attempt, QMediaPlayer::PlaybackState state);
    void mediaStatusChanged(quint64 attempt, QMediaPlayer::MediaStatus status);
    void playbackError(quint64 attempt, const QString &message);
    void systemSuspended();
    void systemResumed();

private:
    void logError(const QString &error);
    void initializeAudio();

    QPointer<QMediaPlayer> m_player;
    QAudioOutput *m_audioOutput = nullptr;
    int m_volume = 100;
    int m_logLineCount = 0;
    QFile m_logFile{"error_log.txt"};
#ifdef Q_OS_WIN
    std::unique_ptr<QAbstractNativeEventFilter> m_powerFilter;
#endif
};

#endif // RADIOAUDIOMANAGER_H
