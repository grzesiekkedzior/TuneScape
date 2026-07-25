#ifndef PLAYERUICONTROLLER_H
#define PLAYERUICONTROLLER_H
#include <QObject>
#include <QPixmap>
class miniplayer;
namespace Ui {
class MainWindow;
}

class PlayerUIController : public QObject
{
    Q_OBJECT
public:
    explicit PlayerUIController() = default;
    void setPlayIcon();
    void setPauseIcon();

    void setDefaultImage();
    void setStationImage(const QPixmap &);

    void setMetadata(const QString &);
    void clearMetadata();

    void setFavorite(bool isFavorite);
    void showDefaultImage();

    void setVolumeIcon(int volume);

    void initialize(Ui::MainWindow *ui, miniplayer *miniPlayer);

private:
    Ui::MainWindow *m_ui = nullptr;
    miniplayer *m_miniPlayer = nullptr;
};

#endif // PLAYERUICONTROLLER_H
