#include "radiolist.h"
#include <QHeaderView>

RadioList::RadioList(QObject *parent)
    : QObject{parent}
{

}

RadioList::RadioList(Ui::MainWindow *ui) : ui(ui), model(new QStandardItemModel(this))
{
    header = ui->tableView->horizontalHeader();
}

void RadioList::loadList()
{
    headers << STATION << COUNTRY << GENRE << HOMEPAGE;
    header->setSectionResizeMode(QHeaderView::Interactive);
    model->setHorizontalHeaderLabels(headers);

    // Dodaj przykładowe dane
    QStringList stations = {"Station 1", "Station 2", "Station 3"};
    QStringList genres = {"Rock", "Pop", "Jazz"};
    QStringList countries = {"Usa", "Poland", "Germany"};
    QStringList homepages = {"http://www.station1.com", "http://www.station2.com", "http://www.station3.com"};

    for (int row = 0; row < stations.size(); ++row) {
        QList<QStandardItem*> rowItems;
        rowItems.append(new QStandardItem(stations.at(row)));
        rowItems.append(new QStandardItem(countries.at(row)));
        rowItems.append(new QStandardItem(genres.at(row)));
        rowItems.append(new QStandardItem(homepages.at(row)));
        model->appendRow(rowItems);
    }
    ui->tableView->setModel(model);

    ui->tableView->resizeRowsToContents();
}
