#ifndef TRACKVIEW_H
#define TRACKVIEW_H
#include "include/IconLoader.h"
#include <QLabel>
#include <QTextBrowser>
namespace Ui { class MainWindow; }
namespace music_data { struct Track; }
class TrackView : public QObject
{
    Q_OBJECT
public:
    explicit TrackView(Ui::MainWindow *ui, QObject *parent = nullptr, IconLoader *images = nullptr);
    ~TrackView() override;
public slots:
    void setTrack(const music_data::Track &track);
    void clear();
private:
    void cancelCover();
    QPointer<QLabel> m_cover, m_title, m_artist, m_album, m_year;
    QPointer<QTextBrowser> m_links;
    QPointer<IconLoader> m_images;
    IconLoader::RequestId m_request = 0;
    quint64 m_generation = 0;
    QPixmap m_defaultCover;
};
#endif
