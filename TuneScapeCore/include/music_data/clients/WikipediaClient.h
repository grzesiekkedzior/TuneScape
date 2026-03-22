#ifndef WIKIPEDIACLIENT_H
#define WIKIPEDIACLIENT_H
#include <QNetworkAccessManager>
#include <QObject>

namespace music_data {
class WikipediaClient : public QObject
{
    Q_OBJECT
public:
private:
    QNetworkAccessManager *m_man;
};
} // namespace music_data

#endif // WIKIPEDIACLIENT_H
