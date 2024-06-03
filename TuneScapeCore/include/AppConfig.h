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
    bool checkFile();
    bool checkBoolState();
    bool changeBoolState(bool state);

private:
    QFile *file = nullptr;
};

#endif // APPCONFIG_H
