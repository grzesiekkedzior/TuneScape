#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QIcon>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    set_icon_btn();
    start();
}

MainWindow::~MainWindow()
{
    delete ui;
    if (radioExplorer) {
        delete radioExplorer;
        qDebug() << "radioExplorer";
    }
}

void MainWindow::set_icon_btn()
{
    ui->stop->setIcon(QIcon(":/images/img/stop30.png"));
    ui->previous->setIcon(QIcon(":/images/img/prev30.png"));
    ui->playPause->setIcon(QIcon(":/images/img/play30.png"));
    ui->next->setIcon(QIcon(":/images/img/next30.png"));
    ui->volume->setIcon(QIcon(":/images/img/audioplay.png"));
}

void MainWindow::start()
{
    radioExplorer = new RadioExplorer(ui);
    radioList     = new RadioList(ui);
    radioExplorer->createMenu();
    //radioList->loadDiscoverList();
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);

    if (height() < 140 && ui->splitter->isVisible()) {
        ui->splitter->hide();
        ui->serachInput->hide();

    } else if (height() >= 140 && !ui->splitter->isVisible()) {
        ui->splitter->show();
        ui->serachInput->show();
    }
    qDebug() << ui->centralwidget->geometry().height();


}
