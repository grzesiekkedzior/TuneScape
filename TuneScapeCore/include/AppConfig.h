#ifndef APPCONFIG_H
#define APPCONFIG_H
#include <QFile>
#include <QObject>

enum class Notification { FALSE, TRUE };

class AppConfig : public QObject
{
    Q_OBJECT
public:
    explicit AppConfig(QString path);
    ~AppConfig();
    bool checkFile();
    bool checkBoolState(QString properties);
    bool changeBoolState(bool state, QString properties);

private:
    QFile *file = nullptr;
};

#endif // APPCONFIG_H
