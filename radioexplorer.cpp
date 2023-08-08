#include "radioexplorer.h"

const QString RadioExplorer::LIBRARY   = "Library";
const QString RadioExplorer::POPULAR   = "Popular";
const QString RadioExplorer::DISCOVER  = "Discover";
const QString RadioExplorer::TOP       = "Top";
const QString RadioExplorer::NEW       = "New";
const QString RadioExplorer::FAVORITE  = "Favorite";

RadioExplorer::RadioExplorer(Ui::MainWindow *ui) : ui(ui),
    itemModel(new QStandardItemModel),
    library(new QStandardItem(LIBRARY)),
    discover(new QStandardItem(DISCOVER)),
    top(new QStandardItem(TOP)),
    popular(new QStandardItem(POPULAR)),
    newRadios(new QStandardItem(NEW)),
    favorite(new QStandardItem(FAVORITE))
{

}

void RadioExplorer::setNotEditableTree()
{
    library->setEditable(false);
    discover->setEditable(false);
    top->setEditable(false);
    popular->setEditable(false);
    newRadios->setEditable(false);
    favorite->setEditable(false);
}

void RadioExplorer::createTree()
{
    itemModel->appendRow(library);
    itemModel->appendRow(favorite);
    library->appendRow(discover);
    library->appendRow(top);
    library->appendRow(popular);
    library->appendRow(newRadios);
}

void RadioExplorer::setBoldFont()
{
    QFont boldFont;
    boldFont.setBold(true);
    library->setData(boldFont, Qt::FontRole);
    favorite->setData(boldFont, Qt::FontRole);
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

