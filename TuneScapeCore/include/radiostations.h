#ifndef RADIOSTATIONS_H
#define RADIOSTATIONS_H

#include <QObject>
struct ServerAddress
{
    QStringList addresses = {"http://de2.api.radio-browser.info/",
                             "https://nl.api.radio-browser.info/"};
};

class RadioStations : public QObject
{
    Q_OBJECT
public:
    explicit RadioStations(QObject *parent = nullptr);
    RadioStations(const QString &endpoint);

    QStringList getAddresses() const;

private:
    QStringList fullServerAddresses;
    ServerAddress serverAddress;

    QStringList generateFullAddresses(const QString &endpoint) const;
};

#endif // RADIOSTATIONS_H
