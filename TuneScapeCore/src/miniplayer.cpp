#include "include/miniplayer.h"
#include "../ui_mini_player.h"
#include "include/radiolist.h"

miniplayer::miniplayer()
    : mui(new Ui::Frame)
{
    mui->setupUi(this);
    connect(mui->play, &QPushButton::clicked, this, &miniplayer::playRadio);
    qDebug() << "miniplayer";
}

miniplayer::miniplayer(Ui::MainWindow *ui, RadioList *radioList)
    : ui(ui)
    , mui(new Ui::Frame)
    , radioList(radioList)
{
    mui->setupUi(this);
}

bool miniplayer::getIsVisible() const
{
    return isVisible;
}

void miniplayer::setIsVisible(bool newIsVisible)
{
    isVisible = newIsVisible;
}

Ui::MainWindow *miniplayer::getUi() const
{
    return ui;
}

void miniplayer::setUi(Ui::MainWindow *newUi)
{
    ui = newUi;
}

RadioList *miniplayer::getRadioList() const
{
    return radioList;
}

void miniplayer::setRadioList(RadioList *newRadioList)
{
    radioList = newRadioList;
}

void miniplayer::showMiniPlayer()
{
    bool state = getIsVisible();
    (state) ? this->show() : this->hide();
}

Ui::Frame *miniplayer::getMui() const
{
    return mui;
}

void miniplayer::setMui(Ui::Frame *newMui)
{
    mui = newMui;
}

void miniplayer::playRadio()
{
    radioList->onPlayPauseButtonCliced();
}

void miniplayer::maximizeWindow() {}
