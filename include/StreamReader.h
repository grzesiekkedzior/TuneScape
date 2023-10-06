#ifndef STREAMREADER_H
#define STREAMREADER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrl>
#include <QTimer>

class StreamReader : public QObject
{
    Q_OBJECT

public:
    StreamReader(QObject* parent = nullptr);
    void startStreaming(const QUrl& url);
    void cleanupReplies();

signals:
    void dataReceived(const QString& data);

private slots:
    void onFinished(QNetworkReply* reply);
    void onReadyRead();

private:
    QNetworkAccessManager* manager;
    QNetworkReply* reply;
    QTimer replyCleanupTimer;
};

#endif // STREAMREADER_H
