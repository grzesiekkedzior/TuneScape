#ifndef THEME_H
#define THEME_H

#include <QFile>
#include <QObject>
#include "../ui_mainwindow.h"

class Theme : public QObject
{
    Q_OBJECT
public:
    Theme();

    bool getIsDark() const;
    void setIsDark(bool newIsDark);
    void setTheme();
    void setDarkTheme();
    void setLightTheme();
    void getApplicationObject(QApplication &app);
    bool isSystemDarkMode();
    void checkThemeChange();

    Ui::MainWindow *getUi() const;
    void setUi(Ui::MainWindow *newUi);

private:
    Ui::MainWindow *ui;
    QApplication *app = nullptr;
    bool isDark = false;

    QFile filetheme;
    QFile lightTheme;
    QFile lightInDarkWIndowsTheme;
    QString style;
    QString lightThm;
    QString lightInDarkWindows;

    const QString THEME_REGISTER = "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize";
};

#endif // THEME_H
