#ifndef RADIOINFO_H
#define RADIOINFO_H
#include <QObject>
#include "../ui_mainwindow.h"
#include "include/jsonlistprocessor.h"

// data to set on label
struct Infodata
{
    QString favicon;
    QString station;
    QString country;
    QString state;
    QString language;
    QString votes;
    QString codec;
    QString bitrate;
    QString clickcount;
    QString clicktrend;
};

class RadioInfo : public QObject
{
    Q_OBJECT
public:
    RadioInfo();
    RadioInfo(const QWidget *parent = nullptr){};
    RadioInfo(Ui::MainWindow *ui);
    ~RadioInfo();

    void setInfo();
    void loadEndpoint(const QString station);
    void processInfoJsonQuery();
    void setDataOnTable();

private:
    Ui::MainWindow *ui;
    JsonListProcessor jsonListProcessor;

    QNetworkAccessManager manager;
    QNetworkReply *reply = nullptr;
    QJsonDocument doc;
    Infodata infoData;

    const QString ENDPOINT = "/json/stations/search?name=";

    // json key to get data from an array
    const QString FAVICON = "favicon";
    const QString STATION = "name";
    const QString COUNTRY = "country";
    const QString STATE = "state";
    const QString LANGUAGE = "language";
    const QString VOTES = "votes";
    const QString CODEC = "codec";
    const QString BITRATE = "bitrate";
    const QString CLICKCOUNT = "clickcount";
    const QString CLICKTREND = "clicktrend";
};

#endif // RADIOINFO_H
