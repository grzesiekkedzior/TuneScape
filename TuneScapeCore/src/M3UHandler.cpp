#include "include/M3UHandler.h"
#include <QFile>
#include <QDebug>
#include <QMessageBox>

M3UHandler::M3UHandler(QObject *parent)
{

}

QList<M3UHandler::TableRow> M3UHandler::importM3Ufile(const QString path)
{
    QList<TableRow> tableRows;
    QList<QString> lines;
    bool extInf = false;

    if(!isM3Ufile(path))
        return {};

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(nullptr, "File Error", "Cannot open file:\n" + path);
        qWarning() << "Can not open file." << path;
        return {};
    }

    while (!file.atEnd()) {
        QString line = file.readLine();
        lines.append(line);
    }

    for (const QString& rawLine : lines) {
        QString line = rawLine.trimmed();
        if (line.isEmpty())
            continue;
        if (line.startsWith("EXTINF:")) {
            extInf = true;

        } else if (extInf && line.startsWith("http")) {
            if (!isRadioInPlaylist(line))
                saveOnTuneScapeFile(line);

            extInf = false;
        }
    }

}

bool M3UHandler::isM3Ufile(const QString &path) const
{
    return path.endsWith(".m3u", Qt::CaseInsensitive);
}

bool M3UHandler::isRadioInPlaylist(const QString data)
{

}

bool M3UHandler::saveOnTuneScapeFile(QString line)
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

