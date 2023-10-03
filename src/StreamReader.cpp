#include "include/Streamreader.h"
#include <QByteArray>
#include <QDebug>

StreamReader::StreamReader(QObject* parent) : QObject(parent) {
    manager = new QNetworkAccessManager(this);

    connect(manager, &QNetworkAccessManager::finished, this, &StreamReader::onFinished);
}

void StreamReader::startStreaming(const QUrl& url) {
    QNetworkRequest request(url);
    request.setRawHeader("Icy-MetaData", "1");
    reply = manager->get(request);

    connect(reply, &QNetworkReply::readyRead, this, &StreamReader::onReadyRead);
}

void StreamReader::onFinished(QNetworkReply* reply) {
    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "Error:" << reply->errorString();
        return;
    }
}

void StreamReader::onReadyRead() {
    while (reply->bytesAvailable()) {
        QByteArray data = reply->readAll();
        emit dataReceived(data);
    }
}
