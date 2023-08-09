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
    headers << STATION << PLAYING << GENRE << HOMEPAGE;
    model->setHorizontalHeaderLabels(headers);

    // Dodaj przykładowe dane
    QStringList stations = {"Station 1", "Station 2", "Station 3"};
    QStringList playing = {"Song A", "Song B", "Song C"};
    QStringList genres = {"Rock", "Pop", "Jazz"};
    QStringList homepages = {"http://www.station1.com", "http://www.station2.com", "http://www.station3.com"};

    for (int row = 0; row < stations.size(); ++row) {
        QList<QStandardItem*> rowItems;
        rowItems.append(new QStandardItem(stations.at(row)));
        rowItems.append(new QStandardItem(playing.at(row)));
        rowItems.append(new QStandardItem(genres.at(row)));
        rowItems.append(new QStandardItem(homepages.at(row)));
        model->appendRow(rowItems);
    }
    ui->tableView->setModel(model);
    header->setSectionResizeMode(QHeaderView::Interactive); // Ustaw interaktywny tryb zmiany rozmiaru kolumn


}
