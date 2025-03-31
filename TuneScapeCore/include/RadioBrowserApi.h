#ifndef RADIOBROWSERAPI_H
#define RADIOBROWSERAPI_H
#include <QHostInfo>
#include <QNetworkInterface>
#include <QProcess>
#include <QDebug>
#include <QtConcurrent>
#include <QMutex>
#include <qregularexpression.h>
#include <QDnsLookup>
#include <QDnsServiceRecord>

class RadioBrowserApi {
public:
    static QStringList getRadioBrowserBaseUrls() {
        QStringList baseUrls;

        //DNS SRV dla _api._tcp.radio-browser.info
        QString baseUrl = "_api._tcp.radio-browser.info";
        QDnsLookup lookup(QDnsLookup::SRV, baseUrl);

        // DNS QUERY
        QEventLoop loop;
        QTimer timeoutTimer;
        timeoutTimer.setSingleShot(true);

        QObject::connect(&timeoutTimer, &QTimer::timeout, &loop, &QEventLoop::quit);
        QObject::connect(&lookup, &QDnsLookup::finished, &loop, &QEventLoop::quit);

        timeoutTimer.start(5000);  // Timeout
        lookup.lookup();
        loop.exec();

        if (timeoutTimer.isActive()) {
            if (lookup.error() == QDnsLookup::NoError) {
                const QList<QDnsServiceRecord> srvRecords = lookup.serviceRecords();

                QList<QDnsServiceRecord> sortedRecords = srvRecords;
                std::sort(sortedRecords.begin(), sortedRecords.end(),
                          [](const QDnsServiceRecord &a, const QDnsServiceRecord &b) {
                              if (a.priority() != b.priority())
                                  return a.priority() < b.priority();
                              return a.weight() > b.weight();
                          });

                for (const QDnsServiceRecord &record : sortedRecords) {
                    QString target = record.target();
                    if (target.endsWith('.')) {
                        target.chop(1);
                    }
                    baseUrls.append("https://" + target + "/");
                }
            } else {
                qWarning() << "DNS SRV lookup failed:" << lookup.errorString();
            }
        } else {
            qWarning() << "DNS lookup timed out.";
        }

        return baseUrls;
    }
};

#endif // RADIOBROWSERAPI_H
