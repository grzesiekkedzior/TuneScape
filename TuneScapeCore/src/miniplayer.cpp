#include "include/miniplayer.h"

miniplayer::miniplayer()
{
    mui.setupUi(this);
    connect(mui.stayTop, &QRadioButton::clicked, this, &miniplayer::stayOnTop);
    setStyleSheet(":/src/theme/Combinear.qss");
}

Ui::Frame *miniplayer::getMui()
{
    return &mui;
}

void miniplayer::stayOnTop()
{
    setWindowFlag(Qt::WindowStaysOnTopHint, mui.stayTop->isChecked());
    show();
}
