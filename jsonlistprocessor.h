#ifndef JSONLISTPROCESSOR_H
#define JSONLISTPROCESSOR_H

#include "radiostations.h"
#include "ui_mainwindow.h"

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QEventLoop>
#include <QNetworkReply>
#include <QJsonDocument>

struct TableRow {
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

    void processJsonQuery();
    QVector<TableRow>& getTableRows();
    void loadEndpoint(QString endpoint);
    void setUi(Ui::MainWindow *ui);

signals:
private slots:
    void checkConnection();

private:
    const QString NAME      = "name";
    const QString GENRE     = "tags";
    const QString COUNTRY   = "country";
    const QString URL       = "homepage";
    QString endpoint        = "";
    QVector<TableRow>       tableRows;
    QNetworkAccessManager   manager;
    QNetworkReply           *reply;
    QJsonDocument           doc;
    RadioStations           severAddress;
    Ui::MainWindow          *ui;
    QTimer                  *connectionTimer;

    QNetworkReply* checkAvailability(const QStringList &radioAddresses);
    QJsonDocument createJasonDocument(QNetworkReply *reply);
    void setConnection(QNetworkReply *connectionReply);
};

#endif // JSONLISTPROCESSOR_H
