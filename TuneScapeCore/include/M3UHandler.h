#ifndef M3UHANDLER_H
#define M3UHANDLER_H

#include <QString>
#include <QVector>
#include "data/RadioStation.h"

class M3UHandler
{
public:
    static QString parse(const QString &text, QVector<RadioStation> &stations);
    static QString serialize(const QVector<RadioStation> &stations, QString &text);
};

#endif // M3UHANDLER_H
