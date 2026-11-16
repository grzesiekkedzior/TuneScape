#include "include/radioinfo.h"
#include "ui_mainwindow.h"

RadioInfo::RadioInfo(Ui::MainWindow *ui)
    : ui(ui)
{
    auto *table = ui->tableWidget;
    table->horizontalHeader()->hide();
    table->verticalHeader()->hide();
    table->setShowGrid(false);
    table->setAlternatingRowColors(true);
    table->setSelectionMode(QAbstractItemView::NoSelection);
    table->setFocusPolicy(Qt::NoFocus);
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
}

void RadioInfo::showStation(const RadioStation &station, const QString &clickTrend)
{
    setFields({station.station, station.country, station.state, station.language,
               QString::number(station.votes), station.codec, QString::number(station.bitrate),
               QString::number(station.clickCount), clickTrend});
}

void RadioInfo::clearInfo()
{
    setFields({});
}

void RadioInfo::setFields(const QStringList &fields)
{
    for (int row = 0; row < ui->tableWidget->rowCount(); ++row)
        ui->tableWidget->setItem(row, 1, new QTableWidgetItem(fields.value(row)));
}
