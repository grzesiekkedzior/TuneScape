#include "include/customcolordelegate.h"

#include <QPainter>

CustomColorDelegate::CustomColorDelegate(int target, const QColor &color, QObject *parent)
    : QStyledItemDelegate(parent)
    , targetRow(target)
    , rowColor(color)
{}

void CustomColorDelegate::paint(QPainter *painter,
                                const QStyleOptionViewItem &option,
                                const QModelIndex &index) const
{
    QStyleOptionViewItem opt(option);
    initStyleOption(&opt, index);

    if (targetRow != -1 && index.row() == targetRow) {
        painter->save();
        painter->fillRect(opt.rect, rowColor);
        painter->restore();

        const QColor textColor = rowColor.lightnessF() < 0.5 ? Qt::white : Qt::black;

        opt.palette.setColor(QPalette::Text, textColor);
        opt.palette.setColor(QPalette::WindowText, textColor);

        opt.state &= ~QStyle::State_Selected;
        opt.backgroundBrush = Qt::NoBrush;
    }

    QStyledItemDelegate::paint(painter, opt, index);
}

void CustomColorDelegate::clearRowColor()
{
    targetRow = -1;
}