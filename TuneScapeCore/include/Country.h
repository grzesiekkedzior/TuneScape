#ifndef COUNTRY_H
#define COUNTRY_H

#include <QObject>
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
class QComboBox;
class QTableView;
class QLabel;
class QTabWidget;
class QWidget;

class Country : public QObject
{
    Q_OBJECT

public:
    Country() = default;

    ~Country() override;

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
    void detach();

    QPointer<QComboBox> selector;
    QPointer<QTableView> stationView;
    QPointer<QLabel> worldImage;
    QPointer<QTabWidget> tabs;
    QPointer<QWidget> countryTab;
    QVector<QMetaObject::Connection> connections;
    QPointer<RadioList> radioList;
    QPointer<RadioBrowserApi> catalog;

    RadioBrowserApi::RequestId countriesRequest = 0;
    RadioBrowserApi::RequestId stationsRequest = 0;
    bool countriesLoaded = false;

    QPointer<CustomColorDelegate> customColor;

    RadioStationsModel *countryStationsModel = nullptr;

    QPointer<Theme> theme;

};

#endif // COUNTRY_H
