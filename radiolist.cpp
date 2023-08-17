#include "radiolist.h"
#include <QHeaderView>

RadioList::RadioList(QObject *parent)
    : QObject{parent}
{}

RadioList::RadioList(Ui::MainWindow *ui) : ui(ui), model(new QStandardItemModel(this))
{
    connect(ui->treeView, &QTreeView::clicked, this, &RadioList::onTreeViewItemClicked);
    header = ui->tableView->horizontalHeader();
    headers << STATION << COUNTRY << GENRE << HOMEPAGE;
    header->setSectionResizeMode(QHeaderView::Interactive);
    model->setHorizontalHeaderLabels(headers);
}

void RadioList::loadRadioList()
{
    int rowCount = model->rowCount();
    if (rowCount > 0) {
        model->removeRows(0, rowCount);
    }

    int dataSize = jsonListProcesor.getTableRows().size();
    qDebug() << dataSize;

    for (int row = 0; row < dataSize; ++row) {
        QList<QStandardItem*> rowItems;
        rowItems.append(new QStandardItem(jsonListProcesor.getTableRows().at(row).station));
        rowItems.append(new QStandardItem(jsonListProcesor.getTableRows().at(row).country));
        rowItems.append(new QStandardItem(jsonListProcesor.getTableRows().at(row).genre));
        rowItems.append(new QStandardItem(jsonListProcesor.getTableRows().at(row).stationUrl));
        model->appendRow(rowItems);
    }
    ui->tableView->setModel(model);

    ui->tableView->resizeRowsToContents();

}

auto checkItem = [] (const QString &item, const QString &target) {
    return item == target;
};

void RadioList::onTreeViewItemClicked(const QModelIndex &index)
{
    QString item = index.data().toString();

    if (checkItem(item, "Top")) {
        jsonListProcesor.loadEndpoint(JSON_ENDPOINT_TOP);
    }if (checkItem(item, "Discover")) {
        jsonListProcesor.loadEndpoint(JSON_ENDPOINT_DISCOVER);
    }if (checkItem(item, "Popular")) {
        jsonListProcesor.loadEndpoint(JSON_ENDPOINT_POPULAR);
    } if (checkItem(item, "New")) {
        jsonListProcesor.loadEndpoint(JSON_ENDPOINT_NEW);
    } else {
        this->treeItem = JSON_ENDPOINT_EMPTY;
    }

    jsonListProcesor.processJsonQuery();
    loadRadioList();
}

