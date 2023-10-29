#ifndef RADIOINFO_H
#define RADIOINFO_H
#include "../ui_mainwindow.h"
#include <QObject>


class RadioInfo : public QObject {
    Q_OBJECT
public:
    RadioInfo();
    RadioInfo(const QWidget *parent = nullptr) {};
    RadioInfo(Ui::MainWindow *ui);

    void setInfo();



private:
    Ui::MainWindow *ui;
};

#endif // RADIOINFO_H
