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
    void importM3Ufile(const QString path, const QString playlist);
    bool isM3Ufile(const QString& path) const;
    bool isRadioInPlaylist(const QString &stationName, const QString &playlist);
    bool saveOnTuneScapeFile(const QString &stationName, const QString &streamAddress, const QString &playlist);

    //Export pls
    bool exportM3Ufile();
    bool convertToM3Ufile(const QString &playlist);

    //Common
    static bool fileExists(const QString path);

    const QString ICECAST_PLAYLIST = "icecast.txt";
    const QString RADIO_BROWSER_PLAYLIST = "radiobrowser.txt";
    const QString RADIO_BROWSER_M3U = "radiobrowser.m3u";


};

#endif // M3UHANDLER_H
