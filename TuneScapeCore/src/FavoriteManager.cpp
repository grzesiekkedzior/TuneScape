#include "include/FavoriteManager.h"
#include "include/M3UHandler.h"

#include <QFile>
#include <QFileInfo>
#include <QLockFile>
#include <QSaveFile>
#include <QStringDecoder>
#include <QStringList>
#include <QUrl>
#include <algorithm>
#include <filesystem>
#include <optional>
#include <utility>

namespace {

const QString CsvHeader = QStringLiteral("# TuneScape Favorites CSV 2");

QUrl normalizedStreamUrl(const QString &streamUrl)
{
    QUrl url(streamUrl.trimmed(), QUrl::StrictMode);
    if (!url.isValid() || url.scheme().isEmpty() || url.host().isEmpty())
        return {};

    const bool http = url.scheme() == QStringLiteral("http");
    const bool https = url.scheme() == QStringLiteral("https");
    if ((http && url.port() == 80) || (https && url.port() == 443))
        url.setPort(-1);
    if ((http || https) && url.path().isEmpty())
        url.setPath(QStringLiteral("/"));

    return url.adjusted(QUrl::NormalizePathSegments | QUrl::RemoveFragment);
}

QString readText(const QString &path, QString &text, bool allowMissing = false)
{
    const QFileInfo info(path);
    if (allowMissing && !info.isSymLink()) {
        std::error_code error;
        if (!std::filesystem::exists(info.filesystemAbsoluteFilePath(), error) && !error) {
            text.clear();
            return {};
        }
    }

    if (!info.isFile())
        return QObject::tr("The playlist is missing or is not a regular file: %1").arg(path);

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return QObject::tr("Cannot open playlist for reading %1: %2").arg(path, file.errorString());

    const QByteArray bytes = file.readAll();
    if (file.error() != QFileDevice::NoError)
        return QObject::tr("Cannot read playlist %1: %2").arg(path, file.errorString());

    QStringDecoder decoder(QStringDecoder::Utf8, QStringConverter::Flag::Stateless);
    text = decoder(bytes);
    if (decoder.hasError())
        return QObject::tr("The playlist is not valid UTF-8: %1").arg(path);
    if (text.startsWith(QChar::ByteOrderMark))
        text.remove(0, 1);
    return {};
}

QString writeText(const QString &path, const QString &text)
{
    QSaveFile file(path);
    file.setDirectWriteFallback(false);
    if (!file.open(QIODevice::WriteOnly))
        return QObject::tr("Cannot open playlist for writing %1: %2").arg(path, file.errorString());

    const QByteArray bytes = text.toUtf8();
    if (file.write(bytes) != bytes.size()) {
        const QString error = file.errorString();
        file.cancelWriting();
        return QObject::tr("Cannot write playlist %1: %2").arg(path, error);
    }
    if (!file.commit())
        return QObject::tr("Cannot replace playlist %1: %2").arg(path, file.errorString());
    return {};
}

bool sameFile(const QString &source, const QString &destination)
{
    const QFileInfo first(source);
    const QFileInfo second(destination);
    if (first.absoluteFilePath() == second.absoluteFilePath())
        return true;

    std::error_code error;
    return std::filesystem::equivalent(first.filesystemAbsoluteFilePath(),
                                       second.filesystemAbsoluteFilePath(), error);
}

QString lockPath(const QString &filePath)
{
    const QFileInfo info(filePath);
    const QString canonical = info.canonicalFilePath();
    return (canonical.isEmpty() ? filePath : canonical) + QStringLiteral(".lock");
}

QString lockError(const QLockFile &lock, const QString &path)
{
    if (lock.error() == QLockFile::LockFailedError)
        return QObject::tr("The Favorites playlist is being saved by another instance: %1").arg(path);
    return QObject::tr("Cannot lock the Favorites playlist for writing: %1").arg(path);
}

std::optional<QStringList> parseCsvRecord(QStringView record)
{
    QStringList fields;
    QString field;
    bool quoted = false;
    bool quoteClosed = false;

    for (qsizetype i = 0; i < record.size(); ++i) {
        const QChar ch = record[i];
        if (quoted) {
            if (ch != '"') {
                field += ch;
            } else if (i + 1 < record.size() && record[i + 1] == '"') {
                field += ch;
                ++i;
            } else {
                quoted = false;
                quoteClosed = true;
            }
        } else if (ch == ',') {
            fields.append(field);
            field.clear();
            quoteClosed = false;
        } else if (quoteClosed) {
            return std::nullopt;
        } else if (ch == '"') {
            if (!field.isEmpty())
                return std::nullopt;
            quoted = true;
        } else {
            field += ch;
        }
    }
    if (quoted)
        return std::nullopt;
    fields.append(field);
    return fields;
}

QString parseNative(const QString &text, QVector<RadioStation> &stations)
{
    const QStringList lines = text.split('\n');
    const bool quoted = lines.constFirst().trimmed() == CsvHeader;
    QVector<RadioStation> parsed;

    for (qsizetype i = quoted ? 1 : 0; i < lines.size(); ++i) {
        QString line = lines[i];
        if (line.endsWith('\r'))
            line.chop(1);
        if (line.trimmed().isEmpty())
            continue;

        // The original writer did not quote fields. A raw quote in these rows is literal.
        const auto fields = quoted ? parseCsvRecord(line)
                                   : std::optional<QStringList>(line.split(','));
        if (!fields || fields->size() != (quoted ? 7 : 6) || line.contains('\r')
            || line.contains(QChar::Null))
            return QObject::tr("Invalid or ambiguous Favorites record on line %1.").arg(i + 1);

        RadioStation station;
        station.iconUrl = fields->at(0);
        station.streamUrl = fields->at(1);
        station.station = fields->at(2);
        station.country = fields->at(3);
        station.genre = fields->at(4);
        station.homepage = fields->at(5);
        if (station.station.trimmed().isEmpty() || normalizedStreamUrl(station.streamUrl).isEmpty())
            return QObject::tr("Invalid station name or stream URL on line %1.").arg(i + 1);

        if (quoted && !fields->at(6).isEmpty()) {
            station.stationUuid = QUuid(fields->at(6));
            if (station.stationUuid.isNull())
                return QObject::tr("Invalid station UUID on line %1.").arg(i + 1);
        }
        parsed.append(std::move(station));
    }

    stations = std::move(parsed);
    return {};
}

QString writeNative(const QString &path, const QVector<RadioStation> &stations)
{
    QString text = CsvHeader + '\n';
    for (const RadioStation &station : stations) {
        QStringList fields = {station.iconUrl, station.streamUrl, station.station, station.country,
                              station.genre, station.homepage,
                              station.stationUuid.isNull()
                                  ? QString() : station.stationUuid.toString(QUuid::WithoutBraces)};
        for (QString &field : fields) {
            if (field.contains('\r') || field.contains('\n') || field.contains(QChar::Null))
                return QObject::tr("Favorites fields cannot contain line breaks or NUL characters.");
            field.replace(QStringLiteral("\""), QStringLiteral("\"\""));
            field = '"' + field + '"';
        }
        text += fields.join(',') + '\n';
    }
    return writeText(path, text);
}

} // namespace

