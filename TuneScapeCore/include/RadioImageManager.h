#ifndef RADIOIMAGEMANAGER_H
#define RADIOIMAGEMANAGER_H
#include <QObject>
#include <QPixmap>
#include <QUrl>

class RadioImageManager : public QObject
{
    Q_OBJECT
public:
    explicit RadioImageManager(QObject *parent = nullptr);

    QPixmap downloadImageSync(const QUrl &url);
    QPixmap prepareImage(const QPixmap &pixmap) const;
};

#endif // RADIOIMAGEMANAGER_H
