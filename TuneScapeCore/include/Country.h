#ifndef COUNTRY_H
#define COUNTRY_H

#include <QDesktopServices>
#include <QMenu>
#include <QObject>
#include <QPixmap>
#include <QPointer>
#include <QString>
#include <QUrl>
#include <QVector>
#include "RadioBrowserApi.h"
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

    void setTheme(Theme *newTheme);
public slots:
    void updateThemeAppearance();
    void showStationContextMenu(const QPoint &position);

private slots:
    void searchCountry(const QString &country);
    void playIndex(const QModelIndex &index);

private:
    void loadCountriesToComboBox(const QJsonArray &countries);
    void cancelStationSearch();

    Ui::MainWindow *ui = nullptr;
    QPointer<RadioList> radioList;
    QPointer<RadioBrowserApi> catalog;

    RadioBrowserApi::RequestId countriesRequest = 0;
    RadioBrowserApi::RequestId stationsRequest = 0;
    bool countriesLoaded = false;

    QPixmap pixmap;
    QPointer<CustomColorDelegate> customColor;

    RadioStationsModel *countryStationsModel = nullptr;

    QPointer<Theme> theme;

};

#endif // COUNTRY_H
