#include "include/mainwindow.h"
#include "include/theme.h"
#include <QApplication>

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    MainWindow w;
    Theme theme;
    theme.setUi(w.getUi());
    theme.getApplicationObject(a);
    w.setWindowIcon(QIcon(":/images/img/radio30.png"));
    w.setWindowTitle("TuneScape");
    w.show();
    return a.exec();
}