FavoriteManager::FavoriteManager(QObject *parent)
    : FavoriteManager(QStringLiteral("radiobrowser.txt"), parent)
{}

FavoriteManager::FavoriteManager(const QString &filePath, QObject *parent)
    : QObject(parent)
    , m_filePath(QFileInfo(filePath).absoluteFilePath())
{}

QString FavoriteManager::load(QVector<RadioStation> &stations) const
{
    QString text;
    const QString error = readText(m_filePath, text, true);
    return error.isEmpty() ? parseNative(text, stations) : error;
}

bool FavoriteManager::sameStation(const RadioStation &first, const RadioStation &second)
{
    if (!first.stationUuid.isNull() && !second.stationUuid.isNull())
        return first.stationUuid == second.stationUuid;

    const QUrl firstUrl = normalizedStreamUrl(first.streamUrl);
    return !firstUrl.isEmpty() && firstUrl == normalizedStreamUrl(second.streamUrl);
}

QString FavoriteManager::setFavorite(const RadioStation &station, bool favorite,
                                     QVector<RadioStation> &stations) const
{
    QLockFile lock(lockPath(m_filePath));
    lock.setStaleLockTime(0);
    if (!lock.tryLock(0))
        return lockError(lock, m_filePath);

    QVector<RadioStation> updated;
    QString error = load(updated);
    if (!error.isEmpty())
        return error;

    const auto matches = [&station](const RadioStation &entry) { return sameStation(station, entry); };
    const auto first = std::find_if(updated.cbegin(), updated.cend(), matches);

    if (favorite && first == updated.cend()) {
        if (station.station.trimmed().isEmpty() || normalizedStreamUrl(station.streamUrl).isEmpty())
            return tr("The station must have a name and a valid stream URL.");
        updated.append(station);
        error = writeNative(m_filePath, updated);
    } else if (!favorite && first != updated.cend()) {
        // A URL alone cannot select between two different known station IDs.
        if (station.stationUuid.isNull()) {
            QUuid matchedId;
            for (const RadioStation &entry : std::as_const(updated)) {
                if (entry.stationUuid.isNull() || !matches(entry))
                    continue;
                if (!matchedId.isNull() && matchedId != entry.stationUuid)
                    return tr("More than one favorite shares this stream URL. Select the station in Favorites.");
                matchedId = entry.stationUuid;
            }
        }
        updated.erase(std::remove_if(updated.begin(), updated.end(), matches), updated.end());
        error = writeNative(m_filePath, updated);
    }

    if (error.isEmpty())
        stations = std::move(updated);
    return error;
}

