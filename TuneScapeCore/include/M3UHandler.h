#ifndef M3UHANDLER_H
#define M3UHANDLER_H
#include <QObject>
#include <QList>
#include <QString>

class M3UHandler : public QObject {
    Q_OBJECT
public:
    explicit M3UHandler(QObject *parent = nullptr);

    struct TableRow
    {
        QString station;
        QString genre;
        QString country;
        QString stationUrl;
    };

private:
    //Load pls
    QList<TableRow> importM3Ufile(const QString path);
    bool isM3Ufile();
    bool isRadioInPlaylist(const QString data, const QString playlist);
    bool saveOnTuneScapeFile();

    //Export pls
    bool exportM3Ufile();
    bool convertToM3Ufile();

    //Common
    static bool fileExists(const QString path);


};

#endif // M3UHANDLER_H
