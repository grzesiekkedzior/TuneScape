#ifndef JSONLISTPROCESSOR_H
#define JSONLISTPROCESSOR_H

#include "data/RadioStation.h"

#include <QJsonArray>
#include <QVector>

class JsonListProcessor
{
public:
    static QVector<RadioStation> stationsFromJson(const QJsonArray &array);
};

#endif // JSONLISTPROCESSOR_H
