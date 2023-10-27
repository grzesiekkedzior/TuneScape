#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include "include/radioexplorer.h"
#include "include/radiolist.h"
#include <QMainWindow>
#include <QResizeEvent>
#include <QDialog>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
private slots:
    void info();

private:
    Ui::MainWindow *ui;
    RadioExplorer *radioExplorer;
    RadioList     *radioList;
    QDialog *licenseDialog;
    QLabel *licenseLabel;
    QLabel *aboutTuneScape;
    QScrollArea *scrollArea;
    void set_icon_btn();
    void start();
    void resizeEvent(QResizeEvent *event);

    void infoDialogApp();
};
#endif // MAINWINDOW_H
