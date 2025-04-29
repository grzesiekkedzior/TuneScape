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
    isClear = false;

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
            //if (!isRadioInPlaylist(stationName, playlist)) {
                streamAddress = line;
                qDebug() << "streamAddress " << streamAddress;
                if (stationName.isEmpty() || streamAddress.isEmpty())
                    continue;
                saveOnTuneScapeFile(stationName, streamAddress, playlist, isClear);
            //}

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

void M3UHandler::clearTunescapePlaylist(const QString &playlist)
{
    QFile file(playlist);
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        file.close();
    } else {
        QMessageBox::critical(nullptr, "File Error", "Cannot clear file:\n" + playlist);
    }
}


bool M3UHandler::saveOnTuneScapeFile(const QString &stationName, const QString &streamAddress, const QString &playlist, bool &isClear)
{
    if (!isClear) {
        clearTunescapePlaylist(RADIO_BROWSER_PLAYLIST);
        isClear = true;
    }
    QFile file(playlist);

    if (!file.open(QIODevice::WriteOnly |QIODevice::Append | QIODevice::Text)) {  // <--- APPEND !!!
        QMessageBox::critical(nullptr, "File Error", "Cannot open file:\n" + playlist);
        qDebug() << "Error";
        return false;
    }

    QTextStream out(&file);
    out << "," << streamAddress << "," << stationName << "," << "," << "," << "\n" ;
    return true;
}


bool M3UHandler::exportM3Ufile(const QString &playlist)
{
    bool createM3u = convertToM3Ufile(playlist);
    return createM3u ? true : false;
}

bool M3UHandler::convertToM3Ufile(const QString &playlist)
{
    QFile file(playlist);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(nullptr, "File Error", "Cannot open file:\n" + playlist);
        qDebug() << "Error";
        return false;
    }

    // Read favorite stations and convert them into a standard M3U format
    QFile m3uFile;
    if (playlist == RADIO_BROWSER_PLAYLIST)
        m3uFile.setFileName(RADIO_BROWSER_M3U);
    else
        m3uFile.setFileName(RADIO_ICECAST_M3U);

    if (!m3uFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(nullptr, "File Error", "Cannot create file:\n" + playlist);
        qDebug() << "Error";
        return false;
    }

    QTextStream in(&file);
    QTextStream out(&m3uFile);

    out << "#EXTM3U\n";

    while (!in.atEnd()) {
        QString line = in.readLine();
        QStringList fields = line.split(',');

        if (fields.size() >= 3) {
            QString streamUrl = fields[1].trimmed();
            QString stationName = fields[2].trimmed();

            if (!streamUrl.isEmpty() && !stationName.isEmpty()) {
                out << "#EXTINF:-1," << stationName << "\n";
                out << streamUrl << "\n\n";
            }
        }
    }

    file.close();
    m3uFile.close();

    return true;
}

bool M3UHandler::fileExists(const QString path)
{

}

