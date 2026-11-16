#ifndef RADIOINFO_H
#define RADIOINFO_H

#include "data/RadioStation.h"
#include <QStringList>

namespace Ui {
class MainWindow;
}

class RadioInfo
{
public:
    explicit RadioInfo(Ui::MainWindow *ui);

    void showStation(const RadioStation &station, const QString &clickTrend = {});
    void clearInfo();

private:
    void setFields(const QStringList &fields);

    Ui::MainWindow *ui;
};

#endif // RADIOINFO_H
