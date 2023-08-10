#ifndef JSONLISTPROCESSOR_H
#define JSONLISTPROCESSOR_H

#include <QObject>

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
    explicit JsonListProcessor(QObject *parent = nullptr);



    QVector<TableRow>& getTableRows();

signals:


private:
    QVector<TableRow> tableRows;

};

#endif // JSONLISTPROCESSOR_H
