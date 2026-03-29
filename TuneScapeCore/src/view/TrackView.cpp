#include "include/view/TrackView.h"
#include <QNetworkReply>
#include <QUrl>
#include "include/music_data/models/Track.h"
#include "ui_mainwindow.h"

TrackView::TrackView(Ui::MainWindow *ui, QObject *parent)
    : QObject(parent)
    , ui(ui)
{
    networkManager = new QNetworkAccessManager(this);
    defaulrCoverAlbum = QPixmap(":/images/img/cover.png");
    ui->cover->setPixmap(defaulrCoverAlbum);
    ui->description->hide();
    ui->linksBrowser->setOpenExternalLinks(true);
}

void TrackView::setTrack(const music_data::Track &track)
{
    if (!track.isValid())
        return;

    loadCover(track.coverUrl);

    ui->title->setText(track.title);
    ui->artist->setText(track.artist);
    ui->album->setText(track.album);
    ui->year->setText(track.releaseDate);

    ui->linksBrowser->setHtml(generateLinksHtml(track.links));
}

void TrackView::loadCover(const QString &url)
{
    ui->cover->setPixmap(defaulrCoverAlbum);

    if (url.isEmpty())
        return;

    QNetworkRequest request((QUrl(url)));
    QNetworkReply *reply = networkManager->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() != QNetworkReply::NoError) {
            reply->deleteLater();
            return;
        }

        QPixmap pixmap;
        if (pixmap.loadFromData(reply->readAll())) {
            ui->cover->setPixmap(pixmap);
            ui->cover->setScaledContents(true);
        }

        reply->deleteLater();
    });
}

QString TrackView::generateLinksHtml(const QMap<QString, QList<QUrl>> &links)
{
    QString html;

    for (auto it = links.begin(); it != links.end(); ++it) {
        html += "<b>" + it.key() + "</b><br>";

        for (const QUrl &url : it.value()) {
            html += "• <a href='" + url.toString() + "'>" + url.toString() + "</a><br>";
        }

        html += "<br>";
    }

    return html;
}

void TrackView::clear()
{
    ui->cover->clear();
    ui->title->clear();
    ui->artist->clear();
    ui->album->clear();
    ui->year->clear();
    ui->linksBrowser->clear();
    ui->cover->setPixmap(defaulrCoverAlbum);
}