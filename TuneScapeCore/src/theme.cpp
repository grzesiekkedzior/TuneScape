#include "include/theme.h"
#include "qsettings.h"

Theme::Theme()
    : filetheme(":/src/theme/Combinear.qss")
    , lightTheme(":/src/theme/LightTheme.qss")
    , light(":/src/theme/Light.qss")
    , style{""}
{
    filetheme.open(QFile::ReadOnly);
    lightTheme.open(QFile::ReadOnly);
    light.open(QFile::ReadOnly);
    style = filetheme.readAll();
    lightThm = lightTheme.readAll();
    lightDarkMode = light.readAll();
    appConfig = new AppConfig("application.properties");
    isDark = appConfig->checkBoolState(DARK_THEME_PROPERTY);
    if (isDark) {
        app->setStyleSheet(style);
        isDark = true;
    } else {
        app->setStyleSheet(lightDarkMode);
        isDark = false;
    }
}

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
    if (isDark) {
        if (isSystemDarkMode()) {
            app->setStyleSheet(lightDarkMode);
        } else {
            app->setStyleSheet(lightThm);

        }
        isDark = false;
    } else {
        app->setStyleSheet(style);
        isDark = true;
    }
    appConfig->changeBoolState(isDark, DARK_THEME_PROPERTY);

}

void Theme::setDarkTheme()
{
    app->setStyleSheet(style);
}

void Theme::setLightTheme()
{
    if (isSystemDarkMode())
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
