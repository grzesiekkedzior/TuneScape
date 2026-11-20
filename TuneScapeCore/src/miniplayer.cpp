#include "include/miniplayer.h"
#include "include/WindowPresentation.h"
#include <QCloseEvent>
#include <QGuiApplication>
#include <QScreen>
#include <QShowEvent>
#include <QTimer>

miniplayer::miniplayer()
{
    mui.setupUi(this);
    mui.stayTop->setAutoExclusive(false);
    connect(mui.stayTop, &QRadioButton::clicked, this, &miniplayer::stayOnTop);
    setStyleSheet(":/src/theme/Combinear.qss");
    for (auto *screen : QGuiApplication::screens())
        watchScreen(screen);
    connect(qGuiApp, &QGuiApplication::screenAdded, this, [this](QScreen *screen) {
        watchScreen(screen);
        keepOnScreen();
    });
    connect(qGuiApp, &QGuiApplication::screenRemoved, this, [this] {
        QTimer::singleShot(0, this, &miniplayer::keepOnScreen);
    });
}

Ui::Frame *miniplayer::getMui()
{
    return &mui;
}

void miniplayer::stayOnTop()
{
    const bool visible = isVisible();
    const auto state = windowState();
    setWindowFlag(Qt::WindowStaysOnTopHint, mui.stayTop->isChecked());
    setWindowState(state);
    if (visible)
        show();
}

void miniplayer::watchScreen(QScreen *screen)
{
    connect(screen, &QScreen::availableGeometryChanged, this, &miniplayer::keepOnScreen, Qt::UniqueConnection);
}

void miniplayer::keepOnScreen()
{
    if (isVisible())
        WindowPresentation::keepOnScreen(this);
}

void miniplayer::showEvent(QShowEvent *event)
{
    QFrame::showEvent(event);
    keepOnScreen();
}

void miniplayer::closeEvent(QCloseEvent *event)
{
    emit restoreMainWindowRequested();
    QFrame::closeEvent(event);
}
