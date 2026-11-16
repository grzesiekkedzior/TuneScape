#ifndef STREAMRECORDER_H
#define STREAMRECORDER_H

#include <QDateTime>
#include <QFile>
#include <QNetworkAccessManager>
#include <QObject>
#include <QUrl>

class QNetworkReply;

class StreamRecorder : public QObject
{
    Q_OBJECT
public:
    enum class State { Idle, Starting, Recording, Stopping };
    Q_ENUM(State)

    explicit StreamRecorder(QObject *parent = nullptr);
    ~StreamRecorder() override;

    State state() const;
    qint64 bytesWritten() const;
    QString filePath() const;

    void startRecording(const QUrl &url, const QString &stationName, const QString &codec,
                        QString directory = {}, const QDateTime &startTime = QDateTime::currentDateTime());
    void stopRecording();

signals:
    void stateChanged(StreamRecorder::State state);
    void bytesWrittenChanged(qint64 bytes);
    void recordingFinished(const QString &path, qint64 bytes);
    void recordingFailed(const QString &error, const QString &path, qint64 bytes);

protected:
    virtual qint64 writeToFile(const char *data, qint64 size);

private:
    void setState(State state);
    QString openRecordingFile(const QString &directory, const QString &stationName,
                              const QString &codec, const QDateTime &startTime);
    void writeAvailable(QNetworkReply *reply);
    void handleFinished(QNetworkReply *reply);
    void finishRecording(QString error = {});

    QNetworkAccessManager m_manager;
    QNetworkReply *m_reply = nullptr;
    QFile m_file;
    QString m_filePath;
    qint64 m_bytesWritten = 0;
    State m_state = State::Idle;
};

#endif // STREAMRECORDER_H
