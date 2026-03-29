#ifndef TRACKVIEW_H
#define TRACKVIEW_H
#include <QMap>
#include <QNetworkAccessManager>
#include <QUrl>
#include <QWidget>

namespace Ui {
class MainWindow;
}

namespace music_data {
struct Track;
}

class TrackView : public QObject
{
    Q_OBJECT

public:
    explicit TrackView(Ui::MainWindow *ui, QObject *parent = nullptr);

public slots:
    void setTrack(const music_data::Track &track);
    void clear();

private:
    void loadCover(const QString &url);
    QString generateLinksHtml(const QMap<QString, QList<QUrl>> &links);

private:
    Ui::MainWindow *ui;
    QNetworkAccessManager *networkManager;
    QPixmap defaulrCoverAlbum;
};
#endif // TRACKVIEW_H
