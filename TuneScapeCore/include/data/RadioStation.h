#ifndef RADIOSTATION_H
#define RADIOSTATION_H
#include <QIcon>
#include <QString>

struct RadioStation
{
    QString station;
    QString genre;
    QString country;
    QString homepage;

    QString streamUrl;
    QString iconUrl;

    QString codec;
    QString language;
    QString countryCode;
    QString state;

    int bitrate = 0;
    int votes = 0;
    int clickCount = 0;

    QIcon icon;
};
#endif // RADIOSTATION_H
