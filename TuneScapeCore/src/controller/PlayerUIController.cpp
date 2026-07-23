#include "include/controller/PlayerUIController.h"

void PlayerUIController::setPlayIcon() {}

void PlayerUIController::setPauseIcon() {}

void PlayerUIController::setDefaultImage() {}

void PlayerUIController::setStationImage(const QPixmap &) {}

void PlayerUIController::setMetadata(const QString &) {}

void PlayerUIController::clearMetadata() {}

void PlayerUIController::setFavorite(bool) {}

void PlayerUIController::showDefaultImage() {}

void PlayerUIController::initialize(Ui::MainWindow *ui, miniplayer *miniPlayer)
{
    Q_ASSERT(ui);
    Q_ASSERT(miniPlayer);

    m_ui = ui;
    m_miniPlayer = miniPlayer;
}
