#include "include/streamrecorder.h"

#include <QDir>
#include <QFileInfo>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QStandardPaths>
#include <utility>

namespace {

constexpr int TransferTimeoutMs = 30000;
constexpr int MaxFileNameAttempts = 1000;

QString safeStationName(QString name)
{
    static const QRegularExpression forbidden(QStringLiteral(R"([<>:"/\\|?*\x00-\x1f\x7f])"));
    name = name.trimmed().left(64);
    if (!name.isEmpty() && name.back().isHighSurrogate())
        name.chop(1);
    name.replace(forbidden, QStringLiteral("_"));
    while (name.endsWith('.') || name.endsWith(' '))
        name.chop(1);
    return name.isEmpty() ? QStringLiteral("radio") : name;
}

QString recordingExtension(QString codec)
{
    static const QRegularExpression safeExtension(QStringLiteral("^[a-z0-9]{1,12}$"));
    codec = codec.trimmed().toLower().remove('+');
    return safeExtension.match(codec).hasMatch() ? codec : QStringLiteral("stream");
}

bool isSuccessfulHttpResponse(const QNetworkReply *reply)
{
    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    return status >= 200 && status < 300;
}

} // namespace

StreamRecorder::StreamRecorder(QObject *parent)
    : QObject(parent)
{}

StreamRecorder::~StreamRecorder()
{
    blockSignals(true);
    stopRecording();
}

StreamRecorder::State StreamRecorder::state() const
{
    return m_state;
}

qint64 StreamRecorder::bytesWritten() const
{
    return m_bytesWritten;
}

QString StreamRecorder::filePath() const
{
    return m_filePath;
}

void StreamRecorder::setState(State state)
{
    m_state = state;
    emit stateChanged(state);
}

QString StreamRecorder::openRecordingFile(const QString &directory, const QString &stationName,
                                          const QString &codec, const QDateTime &startTime)
{
    if (directory.isEmpty())
        return tr("The recording directory is unavailable: %1").arg(directory);

    const QString baseName = startTime.toString(QStringLiteral("yyyy-MM-dd_HH-mm-ss-zzz"))
                             + ' ' + safeStationName(stationName);
    const QString extension = recordingExtension(codec);
    const QDir targetDirectory(directory);

    for (int attempt = 1; attempt <= MaxFileNameAttempts; ++attempt) {
        QString name = baseName;
        if (attempt > 1)
            name += QStringLiteral("-%1").arg(attempt);
        const QString path = targetDirectory.filePath(name + '.' + extension);
        m_file.setFileName(path);

        if (m_file.open(QIODevice::WriteOnly | QIODevice::NewOnly | QIODevice::Unbuffered)) {
            m_filePath = path;
            return {};
        }

        const QFileInfo existing(path);
        if (!existing.exists() && !existing.isSymLink())
            return tr("Cannot open recording file %1: %2").arg(path, m_file.errorString());
    }
    return tr("Could not reserve a unique recording filename in %1.").arg(directory);
}

void StreamRecorder::startRecording(const QUrl &url, const QString &stationName, const QString &codec,
                                    QString directory, const QDateTime &startTime)
{
    if (m_state != State::Idle)
        return;

    m_bytesWritten = 0;
    m_filePath.clear();
    setState(State::Starting);
    if (m_state != State::Starting || m_reply)
        return;

    if (!url.isValid() || url.host().isEmpty()
        || (url.scheme() != QStringLiteral("http") && url.scheme() != QStringLiteral("https"))) {
        finishRecording(tr("The recording requires a valid HTTP(S) stream URL."));
        return;
    }

    if (directory.isEmpty())
        directory = QStandardPaths::writableLocation(QStandardPaths::MusicLocation);

    const QString error = openRecordingFile(directory, stationName, codec, startTime);
    if (!error.isEmpty()) {
        finishRecording(error);
        return;
    }

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("TuneScape/6.0"));
    request.setTransferTimeout(TransferTimeoutMs);
    auto *reply = m_manager.get(request);
    m_reply = reply;
    connect(reply, &QNetworkReply::readyRead, this, [this, reply] { writeAvailable(reply); });
    connect(reply, &QNetworkReply::finished, this, [this, reply] { handleFinished(reply); });
    if (reply->isFinished())
        handleFinished(reply);
}

void StreamRecorder::stopRecording()
{
    finishRecording();
}

qint64 StreamRecorder::writeToFile(const char *data, qint64 size)
{
    return m_file.write(data, size);
}

void StreamRecorder::writeAvailable(QNetworkReply *reply)
{
    if (m_reply != reply || !isSuccessfulHttpResponse(reply))
        return;

    const QByteArray data = reply->readAll();
    if (data.isEmpty())
        return;

    qint64 offset = 0;
    while (offset < data.size()) {
        const qint64 written = writeToFile(data.constData() + offset, data.size() - offset);
        if (written <= 0) {
            finishRecording(tr("Cannot write to recording file %1: %2")
                                .arg(m_filePath, m_file.errorString()));
            return;
        }
        offset += written;
        m_bytesWritten += written;
    }

    if (m_state == State::Starting)
        setState(State::Recording);
    if (m_reply == reply)
        emit bytesWrittenChanged(m_bytesWritten);
}

void StreamRecorder::handleFinished(QNetworkReply *reply)
{
    if (m_reply != reply)
        return;

    writeAvailable(reply);
    if (m_reply != reply)
        return;

    const auto status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (status >= 400)
        finishRecording(tr("The recording server returned HTTP %1 (%2).")
                            .arg(status).arg(reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString()));
    else if (reply->error() != QNetworkReply::NoError)
        finishRecording(tr("The recording connection failed: %1").arg(reply->errorString()));
    else if (!isSuccessfulHttpResponse(reply))
        finishRecording(tr("The server did not return a successful HTTP response."));
    else if (m_bytesWritten == 0)
        finishRecording(tr("The stream ended before any audio data was received."));
    else
        finishRecording();
}

void StreamRecorder::finishRecording(QString error)
{
    if (m_state == State::Idle || m_state == State::Stopping)
        return;

    auto *reply = std::exchange(m_reply, nullptr);
    setState(State::Stopping);
    if (reply) {
        disconnect(reply, nullptr, this, nullptr);
        if (!reply->isFinished())
            reply->abort();
        reply->deleteLater();
    }

    if (m_file.isOpen()) {
        if (!m_file.flush() && error.isEmpty())
            error = tr("Cannot flush recording file %1: %2").arg(m_filePath, m_file.errorString());
        m_file.close();
        if (m_file.error() != QFileDevice::NoError && error.isEmpty())
            error = tr("Cannot close recording file %1: %2").arg(m_filePath, m_file.errorString());
    }

    if (error.isEmpty())
        emit recordingFinished(m_filePath, m_bytesWritten);
    else
        emit recordingFailed(error, m_filePath, m_bytesWritten);
    setState(State::Idle);
}
