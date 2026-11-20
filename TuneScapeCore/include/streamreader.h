#ifndef STREAMREADER_H
#define STREAMREADER_H

#include <QNetworkAccessManager>
#include <QObject>
#include <QPointer>
#include <QUrl>

class QNetworkReply;

class StreamReader : public QObject
{
    Q_OBJECT
public:
    explicit StreamReader(QObject *parent = nullptr);
    ~StreamReader() override;

    void startStreaming(const QUrl &url, quint64 generation);
    void stopStreaming();

signals:
    void dataReceived(quint64 generation, const QString &data);

private:
    void readAvailable(QNetworkReply *reply, quint64 generation);

    QNetworkAccessManager m_manager;
    QPointer<QNetworkReply> m_reply;
    bool m_headersRead = false;
    int m_interval = 0;
    int m_audioRemaining = 0;
    int m_metadataRemaining = -1;
    QByteArray m_metadata;
};

#endif // STREAMREADER_H
