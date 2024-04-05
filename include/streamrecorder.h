#ifndef STREAMRECORDER_H
#define STREAMRECORDER_H
#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include "../ui_mainwindow.h"

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
    void clearLabelText();
    void showStopOnLabel();

    bool getIsRecording() const;
    void setIsRecording(bool newIsRecording);
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void setUI(Ui::MainWindow *ui);
signals:
    void dataToRecord(QByteArray array);
private slots:
    void recordFile();

private:
    Ui::MainWindow *ui;
    QFile file;
    QString downloadDir{};
    QString filePath{};
    QUrl url;
    bool isRecording = false;

    QNetworkAccessManager *manager = nullptr;
    QNetworkReply *reply = nullptr;
    void clearReply();
    void clearRecordLabel();
};

#endif // STREAMRECORDER_H
