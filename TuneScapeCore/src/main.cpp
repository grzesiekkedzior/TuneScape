#include <QApplication>
#include "include/mainwindow.h"
#include "include/theme.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Theme theme(a);
    MainWindow w{theme};
    w.setWindowIcon(QIcon(":/images/img/radio96x96.png"));
    w.setWindowTitle("TuneScape");
    w.show();
    return a.exec();
}
