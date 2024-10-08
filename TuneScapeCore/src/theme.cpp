#include "include/theme.h"

Theme::Theme()
    : filetheme(":/src/theme/Combinear.qss")
    , lightTheme(":/src/theme/LightTheme.qss")
    , style{""}
{
    filetheme.open(QFile::ReadOnly);
    lightTheme.open(QFile::ReadOnly);
    style = filetheme.readAll();
    lightThm = lightTheme.readAll();
    setLightTheme();
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
    app->setStyleSheet(lightThm);
}

void Theme::getApplicationObject(QApplication &app)
{
    connect(ui->themeButton, &QPushButton::clicked, this, &Theme::setTheme);
    this->app = &app;
}

Ui::MainWindow *Theme::getUi() const
{
    return ui;
}

void Theme::setUi(Ui::MainWindow *newUi)
{
    ui = newUi;
}
