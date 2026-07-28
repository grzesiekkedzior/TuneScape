#include "include/theme.h"

#include <QFile>
#include <QSettings>

Theme::Theme(QApplication &application, QObject *parent)
    : QObject(parent)
    , m_application(application)
{
    const Type savedTheme = loadSavedTheme();

    if (!applyTheme(savedTheme))
        applyTheme(Type::System);
}

Theme::Type Theme::currentTheme() const
{
    return m_currentTheme;
}

bool Theme::applyTheme(Type theme)
{
    const QString path = themePath(theme);
    const QString styleSheet = loadStyleSheet(path);

    if (styleSheet.isEmpty())
        return false;

    m_application.setStyleSheet(styleSheet);
    m_currentTheme = theme;

    saveTheme(theme);
    emit themeChanged(theme);

    return true;
}

QString Theme::themePath(Type theme) const
{
    switch (theme) {
    case Type::System:
        return QString::fromLatin1(SystemThemePath);

    case Type::Dark:
        return QString::fromLatin1(DarkThemePath);

    case Type::Classic:
        return QString::fromLatin1(ClassicThemePath);

    case Type::Cyberpunk:
        return QString::fromLatin1(CyberpunkThemePath);
    }

    return QString::fromLatin1(SystemThemePath);
}

QString Theme::loadStyleSheet(const QString &path) const
{
    if (path.isEmpty())
        return {};

    QFile file(path);

    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        qWarning() << "Unable to open theme file:" << path;
        return {};
    }

    return QString::fromUtf8(file.readAll());
}

Theme::Type Theme::loadSavedTheme() const
{
    QSettings settings(QString::fromLatin1(SettingsFile), QSettings::IniFormat);

    const QString savedTheme
        = settings.value(QString::fromLatin1(ThemeProperty), QStringLiteral("system")).toString();

    return themeFromString(savedTheme);
}

void Theme::saveTheme(Type theme) const
{
    QSettings settings(QString::fromLatin1(SettingsFile), QSettings::IniFormat);

    settings.setValue(QString::fromLatin1(ThemeProperty), themeToString(theme));

    settings.sync();
}

QString Theme::themeToString(Type theme)
{
    switch (theme) {
    case Type::System:
        return QStringLiteral("system");

    case Type::Dark:
        return QStringLiteral("dark");

    case Type::Classic:
        return QStringLiteral("classic");

    case Type::Cyberpunk:
        return QStringLiteral("cyberpunk");
    }

    return QStringLiteral("system");
}

Theme::Type Theme::themeFromString(const QString &value)
{
    const QString normalizedValue = value.trimmed().toLower();

    if (normalizedValue == QStringLiteral("dark"))
        return Type::Dark;

    if (normalizedValue == QStringLiteral("classic"))
        return Type::Classic;

    if (normalizedValue == QStringLiteral("cyberpunk"))
        return Type::Cyberpunk;

    return Type::System;
}