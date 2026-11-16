#include "include/controller/PlaybackController.h"
#include "include/FavoriteManager.h"

namespace {
constexpr int BufferTimeoutMs = 15000;
}

PlaybackController::PlaybackController(QObject *parent)
    : PlaybackController(*new RadioAudioManager, parent)
{
    m_backend->setParent(this);
}

PlaybackController::PlaybackController(RadioAudioManager &backend, QObject *parent)
    : QObject(parent)
    , m_backend(&backend)
{
    m_watchdog.setSingleShot(true);
    connect(&m_watchdog, &QTimer::timeout, this, [this] {
        fail(m_state == State::Buffering ? tr("The stream stopped buffering.")
                                        : tr("The connection to the stream timed out."));
    });
    connect(&backend, &RadioAudioManager::mediaStatusChanged, this, [this](auto id, auto status) {
        if (id != m_attempt)
            return;
        m_mediaStatus = status;
        updateFromBackend(id);
    });
    connect(&backend, &RadioAudioManager::playbackStateChanged, this, [this](auto id, auto state) {
        if (id != m_attempt)
            return;
        m_backendState = state;
        updateFromBackend(id);
    });
    connect(&backend, &RadioAudioManager::playbackError, this, [this](auto id, const QString &error) {
        if (id == m_attempt && m_intent == Intent::Play)
            fail(error);
    });
    connect(&backend, &RadioAudioManager::systemSuspended, this, &PlaybackController::systemSuspended);
    connect(&backend, &RadioAudioManager::systemResumed, this, &PlaybackController::systemResumed);
}

PlaybackController::~PlaybackController()
{
    endAttempt();
}

void PlaybackController::play(const RadioStation &station)
{
    // Callers may pass our current station when retrying or resuming.
    const RadioStation next = station;
    if (hasActiveSession() && matches(next) && streamUrl() == QUrl(next.streamUrl)) {
        if (m_intent == Intent::Pause)
            resume();
        return;
    }

    endAttempt();
    m_station = next;
    m_error.clear();
    m_intent = Intent::Play;
    ++m_generation;
    const auto url = streamUrl();
    if (!url.isValid() || url.isEmpty()) {
        fail(tr("The station does not have a valid stream URL."));
        return;
    }
    const auto attempt = m_attempt;
    transition(State::Connecting, true);
    if (attempt == m_attempt && canPause())
        startAttempt();
}

void PlaybackController::pause()
{
    if (!canPause())
        return;
    m_intent = Intent::Pause;
    endAttempt();
    transition(State::Paused);
}

void PlaybackController::resume()
{
    if (m_intent != Intent::Pause)
        return;
    m_intent = Intent::Play;
    const auto attempt = m_attempt;
    transition(State::Connecting);
    if (attempt == m_attempt && canPause())
        startAttempt();
}

void PlaybackController::stop()
{
    if (m_state == State::Idle || m_state == State::Stopped)
        return;
    m_intent = Intent::Stop;
    endAttempt();
    m_station.reset();
    m_error.clear();
    ++m_generation;
    transition(State::Stopped, true);
}

void PlaybackController::toggle(const std::optional<RadioStation> &selection)
{
    if (canPause()) {
        pause();
    } else if (m_state == State::Paused) {
        resume();
    } else if (selection && (m_state != State::Error || !matches(*selection))) {
        play(*selection);
    } else if (m_state == State::Error && m_station) {
        play(*m_station);
    }
}

void PlaybackController::setVolume(int volume)
{
    if (m_backend)
        m_backend->setVolume(volume);
}

PlaybackController::State PlaybackController::state() const { return m_state; }
const std::optional<RadioStation> &PlaybackController::station() const { return m_station; }
QString PlaybackController::lastError() const { return m_error; }
quint64 PlaybackController::generation() const { return m_generation; }
bool PlaybackController::canPause() const { return m_intent == Intent::Play; }
bool PlaybackController::hasActiveSession() const { return m_intent != Intent::Stop; }

QUrl PlaybackController::streamUrl() const
{
    return m_station ? QUrl(m_station->streamUrl) : QUrl();
}

bool PlaybackController::matches(const RadioStation &station) const
{
    return m_station && FavoriteManager::sameStation(*m_station, station);
}

void PlaybackController::startAttempt()
{
    if (m_suspended)
        return;
    if (!m_backend) {
        fail(tr("The audio backend is unavailable."));
        return;
    }
    m_backendState = QMediaPlayer::StoppedState;
    m_mediaStatus = QMediaPlayer::NoMedia;
    m_started = false;
    m_watchdog.start(BufferTimeoutMs);
    m_backend->start(streamUrl(), ++m_attempt);
}

void PlaybackController::endAttempt()
{
    ++m_attempt;
    m_watchdog.stop();
    if (m_backend)
        m_backend->stop();
}

void PlaybackController::transition(State next, bool newSession)
{
    const bool changed = m_state != next;
    m_state = next;
    const auto generation = m_generation;
    if (newSession)
        emit sessionChanged();
    // A new station can replace another while both are still Connecting.
    if ((changed || newSession) && m_generation == generation && m_state == next)
        emit stateChanged(next);
}

void PlaybackController::updateFromBackend(quint64 attempt)
{
    if (!canPause() || m_suspended)
        return;

    State next = State::Connecting;
    switch (m_mediaStatus) {
    case QMediaPlayer::InvalidMedia:
        // Let the backend report its specific error if it follows the status signal.
        QTimer::singleShot(0, this, [this, attempt] {
            if (attempt == m_attempt && m_mediaStatus == QMediaPlayer::InvalidMedia)
                fail(tr("The audio backend could not open this stream."));
        });
        return;
    case QMediaPlayer::EndOfMedia:
        stop();
        return;
    case QMediaPlayer::StalledMedia:
        next = State::Buffering;
        break;
    case QMediaPlayer::BufferingMedia:
        next = State::Buffering;
        // Qt reports usable audio in this status, even if a live stream keeps buffering.
        [[fallthrough]];
    case QMediaPlayer::LoadedMedia:
    case QMediaPlayer::BufferedMedia:
        if (m_backendState == QMediaPlayer::PlayingState) {
            m_started = true;
            next = State::Playing;
        }
        break;
    case QMediaPlayer::NoMedia:
    case QMediaPlayer::LoadingMedia:
        break;
    }

    if (m_started && m_backendState == QMediaPlayer::StoppedState) {
        QTimer::singleShot(0, this, [this, attempt] {
            if (attempt == m_attempt && m_backendState == QMediaPlayer::StoppedState)
                fail(tr("The audio stream stopped unexpectedly."));
        });
    }
    if (next != m_state) {
        if (next == State::Playing)
            m_watchdog.stop();
        else
            m_watchdog.start(BufferTimeoutMs);
        transition(next);
    }
}

void PlaybackController::fail(const QString &message)
{
    if (!hasActiveSession())
        return;
    m_intent = Intent::Stop;
    endAttempt();
    m_error = message.isEmpty() ? tr("Unknown playback error.") : message;
    ++m_generation;
    transition(State::Error, true);
}

void PlaybackController::systemSuspended()
{
    if (m_suspended)
        return;
    m_suspended = true;
    if (canPause()) {
        endAttempt();
        transition(State::Connecting);
    }
}

void PlaybackController::systemResumed()
{
    if (!m_suspended)
        return;
    m_suspended = false;
    if (canPause())
        startAttempt();
}
