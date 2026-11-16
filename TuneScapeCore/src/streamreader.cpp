#include "include/streamreader.h"

#include <QNetworkReply>
#include <QNetworkRequest>
#include <utility>

StreamReader::StreamReader(QObject *parent)
    : QObject(parent)
{}

StreamReader::~StreamReader()
{
    stopStreaming();
}

void StreamReader::startStreaming(const QUrl &url)
{
    stopStreaming();
    if (!url.isValid() || url.isEmpty())
        return;

    QNetworkRequest request(url);
    request.setRawHeader("Icy-MetaData", "1");
    auto *reply = m_manager.get(request);
    m_reply = reply;

    connect(reply, &QNetworkReply::readyRead, this, [this, reply] { readAvailable(reply); });
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        readAvailable(reply);
        if (m_reply == reply)
            stopStreaming();
    });
}

void StreamReader::stopStreaming()
{
    if (auto *reply = std::exchange(m_reply, nullptr)) {
        disconnect(reply, nullptr, this, nullptr);
        if (!reply->isFinished())
            reply->abort();
        reply->deleteLater();
    }
}

void StreamReader::readAvailable(QNetworkReply *reply)
{
    if (m_reply != reply || reply->error() != QNetworkReply::NoError)
        return;

    const QByteArray data = reply->readAll();
    if (!data.isEmpty())
        emit dataReceived(QString::fromUtf8(data));
}
