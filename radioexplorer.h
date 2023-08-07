#ifndef RADIOEXPLORER_H
#define RADIOEXPLORER_H
#include "ui_mainwindow.h"
#include <QStandardItemModel>
#include <QStandardItem>
#include <QString>

class RadioExplorer
{
public:
    RadioExplorer();
    RadioExplorer(Ui::MainWindow *ui);

    void createMenu();
private:
    Ui::MainWindow *ui;
    QStandardItemModel* itemModel;
    QStandardItem*      library;
    QStandardItem*      favorite;
    QStandardItem*      test;

    static const QString LIBRARY;
    static const QString POPULAR;
    static const QString DISCOVER;
    static const QString TOP;
    static const QString NEW;
    static const QString FAVORITE;
};

#endif // RADIOEXPLORER_H
