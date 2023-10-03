#ifndef STREAMREADER_H
#define STREAMREADER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrl>

class StreamReader : public QObject
{
    Q_OBJECT

public:
    StreamReader(QObject* parent = nullptr);
    void startStreaming(const QUrl& url);

signals:
    void dataReceived(const QString& data);

private slots:
    void onFinished(QNetworkReply* reply);
    void onReadyRead();

private:
    QNetworkAccessManager* manager;
    QNetworkReply* reply;
};

#endif // STREAMREADER_H
