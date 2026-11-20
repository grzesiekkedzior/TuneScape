#include "include/music_data/clients/CoverArtClient.h"
using namespace music_data;
CoverArtClient::CoverArtClient(QObject *parent, QUrl base, int timeoutMs)
    : QObject(parent), m_base(base), m_requests(0, timeoutMs, this) {}
void CoverArtClient::getCover(const QString &mbid, quint64 token)
{
    m_requests.request(m_base.resolved(QUrl(QString::fromLatin1(QUrl::toPercentEncoding(mbid)))), this, token,
        [this](auto id, const auto &result) { emit coverFinished(id, result.data); });
}
