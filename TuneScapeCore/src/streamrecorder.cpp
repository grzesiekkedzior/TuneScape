#include "include/streamrecorder.h"
#include <QDir>
#include <QStandardPaths>
#include <QTimer>

// QTC_TEMP
StreamRecorder::StreamRecorder(QObject *parent)
    : QObject(parent)
    , manager(new QNetworkAccessManager(this))
{
    downloadDir = QStandardPaths::writableLocation(QStandardPaths::MusicLocation);
}

void StreamRecorder::recordStream(const QByteArray &data)
{
    file.write(data);
}

void StreamRecorder::closeFile()
{
    if (file.isOpen()) {
        file.close();
        qDebug() << "File closed";
    }
}

void StreamRecorder::setFileName(const QString &title, const QString extention)
{
    QDateTime currentDateTime = QDateTime::currentDateTime();

    // Konwertuj datę i czas na string w określonym formacie
    QString dateTimeString = currentDateTime.toString("[yyyy-hh-mm]");

    filePath = downloadDir + QDir::separator();
    filePath += dateTimeString + title + "." + extention;
    file.setFileName(filePath);
}

void StreamRecorder::loadCurrentAddress(QString address)
{
    url.setUrl(address);
    if (!file.isOpen()) {
        if (file.open(QIODevice::WriteOnly)) {
            qDebug() << "File opened for writing";
        } else {
            qDebug() << "Error opening file for writing";
            return;
        }
    }
}

void StreamRecorder::clearReply()
{
    if (reply != nullptr)
        reply->deleteLater();
}

void StreamRecorder::clearRecordLabel()
{
    QTimer::singleShot(500, this, &StreamRecorder::showStopOnLabel);
    QTimer::singleShot(2000, this, &StreamRecorder::clearLabelText);
}

void StreamRecorder::clearLabelText()
{
    ui->recordLabel->clear();
}

void StreamRecorder::showStopOnLabel()
{
    ui->recordLabel->setText("STOP RECORDING");
}

void StreamRecorder::startRecording()
{
    // check url!!!
    QNetworkRequest request(url);
    reply = manager->get(request);
    connect(reply, &QNetworkReply::readyRead, this, &StreamRecorder::recordFile);
    connect(reply, &QNetworkReply::downloadProgress, this, &StreamRecorder::downloadProgress);
}

void StreamRecorder::stopRecording()
{
    closeFile();
    clearReply();
    clearRecordLabel();

    //This is not ... but in some cases help
    if (reply != nullptr) {
        disconnect(reply, &QNetworkReply::readyRead, this, &StreamRecorder::recordFile);
    }
}

bool StreamRecorder::getIsRecording() const
{
    return isRecording;
}

void StreamRecorder::setIsRecording(bool newIsRecording)
{
    isRecording = newIsRecording;
}

void StreamRecorder::downloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    double megabytesReceived = static_cast<double>(bytesReceived) / (1024 * 1024);
    QString text = QString("%1 MB").arg(QString::number(megabytesReceived, 'f', 2));
    ui->recordLabel->setText(text);
}

void StreamRecorder::setUI(Ui::MainWindow *ui)
{
    this->ui = ui;
}

void StreamRecorder::recordFile()
{
    while (reply->bytesAvailable()) {
        QByteArray data = reply->readAll();
        file.write(data);
    }
}
