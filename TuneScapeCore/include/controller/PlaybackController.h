#ifndef PLAYBACKCONTROLLER_H
#define PLAYBACKCONTROLLER_H

#include "include/data/RadioStation.h"
#include "include/radioaudiomanager.h"
#include <QPointer>
#include <QTimer>
#include <optional>

class PlaybackController : public QObject
{
    Q_OBJECT
public:
    enum class State { Idle, Connecting, Buffering, Playing, Paused, Stopped, Error };
    Q_ENUM(State)

    explicit PlaybackController(QObject *parent = nullptr);
    explicit PlaybackController(RadioAudioManager &backend, QObject *parent = nullptr);
    ~PlaybackController() override;

    void play(const RadioStation &station);
    void pause();
    void resume();
    void stop();
    void toggle(const std::optional<RadioStation> &selection = {});
    void setVolume(int volume);

    State state() const;
    const std::optional<RadioStation> &station() const;
    QUrl streamUrl() const;
    QString lastError() const;
    quint64 generation() const;
    bool canPause() const;
    bool hasActiveSession() const;
    bool matches(const RadioStation &station) const;

signals:
    void sessionChanged();
    void stateChanged(State state);

private:
    enum class Intent { Stop, Play, Pause };

    void startAttempt();
    void endAttempt();
    void transition(State next, bool newSession = false);
    void updateFromBackend(quint64 attempt);
    void fail(const QString &message);
    void systemSuspended();
    void systemResumed();

    QPointer<RadioAudioManager> m_backend;
    QTimer m_watchdog;
    std::optional<RadioStation> m_station;
    QString m_error;
    quint64 m_generation = 0;
    quint64 m_attempt = 0;
    Intent m_intent = Intent::Stop;
    State m_state = State::Idle;
    QMediaPlayer::PlaybackState m_backendState = QMediaPlayer::StoppedState;
    QMediaPlayer::MediaStatus m_mediaStatus = QMediaPlayer::NoMedia;
    bool m_started = false;
    bool m_suspended = false;
};

#endif // PLAYBACKCONTROLLER_H
