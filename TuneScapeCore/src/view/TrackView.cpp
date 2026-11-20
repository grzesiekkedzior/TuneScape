#include "include/view/TrackView.h"
#include "include/music_data/models/Track.h"
#include "ui_mainwindow.h"
#include <utility>

TrackView::TrackView(Ui::MainWindow *ui, QObject *parent, IconLoader *images)
    : QObject(parent), m_cover(ui->cover), m_title(ui->title), m_artist(ui->artist),
      m_album(ui->album), m_year(ui->year), m_links(ui->linksBrowser),
      m_images(images ? images : new IconLoader(this)), m_defaultCover(":/images/img/cover.png")
{
    for (auto *label : {ui->title, ui->artist, ui->album, ui->year}) label->setTextFormat(Qt::PlainText);
    m_cover->setPixmap(m_defaultCover);
    ui->description->hide();
    m_links->setOpenExternalLinks(true);
    connect(m_cover, &QObject::destroyed, this, [this] { cancelCover(); });
}
TrackView::~TrackView() { cancelCover(); }
void TrackView::cancelCover()
{
    ++m_generation;
    const auto request = std::exchange(m_request, 0);
    if (m_images) m_images->cancel(request);
}
void TrackView::setTrack(const music_data::Track &track)
{
    clear();
    if (!track.isValid() || !m_cover || !m_title || !m_artist || !m_album || !m_year || !m_links) return;
    m_title->setText(track.title);
    m_artist->setText(track.artist);
    m_album->setText(track.album);
    m_year->setText(track.releaseDate);
    QString html;
    for (auto it = track.links.cbegin(); it != track.links.cend(); ++it) {
        html += "<b>" + it.key().toHtmlEscaped() + "</b><br>";
        for (const auto &url : it.value()) {
            if (url.scheme() != "https" && url.scheme() != "http") continue;
            html += QString::fromUtf8("\xe2\x80\xa2 <a href=\"") + url.toString(QUrl::FullyEncoded).toHtmlEscaped()
                 + "\">" + url.toDisplayString().toHtmlEscaped() + "</a><br>";
        }
        html += "<br>";
    }
    m_links->setHtml(html);
    if (!m_images || track.coverUrl.isEmpty()) return;
    const auto generation = m_generation;
    m_request = m_images->request(QUrl(track.coverUrl), QSize(250, 250), m_cover->devicePixelRatioF(), this,
        [this, generation](auto id, const QPixmap &image) {
            if (generation != m_generation || id != m_request || !m_cover) return;
            m_request = 0;
            if (!image.isNull()) {
                m_cover->setPixmap(image);
                m_cover->setScaledContents(true);
            }
        });
}
void TrackView::clear()
{
    cancelCover();
    if (m_cover) m_cover->setPixmap(m_defaultCover);
    for (const auto &label : {m_title, m_artist, m_album, m_year}) if (label) label->clear();
    if (m_links) m_links->clear();
}
