#ifndef STREAMREADER_H
#define STREAMREADER_H

#include <QNetworkAccessManager>
#include <QObject>
#include <QUrl>

class QNetworkReply;

class StreamReader : public QObject
{
    Q_OBJECT
public:
    explicit StreamReader(QObject *parent = nullptr);
    ~StreamReader() override;

    void startStreaming(const QUrl &url);
    void stopStreaming();

signals:
    void dataReceived(const QString &data);

private:
    void readAvailable(QNetworkReply *reply);

    QNetworkAccessManager m_manager;
    QNetworkReply *m_reply = nullptr;
};

#endif // STREAMREADER_H
