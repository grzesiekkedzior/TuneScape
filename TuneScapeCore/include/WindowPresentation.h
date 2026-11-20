#ifndef WINDOWPRESENTATION_H
#define WINDOWPRESENTATION_H
#include <QList>
#include <QRect>
#include <QString>
class QWidget;
namespace WindowPresentation {
bool supportsPositioning(const QString &platform);
QRect fittedGeometry(const QRect &geometry, const QList<QRect> &availableScreens);
void keepOnScreen(QWidget *window);
void restore(QWidget *window);
}
#endif
