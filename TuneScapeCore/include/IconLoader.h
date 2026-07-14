#ifndef ICONLOADER_H
#define ICONLOADER_H
#include <QLabel>
#include <QNetworkReply>
#include <QObject>
#include <QPushButton>
#include <QVector>

class IconLoader : public QObject
{
    Q_OBJECT
public:
    explicit IconLoader(QObject *parent = nullptr);
    QLabel *createLabel(const QString &text);
    QPushButton *createIconButton(int row);
    QWidget *createIconButtonWithLabel(int row, const QString &stationName);

    void resizeCache(int size);
    void setButton(int row, QWidget *button);
    QWidget *button(int row) const;
    bool containsEmptyButton() const;
    int buttonCount() const;
    void clearCache();
    QWidget *addButton(int row, const QString &stationName);
    void handleNetworkReply(QNetworkReply *reply, int row);

    QVector<QWidget *> getButtonCache() const;

signals:
    void iconClicked(int row);

private:
    const QString TUNESCAPE_ICON = ":/images/img/TuneScape.ico";
    QVector<QWidget *> buttonCache;
};

#endif // ICONLOADER_H
