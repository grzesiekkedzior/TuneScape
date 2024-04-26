#ifndef CUSTOMCOLORDELEGATE_H
#define CUSTOMCOLORDELEGATE_H

#include <QColor>
#include <QObject>
#include <QPainter>
#include <QStyleOptionViewItem>
#include <QStyledItemDelegate>

class CustomColorDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    CustomColorDelegate(int target, const QColor &col, QObject *parent = nullptr);

    void paint(QPainter *painter,
               const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;

    void clearRowColor();

private:
    int targetRow;
    QColor color;
};

#endif // CUSTOMCOLORDELEGATE_H
