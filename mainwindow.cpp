#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QIcon>

void MainWindow::set_icon_btn()
{
    ui->stop->setIcon(QIcon(":/images/img/stop30.png"));
    ui->previous->setIcon(QIcon(":/images/img/prev30.png"));
    ui->playPause->setIcon(QIcon(":/images/img/play30.png"));
    ui->next->setIcon(QIcon(":/images/img/next30.png"));
    ui->volume->setIcon(QIcon(":/images/img/audioplay.png"));
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    set_icon_btn();
}

MainWindow::~MainWindow()
{
    delete ui;
}

