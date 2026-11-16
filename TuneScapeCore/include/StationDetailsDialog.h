#ifndef STATIONDETAILSDIALOG_H
#define STATIONDETAILSDIALOG_H
#include <QDialog>

#include "include/data/RadioStation.h"

class QLabel;
class QPushButton;
class IconLoader;

class StationDetailsDialog : public QDialog
{
    Q_OBJECT

public:
    StationDetailsDialog(const RadioStation &station, IconLoader &loader, QWidget *parent = nullptr);

private:
    QLabel *createValueLabel(const QString &text) const;
    void openHomepage();
    void copyStreamUrl();

    RadioStation m_station;

    QLabel *m_iconLabel = nullptr;
    QPushButton *m_openHomepageButton = nullptr;
};
#endif // STATIONDETAILSDIALOG_H
