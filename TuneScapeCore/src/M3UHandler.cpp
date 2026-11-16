#include "include/M3UHandler.h"

#include <QObject>
#include <QRegularExpression>
#include <QStringList>
#include <QUrl>
#include <utility>

namespace {

bool isHttpStream(const QString &address)
{
    const QUrl url(address, QUrl::StrictMode);
    return url.isValid() && !url.host().isEmpty()
           && (url.scheme() == QStringLiteral("http") || url.scheme() == QStringLiteral("https"));
}

qsizetype titleSeparator(QStringView info)
{
    bool quoted = false;
    for (qsizetype i = 0; i < info.size(); ++i) {
        if (info[i] == '"' && (i == 0 || info[i - 1] != '\\'))
            quoted = !quoted;
        else if (info[i] == ',' && !quoted)
            return i;
    }
    return -1;
}

} // namespace

QString M3UHandler::parse(const QString &text, QVector<RadioStation> &stations)
{
    const QStringList lines = text.split('\n');
    const QString prefix = QStringLiteral("#EXTINF:");
    static const QRegularExpression Duration(QStringLiteral(R"(^[+-]?\d+(?:\.\d+)?(?:\s|$))"));
    QVector<RadioStation> parsed;
    QString title;

    for (qsizetype i = 0; i < lines.size(); ++i) {
        QString line = lines[i];
        if (line.endsWith('\r'))
            line.chop(1);
        const QString trimmed = line.trimmed();
        if (trimmed.isEmpty())
            continue;

        if (trimmed.startsWith(QStringLiteral("#EXTINF"))) {
            if (!trimmed.startsWith(prefix))
                return QObject::tr("Invalid EXTINF entry on line %1.").arg(i + 1);
            if (!title.isEmpty())
                return QObject::tr("Missing stream URL before line %1 in the M3U playlist.").arg(i + 1);
            const QStringView info = QStringView(line).sliced(line.indexOf(prefix) + prefix.size());
            const qsizetype separator = titleSeparator(info);
            if (separator < 0 || !Duration.match(info.left(separator).trimmed().toString()).hasMatch())
                return QObject::tr("Invalid EXTINF entry on line %1.").arg(i + 1);
            title = info.sliced(separator + 1).toString();
            if (title.trimmed().isEmpty() || title.contains('\r') || title.contains(QChar::Null))
                return QObject::tr("Invalid station title on line %1 in the M3U playlist.").arg(i + 1);
        } else if (!trimmed.startsWith('#')) {
            if (title.isEmpty() || !isHttpStream(trimmed))
                return QObject::tr("Expected a station title and HTTP(S) stream on line %1.").arg(i + 1);
            RadioStation station;
            station.station = title;
            station.streamUrl = trimmed;
            parsed.append(std::move(station));
            title.clear();
        }
    }
    if (!title.isEmpty())
        return QObject::tr("The M3U playlist ends before the final station's stream URL.");
    if (parsed.isEmpty())
        return QObject::tr("The M3U playlist contains no stations.");

    stations = std::move(parsed);
    return {};
}

QString M3UHandler::serialize(const QVector<RadioStation> &stations, QString &text)
{
    QString playlist = QStringLiteral("#EXTM3U\n");
    for (const RadioStation &station : stations) {
        if (station.station.trimmed().isEmpty() || station.station.contains('\r')
            || station.station.contains('\n') || station.station.contains(QChar::Null)
            || !isHttpStream(station.streamUrl.trimmed())) {
            return QObject::tr("Cannot export this station to M3U: %1").arg(station.station);
        }
        playlist += QStringLiteral("#EXTINF:-1,") + station.station + '\n'
                    + station.streamUrl.trimmed() + '\n';
    }
    text = std::move(playlist);
    return {};
}
