#include "include/customcolordelegate.h"

CustomColorDelegate::CustomColorDelegate(int target, const QColor &col,
                                         QObject *parent)
    : QStyledItemDelegate(parent), targetRow(target), color(col) {}

void CustomColorDelegate::paint(QPainter *painter,
                                const QStyleOptionViewItem &option,
                                const QModelIndex &index) const {
    QStyleOptionViewItem opt = option;

    if (targetRow != -1 && index.row() == targetRow) {
        QPalette palette = opt.palette;
        QColor textColor;
        if (color.lightnessF() < 0.5) {
            textColor = Qt::white;
        } else {
            textColor = Qt::black;
        }
        palette.setColor(QPalette::Text, textColor);
        opt.palette = palette;

        QColor adjustedColor = color;
        adjustedColor.setAlpha(255);

        painter->fillRect(opt.rect, adjustedColor);

        painter->setPen(opt.palette.color(QPalette::Text));
        QRect textRect = option.rect.adjusted(3, 0, -3, 0);
        painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter,
                          index.data(Qt::DisplayRole).toString());
    } else {
        QStyledItemDelegate::paint(painter, opt, index);
    }
}

void CustomColorDelegate::clearRowColor() { targetRow = -1; }
