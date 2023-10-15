#include "include/mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.setWindowIcon(QIcon(":/images/img/radio30.png"));
    w.setWindowTitle("AudioLink");
    w.show();
    return a.exec();
}
