#ifndef COUNTRY_H
#define COUNTRY_H

#include <QDesktopServices>
#include <QMenu>
#include <QObject>
#include <QPixmap>
#include <QSharedPointer>
#include <QString>
#include <QUrl>
#include <QVector>
#include "RadioBrowserApi.h"
#include "container.h"
#include "controller/PlaybackController.h"
#include "controller/PlayerUIController.h"
#include "customcolordelegate.h"
#include "include/theme.h"
#include "model/RadioStationsModel.h"

namespace Ui {
class MainWindow;
}

class RadioList;

class Country : public QObject
{
    Q_OBJECT

public:
    Country() = default;

    ~Country() override = default;

    void setData(Ui::MainWindow *ui, RadioList *radioList);
    void load();

    bool getIsPlaying() const;
    void setIsPlaying(bool isPlaying);

    int getCurrentIndexPlaying() const;
    void setCurrentIndexPlaying(int index);

    const RadioStation &getCurrentStation() const;

    void clearTableColor();

    void setTheme(Theme *newTheme);
public slots:
    void updateThemeAppearance();
    void showStationContextMenu(const QPoint &position);

private slots:
    void searchCountry(const QString &country);
    void onDoubleListClicked(const QModelIndex &index);

private:
    void loadCountriesToComboBox(const QJsonArray &countries);
    void cancelStationSearch();
    void setIndexColor(const QModelIndex &index);

    Ui::MainWindow *ui = nullptr;
    RadioList *radioList = nullptr;

    RadioBrowserApi::RequestId countriesRequest = 0;
    RadioBrowserApi::RequestId stationsRequest = 0;
    bool countriesLoaded = false;

    QPixmap pixmap;
    QSharedPointer<CustomColorDelegate> customColor;

    bool isPlaying = false;
    int currentIndexPlaying = -1;

    RadioStationsModel *countryStationsModel = nullptr;
    RadioStation currentStation;

    PlaybackController &playbackController = SingletonContainer::getSingleton()
                                                 .getInstance<PlaybackController>();

    PlayerUIController &playerUIController = SingletonContainer::getSingleton()
                                                 .getInstance<PlayerUIController>();

    Theme *theme = nullptr;

};

#endif // COUNTRY_H
