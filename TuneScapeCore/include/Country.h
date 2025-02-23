#ifndef COUNTRY_H
#define COUNTRY_H

#include "jsonlistprocessor.h"
#include "qnetworkreply.h"
#include "qobject.h"
#include "ui_mainwindow.h"

class RadioList;

class Country : public QObject
{
    struct TableRow
    {
        QString station;
        QString genre;
        QString country;
        QString stationUrl;
    };

    struct CountriesData
    {
        QString countryName;
        QString countryIso;
        QString countryCount;
    };

    Q_OBJECT
public:
    Country();
    ~Country();

    void setData(Ui::MainWindow *ui, RadioList *radioList);
    void load();
    void createHeaders();
    void loadCountriesToComboBox();
    QNetworkReply *setConnection(QString endpoint);
    bool createCountryArray(QNetworkReply *reply);
    void countryNameJsonQuery(QNetworkReply *reply);

    QVector<TableRow> &getTableRows();
    QVector<QString> &getStreamAddresses();

    // const -> JSON
    const QString NAME = "name";
    const QString GENRE = "tags";
    const QString COUNTRY = "country";
    const QString URL = "homepage";
    const QString URL_RESOLVED = "url_resolved";
    const QString URL_STREAM = "url";
    // const -> array
    const QString COUNTRY_NAME = "name";
    const QString COUNTRY_ISO = "iso_3166_1";
    const QString COUNTRY_COUNT = "stationcount";

private:
    Ui::MainWindow *ui = nullptr;
    RadioList *radioList = nullptr;

    QNetworkReply *reply = nullptr;
    QNetworkAccessManager manager;

    JsonListProcessor jsonListProcessor;
    QJsonDocument doc;

    QVector<TableRow> tableRows;
    QVector<CountriesData> countriesData;
    QVector<QString> streamAddresses;

    const QString COUNTRY_ENDPOINT_SEARCH = "json/stations/search?country=";
    const QString COUNTRY_ENDPOINT_NAME = "json/countries";
};

#endif // COUNTRY_H
