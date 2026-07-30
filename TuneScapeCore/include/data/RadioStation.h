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

    QIcon icon;
};
#endif // RADIOSTATION_H
