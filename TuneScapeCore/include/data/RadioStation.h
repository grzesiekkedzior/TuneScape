#ifndef RADIOSTATION_H
#define RADIOSTATION_H
#include <QString>

struct RadioStation
{
    QString station;
    QString genre;
    QString country;
    QString homepage;

    QString streamUrl;
    QString iconUrl;
};
#endif // RADIOSTATION_H
