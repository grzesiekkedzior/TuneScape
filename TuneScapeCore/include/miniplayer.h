#ifndef MINIPLAYER_H
#define MINIPLAYER_H

#include <QFrame>
#include "ui_mini_player.h"
class QScreen;
class QCloseEvent;
class QShowEvent;

class miniplayer : public QFrame
{
    Q_OBJECT
public:
    miniplayer();
    Ui::Frame *getMui();

signals:
    void restoreMainWindowRequested();

protected:
    void closeEvent(QCloseEvent *event) override;
    void showEvent(QShowEvent *event) override;

private slots:
    void keepOnScreen();

private:
    void watchScreen(QScreen *screen);
    void stayOnTop();
    Ui::Frame mui;
};

#endif // MINIPLAYER_H
