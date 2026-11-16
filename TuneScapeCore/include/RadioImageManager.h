#ifndef RADIOIMAGEMANAGER_H
#define RADIOIMAGEMANAGER_H

#include <QNetworkAccessManager>
#include <QObject>
#include <QPixmap>
#include <QPointer>
#include <QUrl>

class QNetworkReply;

class RadioImageManager : public QObject
{
    Q_OBJECT
public:
    explicit RadioImageManager(QObject *parent = nullptr);
    ~RadioImageManager() override;

    void load(const QUrl &url);
    void cancel();

signals:
    void imageLoaded(const QPixmap &image);

private:
    QNetworkAccessManager m_network;
    QPointer<QNetworkReply> m_reply;
};

#endif // RADIOIMAGEMANAGER_H
