#include "include/theme.h"
#include "qsettings.h"
#include "qtimer.h"

Theme::Theme()
    : filetheme(":/src/theme/Combinear.qss")
    , lightTheme(":/src/theme/LightTheme.qss")
    , lightInDarkWIndowsTheme(":/src/theme/LightInDarkWindows.qss")
    , style{""}
{
    filetheme.open(QFile::ReadOnly);
    lightTheme.open(QFile::ReadOnly);
    lightInDarkWIndowsTheme.open(QFile::ReadOnly);
    style = filetheme.readAll();
    lightThm = lightTheme.readAll();
    lightInDarkWindows = lightInDarkWIndowsTheme.readAll();
    setLightTheme();

    // QTimer *themeCheckTimer = new QTimer(this);
    // connect(themeCheckTimer, &QTimer::timeout, this, &Theme::checkThemeChange);
    // themeCheckTimer->start(5000);
}
// QTC_TEMP
bool Theme::getIsDark() const
{
    return isDark;
}

void Theme::setIsDark(bool newIsDark)
{
    isDark = newIsDark;
}

void Theme::setTheme()
{
    if (!isDark) {
        setDarkTheme();
        isDark = true;
    } else {
        setLightTheme();
        isDark = false;
    }
}

void Theme::setDarkTheme()
{
    app->setStyleSheet(style);
}

void Theme::setLightTheme()
{
    if (isSystemDarkMode())
        app->setStyleSheet(lightInDarkWindows);
    else
        app->setStyleSheet(lightThm);
}

void Theme::getApplicationObject(QApplication &app)
{
    qDebug() << "System theme!? : " << isSystemDarkMode();
    connect(ui->themeButton, &QPushButton::clicked, this, &Theme::setTheme);
    this->app = &app;
}

bool Theme::isSystemDarkMode()
{
    QSettings settings(THEME_REGISTER, QSettings::NativeFormat);
    return settings.value("AppsUseLightTheme", 1).toInt() == 0;
}
//TODO
// void Theme::checkThemeChange()
// {
//
// }

Ui::MainWindow *Theme::getUi() const
{
    return ui;
}

void Theme::setUi(Ui::MainWindow *newUi)
{
    ui = newUi;
}
