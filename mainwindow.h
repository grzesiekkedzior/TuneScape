#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include "radioexplorer.h"
#include "radiolist.h"
#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    RadioExplorer *radioExplorer;
    RadioList     *radioList;
    void set_icon_btn();
    void start();
};
#endif // MAINWINDOW_H
