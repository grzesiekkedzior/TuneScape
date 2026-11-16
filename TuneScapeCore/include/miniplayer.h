#ifndef MINIPLAYER_H
#define MINIPLAYER_H

#include <QFrame>
#include "ui_mini_player.h"

class miniplayer : public QFrame
{
    Q_OBJECT
public:
    miniplayer();
    Ui::Frame *getMui();

private:
    void stayOnTop();
    Ui::Frame mui;
};

#endif // MINIPLAYER_H
