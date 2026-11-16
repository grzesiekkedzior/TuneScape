#include "include/jsonlistprocessor.h"

#include <QJsonObject>
#include <QRegularExpression>

QVector<RadioStation> JsonListProcessor::stationsFromJson(const QJsonArray &array)
{
    static const QRegularExpression namePrefix(QStringLiteral("^[\\s?_.-]+"));
    QVector<RadioStation> stations;
    stations.reserve(array.size());

    for (const QJsonValue &value : array) {
        const auto object = value.toObject();
        RadioStation station;
        station.stationUuid = QUuid(object["stationuuid"].toString());
        station.station = object["name"].toString().trimmed().remove(namePrefix);
        station.genre = object["tags"].toString().section(',', 0, 0).trimmed();
        station.country = object["country"].toString().trimmed();
        station.homepage = object["homepage"].toString().trimmed();
        station.streamUrl = object["url_resolved"].toString().trimmed();
        if (station.streamUrl.isEmpty())
            station.streamUrl = object["url"].toString().trimmed();
        station.iconUrl = object["favicon"].toString().trimmed();
        station.codec = object["codec"].toString().trimmed();
        station.language = object["language"].toString().trimmed();
        station.state = object["state"].toString().trimmed();
        station.countryCode = object["countrycode"].toString().trimmed();
        station.bitrate = object["bitrate"].toInt();
        station.votes = object["votes"].toInt();
        station.clickCount = object["clickcount"].toInt();
        stations.append(std::move(station));
    }

    return stations;
}
