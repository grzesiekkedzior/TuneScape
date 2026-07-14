#include "include/IconLoader.h"
#include <QVBoxLayout>

IconLoader::IconLoader(QObject *parent) {}

QLabel *IconLoader::createLabel(const QString &text)
{
    QLabel *label = new QLabel(text);
    label->setFixedWidth(120);
    label->setWordWrap(true);
    label->setAlignment(Qt::AlignCenter);

    return label;
}

QPushButton *IconLoader::createIconButton(int row)
{
    QPushButton *button = new QPushButton;
    button->setFixedSize(120, 120);

    QIcon icon(TUNESCAPE_ICON);
    button->setIcon(icon);
    button->setIconSize(QSize(100, 100));

    connect(button, &QPushButton::clicked, this, [this, row]() { emit iconClicked(row); });

    return button;
}

QWidget *IconLoader::createIconButtonWithLabel(int row, const QString &stationName)
{
    QWidget *itemContainer = new QWidget;

    QVBoxLayout *itemLayout = new QVBoxLayout(itemContainer);

    QLabel *label = createLabel(stationName);
    QPushButton *button = createIconButton(row);

    itemLayout->addWidget(button);
    itemLayout->addWidget(label);

    return itemContainer;
}

void IconLoader::resizeCache(int size)
{
    buttonCache.resize(size, nullptr);
}

void IconLoader::setButton(int row, QWidget *button)
{
    if (row < buttonCache.size())
        buttonCache[row] = button;
}

QWidget *IconLoader::button(int row) const
{
    if (row < 0 || row >= buttonCache.size())
        return nullptr;

    return buttonCache[row];
}

bool IconLoader::containsEmptyButton() const
{
    return buttonCache.contains(nullptr);
}

int IconLoader::buttonCount() const
{
    return buttonCache.size();
}

void IconLoader::clearCache()
{
    buttonCache.clear();
}

QVector<QWidget *> IconLoader::getButtonCache() const
{
    return buttonCache;
}
