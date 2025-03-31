#include "include/radiostations.h"
#include "include/RadioBrowserApi.h"

RadioStations::RadioStations(QObject *parent)
    : QObject{parent}
{}

RadioStations::RadioStations(const QString &endpoint)
{
    serverAddress.addresses = RadioBrowserApi::getRadioBrowserBaseUrls();
    for (const QString &address : serverAddress.addresses) {
        qDebug() << address;
    }
    fullServerAddresses = generateFullAddresses(endpoint);
}

QStringList RadioStations::getAddresses() const
{
    return fullServerAddresses;
}

QStringList RadioStations::generateFullAddresses(const QString &endpoint) const
{
    QStringList fullServerAddress;
    for (const QString &address : serverAddress.addresses) {
        fullServerAddress.append(address + endpoint);
    }
    return fullServerAddress;
}
