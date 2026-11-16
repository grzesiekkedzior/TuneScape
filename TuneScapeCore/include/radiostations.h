#ifndef RADIOSTATIONS_H
#define RADIOSTATIONS_H

#include <QObject>
#include <QStringList>

class RadioStations : public QObject
{
    Q_OBJECT
public:
    explicit RadioStations(QObject *parent = nullptr);
    RadioStations(const QString &endpoint);
    RadioStations(const QString &endpoint, const QStringList &baseUrls);

    QStringList getAddresses() const;

private:
    QStringList fullServerAddresses;
};

#endif // RADIOSTATIONS_H
