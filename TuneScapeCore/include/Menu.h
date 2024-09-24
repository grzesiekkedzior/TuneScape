#ifndef MENU_H
#define MENU_H
#include <QObject>
#include <ui_mainwindow.h>

class Menu : public QObject
{
    Q_OBJECT
public:
    void setUi(Ui::MainWindow *ui);

private:
    Ui::MainWindow *ui = nullptr;
};

#endif // MENU_H
