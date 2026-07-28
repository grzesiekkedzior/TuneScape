#ifndef CUSTOMCOLORDELEGATE_H
#define CUSTOMCOLORDELEGATE_H

#include <QColor>
#include <QStyledItemDelegate>

class CustomColorDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit CustomColorDelegate(int targetRow, const QColor &color, QObject *parent = nullptr);

    void paint(QPainter *painter,
               const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;

    void clearRowColor();

private:
    int targetRow = -1;
    QColor rowColor;
};

#endif // CUSTOMCOLORDELEGATE_H