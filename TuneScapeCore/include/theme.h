#ifndef THEME_H
#define THEME_H

#include <QApplication>
#include <QObject>
#include <QString>

class Theme : public QObject
{
    Q_OBJECT

public:
    enum class Type { System, Dark, Classic, Cyberpunk };
    Q_ENUM(Type)

    explicit Theme(QApplication &application, QObject *parent = nullptr);

    Type currentTheme() const;

public slots:
    bool applyTheme(Type theme);

signals:
    void themeChanged(Theme::Type theme);

private:
    QString themePath(Type theme) const;
    QString loadStyleSheet(const QString &path) const;

    Type loadSavedTheme() const;
    void saveTheme(Type theme) const;

    static QString themeToString(Type theme);
    static Type themeFromString(const QString &value);

    QApplication &m_application;
    Type m_currentTheme = Type::System;

    static constexpr auto SettingsFile = "application.properties";
    static constexpr auto ThemeProperty = "theme";

    static constexpr auto DarkThemePath = ":/src/theme/Combinear.qss";
    static constexpr auto SystemThemePath = ":/src/theme/LightTheme.qss";
    static constexpr auto ClassicThemePath = ":/src/theme/Light.qss";

    static constexpr auto CyberpunkThemePath = ":/src/theme/Cyberpunk.qss";
};

#endif // THEME_H