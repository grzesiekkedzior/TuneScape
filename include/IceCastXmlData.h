#ifndef ICECASTXMLDATA_H
#define ICECASTXMLDATA_H
#include <QUrl>
#include "../ui_mainwindow.h"
#include "jsonlistprocessor.h"
#include "radioaudiomanager.h"

struct IceCastTableRow
{
    QString station = "";
    QString genre = "";
    QString codec = "";
    QString bitrate = "";
    QString sample = "";
    QString listen_url = "";
};

struct DOM
{
    const QString STATION = "server_name";
    const QString GENRE = "genre";
    const QString CODEC = "server_type";
    const QString BITRATE = "bitrate";
    const QString SAMPLE = "samplerate";
    const QString LISTEN = "listen_url";
};

class IceCastXmlData : public QObject
{
    Q_OBJECT
public:
    IceCastXmlData();
    IceCastXmlData(Ui::MainWindow *ui);
    ~IceCastXmlData();

    void setUi();
    void loadXmlData();
    void loadXmlToTable();
    void addRowToTable(const IceCastTableRow &row);
    void loadXmlAsync();
    void setJsonListProcessor(JsonListProcessor &jsonListProcesor);
    void setRadioAudioManager(RadioAudioManager &radioAudioManager);
    void setRadioList(RadioList *radioList);
    void setTableProperty(Ui::MainWindow *ui);
    void clearTableViewColor();

private slots:
    void onDoubleListClicked(const QModelIndex &index);

private:
    Ui::MainWindow *ui;
    DOM dom;
    RadioAudioManager *radioAudioManager;
    JsonListProcessor *jsonListProcesor;
    RadioList *radioList = nullptr;
    const QString iceCastUrl = "https://dir.xiph.org/yp.xml";
    QVector<IceCastTableRow> iceCastTableRows;

    void setIndexColor(const QModelIndex &index);
};

#endif // ICECASTXMLDATA_H
