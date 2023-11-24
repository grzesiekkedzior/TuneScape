#ifndef JSONLISTPROCESSOR_H
#define JSONLISTPROCESSOR_H

#include "../ui_mainwindow.h"
#include "include/radiostations.h"

#include <QEventLoop>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QTimer>

class RadioList;

struct TableRow
{
    QString station;
    QString genre;
    QString country;
    QString stationUrl;
};

class JsonListProcessor : public QObject
{
    Q_OBJECT
public:
    JsonListProcessor();
    ~JsonListProcessor();

    bool isConnected = false;

    void processJsonQuery();
    void loadEndpoint(QString endpoint);
    void setUi(Ui::MainWindow *ui);
    void setRadioList(RadioList *radioList);
    bool checkInternetConnection();
    void lostConnection();
    void connected();
    QVector<TableRow> &getTableRows();
    QVector<QString> &getStreamAddresses();
    QVector<QString> &getIconAddresses();
    QString getStreamAddresses(int index) const;
    QString getIconAddresses(int index) const;

    void setTableRows(const QVector<TableRow> &rows);
    void setStreamAddresses(const QVector<QString> &addresses);
    void setIconAddresses(const QVector<QString> &icons);
    QNetworkReply *checkAvailability(const QStringList &radioAddresses);
    QJsonDocument createJasonDocument(QNetworkReply *reply);
signals:
    void connectionLost();

private:
    const QString NAME = "name";
    const QString GENRE = "tags";
    const QString COUNTRY = "country";
    const QString URL = "homepage";
    const QString URL_RESOLVED = "url_resolved";
    const QString FAVICON = "favicon";
    QString endpoint = "";

    QVector<TableRow> tableRows;
    QVector<QString> streamAddresses;
    QVector<QString> iconAddresses;

    QNetworkAccessManager manager;
    QNetworkReply *reply = nullptr;
    QJsonDocument doc;
    RadioStations severAddress;
    Ui::MainWindow *ui;
    QTimer *connectionTimer;
    QTimer *internetConnectionChecker;
    RadioList *radioList = nullptr;

    void retryInternetConnection();
    void setConnection(QNetworkReply *connectionReply);
};

#endif // JSONLISTPROCESSOR_H
