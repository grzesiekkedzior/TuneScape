#include "include/controller/PlayerUIController.h"
#include <QIcon>
#include "include/miniplayer.h"
#include "ui_mainwindow.h"

namespace {
constexpr auto RadioIconPath = ":/images/img/radio96x96.png";
constexpr auto TuneScapeIconPath = ":/images/img/TuneScape.ico";
constexpr auto PlayIconPath = ":/images/img/play30.png";
constexpr auto PauseIconPath = ":/images/img/pause30.png";
constexpr auto FavoriteIconPath = ":/images/img/bookmark-file.png";
constexpr auto EmptyFavoriteIconPath = ":/images/img/bookmark-empty.png";
} // namespace

void PlayerUIController::setPlayIcon()
{
    Q_ASSERT(m_ui);
    Q_ASSERT(m_miniPlayer);

    const QIcon icon(PlayIconPath);

    m_ui->playPause->setIcon(icon);
    m_miniPlayer->getMui()->play->setIcon(icon);
}

void PlayerUIController::setPauseIcon()
{
    Q_ASSERT(m_ui);
    Q_ASSERT(m_miniPlayer);

    const QIcon icon(PauseIconPath);

    m_ui->playPause->setIcon(icon);
    m_miniPlayer->getMui()->play->setIcon(icon);
}

void PlayerUIController::setDefaultImage()
{
    const QPixmap pixmap(RadioIconPath);

    m_ui->infoLabel->setPixmap(pixmap);
    m_ui->radioIcon->setPixmap(pixmap);
    m_miniPlayer->getMui()->radioImage->setPixmap(pixmap);
}

void PlayerUIController::setStationImage(const QPixmap &pixmap)
{
    Q_ASSERT(m_ui);
    Q_ASSERT(m_miniPlayer);

    m_ui->infoLabel->setPixmap(pixmap);
    m_ui->radioIcon->setPixmap(pixmap);
    m_miniPlayer->getMui()->radioImage->setPixmap(pixmap);

    m_ui->infoLabel->show();
}

void PlayerUIController::setMetadata(const QString &metadata)
{
    Q_ASSERT(m_ui);
    Q_ASSERT(m_miniPlayer);

    m_ui->infoData->setText(metadata);
    m_miniPlayer->getMui()->radioText->setText(metadata);
}

void PlayerUIController::clearMetadata()
{
    Q_ASSERT(m_ui);
    Q_ASSERT(m_miniPlayer);

    m_ui->infoData->clear();
    m_miniPlayer->getMui()->radioText->clear();
}

void PlayerUIController::setFavorite(bool isFavorite)
{
    Q_ASSERT(m_ui);

    const QIcon icon(isFavorite ? FavoriteIconPath : EmptyFavoriteIconPath);
    m_ui->favorite->setIcon(icon);
}

void PlayerUIController::showDefaultImage() {}

void PlayerUIController::setVolumeIcon(int volume)
{
    Q_ASSERT(m_ui);

    const QIcon icon(volume == 0 ? ":/images/img/audiostop.png" : ":/images/img/audioplay.png");

    m_ui->volume->setIcon(icon);
}

void PlayerUIController::initialize(Ui::MainWindow *ui, miniplayer *miniPlayer)
{
    Q_ASSERT(ui);
    Q_ASSERT(miniPlayer);

    m_ui = ui;
    m_miniPlayer = miniPlayer;
}
