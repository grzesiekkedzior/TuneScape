#include "include/AppConfig.h"
#include <QFile>
#include "qdebug.h"

AppConfig::AppConfig(QString path)
{
    file = new QFile(path);
    if (!file->exists()) {
        if (file->open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(file);
            out << "notifications=true\n";
            file->close();
        } else {
            qDebug() << "Can't create file: " << file->errorString();
        }
    }
}

bool AppConfig::checkFile()
{
    return file->exists();
}

bool AppConfig::checkBoolState()
{
    if (!file->open(QIODevice::ReadOnly | QIODevice::Text))
        return false;

    while (!file->atEnd()) {
        QByteArray line = file->readLine().trimmed();
        if (line.contains("notifications")) {
            QList<QByteArray> parts = line.split('=');
            if (parts.size() == 2 && parts[1].trimmed() == "true") {
                file->close();
                return true;
            }
        }
    }
    file->close();
    return false;
}

bool AppConfig::changeBoolState(bool state)
{
    if (!checkFile())
        return false;

    QVector<QByteArray> lines;

    if (!file->open(QIODevice::ReadOnly | QIODevice::Text))
        return false;

    while (!file->atEnd()) {
        QByteArray line = file->readLine();
        lines.append(line);
    }
    file->close();

    bool found = false;
    for (int i = 0; i < lines.size(); ++i) {
        if (lines[i].contains("notifications")) {
            QList<QByteArray> parts = lines[i].split('=');
            if (parts.size() == 2) {
                parts[1] = state ? "true\n" : "false\n";
                lines[i] = parts.join('=');
                found = true;
                break;
            }
        }
    }

    if (!found)
        return false;

    if (!file->open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
        return false;

    for (const QByteArray &line : lines) {
        file->write(line);
    }

    file->close();
    return true;
}
