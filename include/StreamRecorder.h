#ifndef STREAMRECORDER_H
#define STREAMRECORDER_H
#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>

class StreamRecorder : public QObject
{
    Q_OBJECT
public:
    StreamRecorder(QObject *parent = nullptr);
    void recordStream(const QByteArray &data);
    void closeFile();
    void setFileName(const QString &title, const QString extention);
    void loadCurrentAddress(QString address);
    void extracted();
    void startRecording();
    void stopRecording();

    bool getIsRecording() const;
    void setIsRecording(bool newIsRecording);
signals:
    void dataToRecord(QByteArray array);
private slots:
    void recordFile();

private:
    QFile file;
    QString downloadDir{};
    QString filePath{};
    QUrl url;
    bool isRecording = false;

    QNetworkAccessManager *manager = nullptr;
    QNetworkReply *reply = nullptr;
    void clearReply();
};

#endif // STREAMRECORDER_H
