#include "include/streamreader.h"

#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStringDecoder>

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
    m_headersRead = false;
    m_interval = 0;
    m_audioRemaining = 0;
    m_metadataRemaining = -1;
    m_metadata.clear();
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

    if (!m_headersRead) {
        m_headersRead = true;
        bool valid = false;
        m_interval = reply->rawHeader("icy-metaint").toInt(&valid);
        if (!valid || m_interval <= 0 || m_interval > 1024 * 1024) m_interval = 0;
        m_audioRemaining = m_interval;
    }
    while (m_reply == reply && reply->bytesAvailable() > 0) {
        if (!m_interval) { reply->read(65536); continue; }
        if (m_audioRemaining > 0) {
            m_audioRemaining -= int(reply->read(qMin(qint64(m_audioRemaining), reply->bytesAvailable())).size());
        } else if (m_metadataRemaining < 0) {
            m_metadataRemaining = int(static_cast<unsigned char>(reply->read(1).at(0))) * 16;
            m_metadata.clear();
            if (!m_metadataRemaining) { m_audioRemaining = m_interval; m_metadataRemaining = -1; }
        } else {
            const auto bytes = reply->read(qMin(qint64(m_metadataRemaining), reply->bytesAvailable()));
            m_metadata += bytes;
            m_metadataRemaining -= int(bytes.size());
            if (!m_metadataRemaining) {
                const auto nul = m_metadata.indexOf('\0');
                const auto block = nul < 0 ? m_metadata : m_metadata.left(nul);
                QStringDecoder decoder(QStringDecoder::Utf8);
                const QString text = decoder.decode(block);
                m_metadataRemaining = -1;
                m_audioRemaining = m_interval;
                emit dataReceived(generation, decoder.hasError() ? QString::fromLatin1(block) : text);
            }
        }
    }
}
