#ifndef RADIOLIST_H
#define RADIOLIST_H

#include "ui_mainwindow.h"
#include "jsonlistprocessor.h"
#include <QObject>
#include <QStandardItemModel>
#include <QHeaderView>
#include <QStringList>

class RadioList : public QObject
{
    Q_OBJECT
public:
    explicit RadioList(QObject *parent = nullptr);
    RadioList(Ui::MainWindow *ui);

    void loadList();

signals:


private:
    Ui::MainWindow     *ui      = nullptr;
    QStandardItemModel *model   = nullptr;
    QHeaderView        *header  = nullptr;
    QStringList         headers;
    JsonListProcessor   jsonListProcesor;

    const QString STATION   = "Station";
    const QString GENRE     = "Genre";
    const QString COUNTRY   = "Country";
    const QString HOMEPAGE  = "Homepage";

};

#endif // RADIOLIST_H
