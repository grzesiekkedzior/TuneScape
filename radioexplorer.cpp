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
    favorite(new QStandardItem(FAVORITE))
{

}

void RadioExplorer::createMenu()
{
    ui->treeView->setHeaderHidden(true);
    ui->treeView->setModel(itemModel);

    library->setEditable(false);
    favorite->setEditable(false);

    QFont boldFont;
    boldFont.setPointSize(12);
    boldFont.setBold(true);
    library->setData(boldFont, Qt::FontRole);
    favorite->setData(boldFont, Qt::FontRole);

    itemModel->appendRow(library);
    itemModel->appendRow(favorite);

    ui->treeView->setModel(itemModel);
}

