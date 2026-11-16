#ifndef ICONLOADER_H
#define ICONLOADER_H

#include <QHash>
#include <QIcon>
#include <QNetworkAccessManager>
#include <QObject>
#include <QPointer>
#include <QUrl>
#include <QVector>

class QNetworkReply;

class IconLoader : public QObject
{
    Q_OBJECT

public:
    explicit IconLoader(QObject *parent = nullptr);
    ~IconLoader() override;

    void loadIcon(int row, const QUrl &url, const QSize &size);
    void cancelPending();

signals:
    void iconLoaded(int row, const QIcon &icon);
    void iconLoadFailed(int row);

private:
    struct Pending {
        QPointer<QNetworkReply> reply;
        QVector<int> rows;
    };

    QNetworkAccessManager m_networkManager;
    QHash<QString, QIcon> m_cache;
    QHash<QString, Pending> m_pending;
    quint64 m_generation = 0;
};
#endif // ICONLOADER_H
