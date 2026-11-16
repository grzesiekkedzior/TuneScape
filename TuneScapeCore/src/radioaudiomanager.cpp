#include "include/radioaudiomanager.h"
#include <QAudioOutput>
#include <QCoreApplication>
#include <QDateTime>
#include <QTextStream>

#ifdef Q_OS_WIN
#include <QAbstractNativeEventFilter>
#include <windows.h>
#endif

namespace {
constexpr int MaxLogLines = 10000;

#ifdef Q_OS_WIN
class PowerEventFilter : public QAbstractNativeEventFilter
{
public:
    explicit PowerEventFilter(RadioAudioManager &backend) : m_backend(backend) {}

    bool nativeEventFilter(const QByteArray &type, void *message, qintptr *) override
    {
        if (type != "windows_generic_MSG" && type != "windows_dispatcher_MSG")
            return false;
        const auto *msg = static_cast<const MSG *>(message);
        if (!msg || msg->message != WM_POWERBROADCAST)
            return false;
        switch (msg->wParam) {
        case PBT_APMSUSPEND:
            emit m_backend.systemSuspended();
            break;
        case PBT_APMRESUMEAUTOMATIC:
        case PBT_APMRESUMECRITICAL:
        case PBT_APMRESUMESUSPEND:
            emit m_backend.systemResumed();
            break;
        }
        return false;
    }

private:
    RadioAudioManager &m_backend;
};
#endif
} // namespace

RadioAudioManager::RadioAudioManager(QObject *parent)
    : QObject(parent)
{}

RadioAudioManager::~RadioAudioManager()
{
#ifdef Q_OS_WIN
    if (m_powerFilter && QCoreApplication::instance())
        QCoreApplication::instance()->removeNativeEventFilter(m_powerFilter.get());
#endif
    stop();
}

void RadioAudioManager::initializeAudio()
{
    if (m_audioOutput)
        return;
    m_audioOutput = new QAudioOutput(this);
    m_audioOutput->setVolume(m_volume / 100.0);
#ifdef Q_OS_WIN
    if (auto *app = QCoreApplication::instance()) {
        m_powerFilter = std::make_unique<PowerEventFilter>(*this);
        app->installNativeEventFilter(m_powerFilter.get());
    }
#endif
    if (m_logFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream input(&m_logFile);
        while (!input.atEnd() && m_logLineCount < MaxLogLines) {
            input.readLine();
            ++m_logLineCount;
        }
        m_logFile.close();
    }
}

void RadioAudioManager::start(const QUrl &url, quint64 attempt)
{
    stop();
    initializeAudio();
    auto *player = new QMediaPlayer(this);
    m_player = player;
    connect(player, &QMediaPlayer::playbackStateChanged, this, [this, player, attempt](auto state) {
        if (m_player == player)
            emit playbackStateChanged(attempt, state);
    });
    connect(player, &QMediaPlayer::mediaStatusChanged, this, [this, player, attempt](auto status) {
        if (m_player == player)
            emit mediaStatusChanged(attempt, status);
    });
    connect(player, &QMediaPlayer::errorOccurred, this,
            [this, player, attempt](auto error, const QString &message) {
                if (m_player != player || error == QMediaPlayer::NoError)
                    return;
                logError(message);
                emit playbackError(attempt, message);
            });

    if (!player->isAvailable()) {
        const auto message = tr("The audio backend is unavailable.");
        logError(message);
        emit playbackError(attempt, message);
        return;
    }
    player->setAudioOutput(m_audioOutput);
    if (m_player != player)
        return;
    player->setSource(url);
    if (m_player == player)
        player->play();
}

void RadioAudioManager::stop()
{
    auto *player = m_player.data();
    m_player.clear();
    if (!player)
        return;
    disconnect(player, nullptr, this, nullptr);
    player->stop();
    player->setAudioOutput(nullptr);
    player->setSource(QUrl());
    player->deleteLater();
}

void RadioAudioManager::setVolume(int value)
{
    m_volume = qBound(0, value, 100);
    if (m_audioOutput)
        m_audioOutput->setVolume(m_volume / 100.0);
}

void RadioAudioManager::logError(const QString &error)
{
    const bool truncate = m_logLineCount >= MaxLogLines;
    if (m_logFile.open(QIODevice::Text | (truncate ? QIODevice::WriteOnly : QIODevice::Append))) {
        QTextStream output(&m_logFile);
        output << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")
               << " - " << error << Qt::endl;
        m_logFile.close();
        m_logLineCount = truncate ? 1 : m_logLineCount + 1;
    }
}
