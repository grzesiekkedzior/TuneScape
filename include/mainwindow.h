#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include "include/radioexplorer.h"
#include "include/radiolist.h"
#include <QMainWindow>
#include <QResizeEvent>

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
    void resizeEvent(QResizeEvent *event);

};
#endif // MAINWINDOW_H
