#ifndef RADIOIMAGEMANAGER_H
#define RADIOIMAGEMANAGER_H
#include "include/miniplayer.h"
#include "ui_mainwindow.h"

class RadioImageManager : public QObject
{
    Q_OBJECT
public:
    explicit RadioImageManager(Ui::MainWindow *ui, miniplayer *mp, QObject *parent = nullptr);

    QPixmap downloadImageSync(const QUrl &url);
    void setImageToUI(const QPixmap &pixmap);

private:
    Ui::MainWindow *m_ui;
    miniplayer *m_miniPlayer;

    const QString RADIO_ICON = ":/images/img/radio96x96.png";
};

#endif // RADIOIMAGEMANAGER_H