QString FavoriteManager::importPlaylist(const QString &source, QVector<RadioStation> &stations) const
{
    if (sameFile(source, m_filePath))
        return tr("The selected file is already the active Favorites playlist.");

    const QString suffix = QFileInfo(source).suffix().toLower();
    const bool m3u = suffix == QStringLiteral("m3u") || suffix == QStringLiteral("m3u8");
    if (!m3u && suffix != QStringLiteral("txt"))
        return tr("Select a TuneScape TXT or an M3U playlist.");

    QString text;
    QString error = readText(source, text);
    if (!error.isEmpty())
        return error;

    QVector<RadioStation> imported;
    error = m3u ? M3UHandler::parse(text, imported) : parseNative(text, imported);
    if (!error.isEmpty())
        return error;
    if (imported.isEmpty())
        return tr("The selected playlist contains no stations.");

    QLockFile lock(lockPath(m_filePath));
    lock.setStaleLockTime(0);
    if (!lock.tryLock(0))
        return lockError(lock, m_filePath);
    if (sameFile(source, m_filePath))
        return tr("The selected file is already the active Favorites playlist.");

    error = writeNative(m_filePath, imported);
    if (error.isEmpty())
        stations = std::move(imported);
    return error;
}

QString FavoriteManager::exportPlaylist(const QString &destination, Format format) const
{
    if (sameFile(m_filePath, destination))
        return tr("Choose a different path to export the Favorites playlist.");

    QVector<RadioStation> stations;
    const QString error = load(stations);
    if (!error.isEmpty())
        return error;
    if (stations.isEmpty())
        return tr("There are no favorite stations to export.");

    if (format == Format::Native)
        return writeNative(destination, stations);

    QString text;
    const QString formatError = M3UHandler::serialize(stations, text);
    return formatError.isEmpty() ? writeText(destination, text) : formatError;
}
