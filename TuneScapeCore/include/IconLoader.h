#ifndef ICONLOADER_H
#define ICONLOADER_H
#include <QLabel>
#include <QObject>
#include <QPushButton>

class IconLoader : public QObject
{
    Q_OBJECT
public:
    explicit IconLoader(QObject *parent = nullptr);
    QLabel *createLabel(const QString &text);
    QPushButton *createIconButton(int row);
    QWidget *createIconButtonWithLabel(int row, const QString &stationName);

signals:
    void iconClicked(int row);

private:
    const QString TUNESCAPE_ICON = ":/images/img/TuneScape.ico";
};

#endif // ICONLOADER_H
