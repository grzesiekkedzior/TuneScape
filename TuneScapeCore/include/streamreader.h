#ifndef STREAMREADER_H
#define STREAMREADER_H

#include <QNetworkReply>
#include <QObject>
#include <QTimer>
#include <QUrl>

class StreamReader : public QObject
{
    Q_OBJECT

public:
    StreamReader(QObject *parent = nullptr);
    void startStreaming(const QUrl &url);
    void cleanupReplies();

signals:
    void dataReceived(const QString &data);

private slots:
    void onFinished(QNetworkReply *reply);
    void onReadyRead();

private:
    QNetworkAccessManager *manager;
    QNetworkReply *reply = nullptr;
    //QTimer replyCleanupTimer;
};

#endif // STREAMREADER_H
