#include "include/streamreader.h"

#include <QNetworkReply>
#include <QNetworkRequest>

StreamReader::StreamReader(QObject *parent)
    : QObject(parent)
{}

StreamReader::~StreamReader()
{
    stopStreaming();
}

void StreamReader::startStreaming(const QUrl &url, quint64 generation)
{
    stopStreaming();
    if (!url.isValid() || url.isEmpty())
        return;

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("TuneScape/6.0"));
    request.setRawHeader("Icy-MetaData", "1");
    auto *reply = m_manager.get(request);
    m_reply = reply;

    connect(reply, &QNetworkReply::readyRead, this, [this, reply, generation] {
        readAvailable(reply, generation);
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply, generation] {
        readAvailable(reply, generation);
        if (m_reply == reply)
            stopStreaming();
    });
}

void StreamReader::stopStreaming()
{
    auto *reply = m_reply.data();
    m_reply.clear();
    if (reply) {
        disconnect(reply, nullptr, this, nullptr);
        if (!reply->isFinished())
            reply->abort();
        reply->deleteLater();
    }
}

void StreamReader::readAvailable(QNetworkReply *reply, quint64 generation)
{
    if (m_reply != reply || reply->error() != QNetworkReply::NoError)
        return;

    const QByteArray data = reply->readAll();
    if (!data.isEmpty())
        emit dataReceived(generation, QString::fromUtf8(data));
}
