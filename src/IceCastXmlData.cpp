#include "include/IceCastXmlData.h"
#include <QFuture>
#include <QFutureWatcher>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QXmlStreamReader>
#include <QtConcurrent>

IceCastXmlData::IceCastXmlData(Ui::MainWindow *ui)
{
    this->ui = ui;
    ui->icecastTable->verticalHeader()->setDefaultSectionSize(18);
    ui->icecastTable->setColumnCount(5);
    ui->icecastTable->setHorizontalHeaderLabels({"Station", "Genre", "Codec", "Bitrate", "Sample"});
    loadXmlAsync();
}

void IceCastXmlData::loadXmlData()
{
    QUrl url(iceCastUrl);
    QNetworkAccessManager manager;
    QNetworkRequest request(url);
    QNetworkReply *reply = manager.get(request);

    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error()) {
        reply->deleteLater();
        return;
    }

    QXmlStreamReader xml(reply->readAll());

    while (!xml.atEnd()) {
        IceCastTableRow icast;
        xml.readNext();
        if (xml.name().toString() == "entry") {
            while (!(xml.isEndElement() && xml.name().toString() == "entry")) {
                xml.readNext();

                if (xml.isStartElement()) {
                    QString elementName = xml.name().toString();

                    if (elementName == "server_name") {
                        icast.station = xml.readElementText();
                    } else if (elementName == "genre") {
                        icast.genre = xml.readElementText();
                    } else if (elementName == "server_type") {
                        icast.codec = xml.readElementText();
                    } else if (elementName == "bitrate") {
                        icast.bitrate = xml.readElementText();
                    } else if (elementName == "samplerate") {
                        icast.sample = xml.readElementText();
                    } else if (elementName == "listen_url") {
                        icast.listen_url = xml.readElementText();
                    }
                }
            }
            iceCastTableRows.push_back(icast);
        }
    }

    reply->deleteLater();
}

void IceCastXmlData::loadXmlToTable()
{
    for (const auto &row : iceCastTableRows) {
        addRowToTable(row);
    }
}

void IceCastXmlData::addRowToTable(const IceCastTableRow &row)
{
    int rowPosition = ui->icecastTable->rowCount();
    ui->icecastTable->insertRow(rowPosition);

    ui->icecastTable->setItem(rowPosition, 0, new QTableWidgetItem(row.station));
    ui->icecastTable->setItem(rowPosition, 1, new QTableWidgetItem(row.genre));
    ui->icecastTable->setItem(rowPosition, 2, new QTableWidgetItem(row.codec));
    ui->icecastTable->setItem(rowPosition, 3, new QTableWidgetItem(row.bitrate));
    ui->icecastTable->setItem(rowPosition, 4, new QTableWidgetItem(row.sample));
}

void IceCastXmlData::loadXmlAsync()
{
    QFutureWatcher<void> *watcher = new QFutureWatcher<void>(this);

    connect(watcher, &QFutureWatcher<void>::finished, [=]() {
        watcher->deleteLater();
        loadXmlToTable();
    });

    QFuture<void> future = QtConcurrent::run([this]() { loadXmlData(); });

    watcher->setFuture(future);
}
