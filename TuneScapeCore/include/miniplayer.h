#ifndef MINIPLAYER_H
#define MINIPLAYER_H

#include <QWidget>
#include "ui_mainwindow.h"
#include "ui_mini_player.h"

class RadioList;

class miniplayer : public QFrame
{
    Q_OBJECT
public:
    miniplayer();
    miniplayer(Ui::MainWindow *ui, RadioList *radioList);

    bool getIsVisible() const;
    void setIsVisible(bool newIsVisible);

    Ui::MainWindow *getUi() const;
    void setUi(Ui::MainWindow *newUi);

    RadioList *getRadioList() const;
    void setRadioList(RadioList *newRadioList);
    void showMiniPlayer();

    Ui::Frame *getMui() const;
    void setMui(Ui::Frame *newMui);

private:
    Ui::MainWindow *ui = nullptr;
    Ui::Frame *mui = nullptr;
    RadioList *radioList = nullptr;
    bool isVisible = true;

    void playRadio();
    void maximizeWindow();
};

#endif // MINIPLAYER_H
