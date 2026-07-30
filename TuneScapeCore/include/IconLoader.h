#ifndef ICONLOADER_H
#define ICONLOADER_H

#include <QHash>
#include <QIcon>
#include <QNetworkAccessManager>
#include <QObject>
#include <QUrl>

class IconLoader : public QObject
{
    Q_OBJECT

public:
    explicit IconLoader(QObject *parent = nullptr);

    void loadIcon(int row, const QUrl &url, const QSize &size);

signals:
    void iconLoaded(int row, const QIcon &icon);
    void iconLoadFailed(int row);

private:
    QNetworkAccessManager m_networkManager;
    QHash<QString, QIcon> m_cache;
};
#endif // ICONLOADER_H