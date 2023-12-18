#ifndef ICECASTXMLDATA_H
#define ICECASTXMLDATA_H
#include <QUrl>
#include "../ui_mainwindow.h"

struct IceCastTableRow
{
    QString station = "";
    QString genre = "";
    QString codec = "";
    QString bitrate = "";
    QString sample = "";
};

struct DOM
{
    const QString STATION = "server_name";
    const QString GENRE = "genre";
    const QString CODEC = "server_type";
    const QString BITRATE = "bitrate";
    const QString SAMPLE = "samplerate";
};

class IceCastXmlData : public QObject
{
    Q_OBJECT
public:
    IceCastXmlData(Ui::MainWindow *ui);
    void setUi();
    void loadXmlData();
    void loadXmlToTable();
    void addRowToTable(const IceCastTableRow &row);

private:
    Ui::MainWindow *ui;
    DOM dom;
    const QString iceCastUrl = "https://dir.xiph.org/yp.xml";
    QVector<IceCastTableRow> iceCastTableRows;
};

#endif // ICECASTXMLDATA_H
