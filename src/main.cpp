#include <QApplication>
#include "include/mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.setWindowIcon(QIcon(":/images/img/radio30.png"));
    w.setWindowTitle("TuneScape");
    w.show();
    return a.exec();
}
