#include "include/radiostations.h"
#include "include/RadioBrowserApi.h"

RadioStations::RadioStations(QObject *parent)
    : QObject{parent}
{}

RadioStations::RadioStations(const QString &endpoint)
    : RadioStations(endpoint, RadioBrowserApi::getRadioBrowserBaseUrls())
{}

RadioStations::RadioStations(const QString &endpoint, const QStringList &baseUrls)
{
    for (const QString &address : baseUrls) {
        qDebug() << address;
        fullServerAddresses.append(address + endpoint);
    }
}

QStringList RadioStations::getAddresses() const
{
    return fullServerAddresses;
}
