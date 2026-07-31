#include "include/StationDetailsDialog.h"

#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QUrl>
#include <QVBoxLayout>

namespace {

constexpr auto DefaultRadioIconPath = ":/images/img/radio30.png";

}

StationDetailsDialog::StationDetailsDialog(const RadioStation &station, QWidget *parent)
    : QDialog(parent)
    , m_station(station)
{
    setWindowTitle(tr("Station details"));
    setWindowIcon(QIcon(DefaultRadioIconPath));
    setModal(true);
    setMinimumWidth(520);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 24, 24, 20);
    mainLayout->setSpacing(18);

    /*
     * Header:
     *
     * [icon] Station name
     *        Country
     */
    auto *headerLayout = new QHBoxLayout;
    headerLayout->setSpacing(16);

    m_iconLabel = new QLabel(this);
    m_iconLabel->setFixedSize(120, 120);
    m_iconLabel->setAlignment(Qt::AlignCenter);

    QIcon stationIcon = station.icon;

    if (stationIcon.isNull())
        stationIcon = QIcon(DefaultRadioIconPath);

    m_iconLabel->setPixmap(stationIcon.pixmap(QSize(112, 112), devicePixelRatioF()));

    auto *titleLayout = new QVBoxLayout;
    titleLayout->setSpacing(4);

    auto *stationNameLabel = new QLabel(station.station, this);

    QFont titleFont = stationNameLabel->font();
    titleFont.setPointSize(titleFont.pointSize() + 4);
    titleFont.setBold(true);

    stationNameLabel->setFont(titleFont);
    stationNameLabel->setWordWrap(true);
    stationNameLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

    auto *countryLabel = new QLabel(this);

    if (station.country.isEmpty())
        countryLabel->setText(tr("Unknown country"));
    else
        countryLabel->setText(station.country);

    countryLabel->setProperty("secondaryText", true);
    countryLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

    titleLayout->addWidget(stationNameLabel);
    titleLayout->addWidget(countryLabel);
    titleLayout->addStretch();

    headerLayout->addWidget(m_iconLabel);
    headerLayout->addLayout(titleLayout, 1);

    mainLayout->addLayout(headerLayout);

    auto *separator = new QFrame(this);
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Sunken);

    mainLayout->addWidget(separator);

    /*
     * Station information.
     */
    auto *formLayout = new QFormLayout;
    formLayout->setHorizontalSpacing(24);
    formLayout->setVerticalSpacing(12);
    formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

    formLayout->addRow(tr("Genre"),
                       createValueLabel(station.genre.isEmpty() ? tr("Not available")
                                                                : station.genre));

    formLayout->addRow(tr("Country"),
                       createValueLabel(station.country.isEmpty() ? tr("Not available")
                                                                  : station.country));

    formLayout->addRow(tr("Stream URL"),
                       createValueLabel(station.streamUrl.isEmpty() ? tr("Not available")
                                                                    : station.streamUrl));

    formLayout->addRow(tr("Homepage"),
                       createValueLabel(station.homepage.isEmpty() ? tr("Not available")
                                                                   : station.homepage));
    formLayout->addRow(tr("Codec"),
                       createValueLabel(station.codec.isEmpty() ? tr("Not available")
                                                                : station.codec));

    formLayout->addRow(tr("Bitrate"),
                       createValueLabel(station.bitrate > 0 ? tr("%1 kbps").arg(station.bitrate)
                                                            : tr("Not available")));

    formLayout->addRow(tr("Language"),
                       createValueLabel(station.language.isEmpty() ? tr("Not available")
                                                                   : station.language));

    formLayout->addRow(tr("State"),
                       createValueLabel(station.state.isEmpty() ? tr("Not available")
                                                                : station.state));

    formLayout->addRow(tr("Country code"),
                       createValueLabel(station.countryCode.isEmpty() ? tr("Not available")
                                                                      : station.countryCode));

    formLayout->addRow(tr("Votes"), createValueLabel(QString::number(station.votes)));

    formLayout->addRow(tr("Click count"), createValueLabel(QString::number(station.clickCount)));

    mainLayout->addLayout(formLayout);

    /*
     * Action buttons.
     */
    auto *actionsLayout = new QHBoxLayout;
    actionsLayout->setSpacing(8);

    auto *copyStreamButton = new QPushButton(tr("Copy stream URL"), this);

    m_openHomepageButton = new QPushButton(tr("Open homepage"), this);

    m_openHomepageButton->setEnabled(!station.homepage.isEmpty());

    connect(copyStreamButton, &QPushButton::clicked, this, &StationDetailsDialog::copyStreamUrl);

    connect(m_openHomepageButton, &QPushButton::clicked, this, &StationDetailsDialog::openHomepage);

    actionsLayout->addWidget(copyStreamButton);
    actionsLayout->addWidget(m_openHomepageButton);
    actionsLayout->addStretch();

    mainLayout->addLayout(actionsLayout);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, Qt::Horizontal, this);

    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    mainLayout->addWidget(buttonBox);

    setStyleSheet(QStringLiteral(R"(
        StationDetailsDialog {
            background-color: palette(window);
        }

        QLabel[secondaryText="true"] {
            color: palette(mid);
        }

        QFormLayout QLabel {
            padding-top: 2px;
            padding-bottom: 2px;
        }

        QPushButton {
            min-height: 28px;
            padding-left: 12px;
            padding-right: 12px;
        }
    )"));
}

QLabel *StationDetailsDialog::createValueLabel(const QString &text) const
{
    auto *label = new QLabel(text);

    label->setWordWrap(true);
    label->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);

    label->setCursor(Qt::IBeamCursor);

    return label;
}

void StationDetailsDialog::openHomepage()
{
    if (m_station.homepage.isEmpty())
        return;

    QDesktopServices::openUrl(QUrl::fromUserInput(m_station.homepage));
}

void StationDetailsDialog::copyStreamUrl()
{
    if (m_station.streamUrl.isEmpty())
        return;

    QApplication::clipboard()->setText(m_station.streamUrl);
}