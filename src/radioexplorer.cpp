#include "include/radioexplorer.h"

RadioExplorer::RadioExplorer(QWidget *parent) {}

RadioExplorer::RadioExplorer(Ui::MainWindow *ui)
    : ui(ui)
{
    itemModel = new QStandardItemModel;
    library = new QStandardItem(LIBRARY);
    top = new QStandardItem(QIcon(":/images/img/top.png"), TOP);
    popular = new QStandardItem(QIcon(":/images/img/popular.png"), POPULAR);
    newRadios = new QStandardItem(QIcon(":/images/img/new.png"), NEW);
    favorite = new QStandardItem(QIcon(":/images/img/favourite.png"), FAVORITE);
    icecast = new QStandardItem(ICECAST);
    discover = new QStandardItem(QIcon(":/images/img/icecast.png"), DISCOVER);
    icecastFavorite = new QStandardItem(QIcon(":/images/img/favourite.png"), FAVORITE_ICECAST);
}

void RadioExplorer::setNotEditableTree()
{
    library->setEditable(false);
    top->setEditable(false);
    popular->setEditable(false);
    newRadios->setEditable(false);
    favorite->setEditable(false);
    icecast->setEditable(false);
    discover->setEditable(false);
    icecastFavorite->setEditable(false);
}

void RadioExplorer::createTree()
{
    itemModel->appendRow(library);
    library->appendRow(top);
    library->appendRow(popular);
    library->appendRow(newRadios);
    library->appendRow(favorite);
    itemModel->appendRow(icecast);
    icecast->appendRow(discover);
    icecast->appendRow(icecastFavorite);
}

void RadioExplorer::setBoldFont()
{
    QFont boldFont;
    boldFont.setBold(true);
    library->setData(boldFont, Qt::FontRole);
    icecast->setData(boldFont, Qt::FontRole);
}

void RadioExplorer::setModelView()
{
    ui->treeView->setModel(itemModel);
    ui->treeView->expandAll();
}

void RadioExplorer::createMenu()
{
    setNotEditableTree();
    setBoldFont();
    createTree();
    setModelView();
}
