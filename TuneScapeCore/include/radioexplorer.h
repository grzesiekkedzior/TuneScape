#ifndef RADIOEXPLORER_H
#define RADIOEXPLORER_H
#include <QObject>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QString>
#include "../ui_mainwindow.h"

class RadioExplorer : public QObject
{
    Q_OBJECT
public:
    RadioExplorer(QWidget *parent = nullptr);
    RadioExplorer();
    RadioExplorer(Ui::MainWindow *ui);

    void createMenu();

private:
    Ui::MainWindow *ui;
    QStandardItemModel *itemModel;
    QStandardItem *library;
    QStandardItem *discover;
    QStandardItem *top;
    QStandardItem *popular;
    QStandardItem *newRadios;
    QStandardItem *favorite;
    QStandardItem *icecast;
    QStandardItem *icecastFavorite;

    const QString LIBRARY = "Radio-Browser";
    const QString ICECAST = "Ice-Cast";
    const QString POPULAR = "Popular";
    const QString TOP = "Top";
    const QString NEW = "New";
    const QString FAVORITE = "Favorite";
    const QString FAVORITE_ICECAST = "Ice-Favorite";
    const QString DISCOVER = "Discover";

    void setNotEditableTree();
    void createTree();
    void setBoldFont();
    void setModelView();
};

#endif // RADIOEXPLORER_H
