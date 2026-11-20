#include "include/WindowPresentation.h"
#include <QGuiApplication>
#include <QScreen>
#include <QWidget>

bool WindowPresentation::supportsPositioning(const QString &platform)
{
    return !platform.startsWith("wayland", Qt::CaseInsensitive);
}

QRect WindowPresentation::fittedGeometry(const QRect &geometry, const QList<QRect> &availableScreens)
{
    QRect screen;
    qint64 bestOverlap = -1;
    for (const auto &candidate : availableScreens) {
        if (!candidate.isValid())
            continue;
        const auto overlap = candidate.intersected(geometry);
        const auto area = qint64(overlap.width()) * overlap.height();
        if (screen.isNull() || area > bestOverlap) {
            screen = candidate;
            bestOverlap = area;
        }
    }
    if (screen.isNull())
        return geometry;
    const QSize size(qBound(1, geometry.width(), screen.width()), qBound(1, geometry.height(), screen.height()));
    return QRect(QPoint(qBound(screen.left(), geometry.x(), screen.right() - size.width() + 1),
                        qBound(screen.top(), geometry.y(), screen.bottom() - size.height() + 1)), size);
}

void WindowPresentation::keepOnScreen(QWidget *window)
{
    if (!window || !supportsPositioning(QGuiApplication::platformName()) || window->isMaximized() || window->isFullScreen())
        return;
    QList<QRect> screens;
    if (auto *primary = QGuiApplication::primaryScreen())
        screens.append(primary->availableGeometry());
    for (auto *screen : QGuiApplication::screens()) {
        if (screen != QGuiApplication::primaryScreen())
            screens.append(screen->availableGeometry());
    }
    const auto frame = window->frameGeometry();
    const auto fitted = fittedGeometry(frame, screens);
    if (fitted == frame)
        return;
    const QSize decoration = frame.size() - window->size();
    window->resize(qMax(1, fitted.width() - decoration.width()), qMax(1, fitted.height() - decoration.height()));
    window->move(fitted.topLeft());
}

void WindowPresentation::restore(QWidget *window)
{
    if (!window)
        return;
    if (window->isMinimized())
        window->setWindowState(window->windowState() & ~Qt::WindowMinimized);
    window->show();
    keepOnScreen(window);
    // These are requests to the window manager; Wayland may deny focus/stacking.
    window->raise();
    window->activateWindow();
}
