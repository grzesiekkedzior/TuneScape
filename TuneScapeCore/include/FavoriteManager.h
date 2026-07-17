#ifndef FAVORITEMANAGER_H
#define FAVORITEMANAGER_H
#include <QObject>
#include <QVector>
#include "data/RadioStation.h"
#include "ui_mainwindow.h"

class FavoriteManager : public QObject
{
    Q_OBJECT
public:
    explicit FavoriteManager(Ui::MainWindow *ui, QObject *parent = nullptr);

    bool isRadioAdded(const QString data, const QString playlist);
    void toggleFavorite(const QString &stationName, const QString &data, const QString &playlist);
    void readFavoriteStationsFromFile(QVector<RadioStation> &stations);
    void removeRadio(const QString data, const QString playlist);
    bool isAddressExists(const QString station, const QString playlist);

private:
    Ui::MainWindow *m_ui;
    const QString RADIO_BROWSER_PLAYLIST = "radiobrowser.txt";
};

#endif // FAVORITEMANAGER_H
