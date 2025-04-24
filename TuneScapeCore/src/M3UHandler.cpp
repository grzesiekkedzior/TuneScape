#include "include/M3UHandler.h"
#include <QFile>
#include <QDebug>
#include <QMessageBox>

M3UHandler::M3UHandler(QObject *parent)
{

}

void M3UHandler::importM3Ufile(const QString path ,const QString playlist)
{
    bool extInf = false;
    QString stationName{};
    QString streamAddress{};

    if(!isM3Ufile(path))
        return;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(nullptr, "File Error", "Cannot open file:\n" + path);
        qWarning() << "Can not open file." << path;
        return;
    }

    while (!file.atEnd()) {
        QString line = file.readLine().trimmed();
        if (line.isEmpty())
            continue;

        if (line.startsWith("#EXTINF:")) {
            extInf = true;
            stationName = line.section(',', 1).trimmed();
            qDebug() << "stationName " << stationName;
        } else if (extInf && line.startsWith("http")) {
            if (!isRadioInPlaylist(stationName, playlist)) {
                streamAddress = line;
                qDebug() << "streamAddress " << streamAddress;
                if (stationName.isEmpty() || streamAddress.isEmpty())
                    continue;
                saveOnTuneScapeFile(stationName, streamAddress);
            }

            extInf = false;
        }
    }

}

bool M3UHandler::isM3Ufile(const QString &path) const
{
    return path.endsWith(".m3u", Qt::CaseInsensitive);
}

bool M3UHandler::isRadioInPlaylist(const QString& stationName, const QString& playlist)
{
    QFile file(playlist);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;

    QTextStream in(&file);
    while (!in.atEnd()) {
        const QString line = in.readLine().trimmed();
        const QString nameFromPlaylist = line.section(',', 2, 2).trimmed();

        qDebug() << "stationName:" << stationName << "vs" << nameFromPlaylist;

        if (nameFromPlaylist.compare(stationName, Qt::CaseInsensitive) == 0)
            return true;
    }

    return false;
}

bool M3UHandler::saveOnTuneScapeFile(QString stationName, QString streamAddress)
{

}

bool M3UHandler::exportM3Ufile()
{

}

bool M3UHandler::convertToM3Ufile()
{

}

bool M3UHandler::fileExists(const QString path)
{

}

