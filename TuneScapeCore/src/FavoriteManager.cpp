#include "include/FavoriteManager.h"
#include <QDebug>
#include <QFile>
#include <QIcon>
#include <QTextStream>

FavoriteManager::FavoriteManager(QObject *parent)
    : QObject{parent}
{}

bool FavoriteManager::isRadioAdded(const QString &streamUrl, const QString &playlist)
{
    QFile file(playlist);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;

    QTextStream in(&file);

    while (!in.atEnd()) {
        QStringList fields = in.readLine().split(',');

        if (fields.size() >= 2 && fields[1].compare(streamUrl, Qt::CaseInsensitive) == 0) {
            return true;
        }
    }

    return false;
}

bool FavoriteManager::toggleFavorite(const QString &streamUrl,
                                     const QString &data,
                                     const QString &playlist)
{
    if (isRadioAdded(streamUrl, playlist)) {
        removeRadio(streamUrl, playlist);
        return false;
    }

    if (data.isEmpty())
        return false;

    QFile file(playlist);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Append)) {
        qDebug() << "Error";
        return false;
    }

    QTextStream out(&file);
    out << data << '\n';

    return true;
}

void FavoriteManager::readFavoriteStationsFromFile(QVector<RadioStation> &stations)
{
    QFile file(RADIO_BROWSER_PLAYLIST);

    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);

        while (!in.atEnd()) {
            QString line = in.readLine();
            QStringList fields = line.split(",");

            if (fields.size() >= 6) {
                RadioStation station;

                station.iconUrl = fields[0];
                station.streamUrl = fields[1];
                station.station = fields[2];
                station.country = fields[3];
                station.genre = fields[4];
                station.homepage = fields[5];

                stations.push_back(station);
            }
        }

        file.close();
    }
}

void FavoriteManager::removeRadio(const QString &streamUrl, const QString &playlist)
{
    QFile inputFile(playlist);
    if (!inputFile.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    QFile outputFile("temp_playlist.txt");
    if (!outputFile.open(QIODevice::WriteOnly | QIODevice::Text))
        return;

    QTextStream in(&inputFile);
    QTextStream out(&outputFile);

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        QStringList fields = line.split(',');

        if (fields.size() >= 2 && fields[1].compare(streamUrl, Qt::CaseInsensitive) == 0) {
            continue; // pomiń wpis do usunięcia
        }

        out << line << '\n';
    }

    inputFile.close();
    outputFile.close();

    // swap file
    if (QFile::remove(playlist) && QFile::rename("temp_playlist.txt", playlist)) {
        qDebug() << "Correct" << streamUrl;
    } else {
        qDebug() << "Error" << streamUrl;
    }
}

bool FavoriteManager::isAddressExists(const QString station, const QString playlist)
{
    QFile file(playlist);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Error opening the file: " << file.errorString();
        return false;
    }

    QTextStream in(&file);
    const QString lowerCaseStation = station.toLower();

    while (!in.atEnd()) {
        QString line = in.readLine();
        if (line.toLower().contains(lowerCaseStation)) {
            file.close();
            qDebug() << "true";
            return true;
        }
    }

    file.close();
    return false;
}
