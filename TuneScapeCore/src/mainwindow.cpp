#include "include/mainwindow.h"
#include <QDebug>
#include <QFile>
#include <QIcon>
#include <QMenu>
#include <QMessageBox>
#include <QSystemTrayIcon>
#include "ui_mainwindow.h"
#include "include/AppConfig.h"
#include "include/theme.h"

MainWindow::MainWindow(Theme &theme, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_theme(theme)
{
    infoDialogApp();

    //******************************************************************
    ui->setupUi(this);
    QMenu *themeMenu = new QMenu(ui->themeButton);

    QAction *systemAction = themeMenu->addAction(tr("System"));
    QAction *darkAction = themeMenu->addAction(tr("Dark"));
    QAction *classicAction = themeMenu->addAction(tr("Classic"));
    QAction *cyberpunkAction = themeMenu->addAction(tr("Cyberpunk"));

    ui->themeButton->setMenu(themeMenu);

    connect(systemAction, &QAction::triggered, this, [this]() {
        m_theme.applyTheme(Theme::Type::System);
    });

    connect(darkAction, &QAction::triggered, this, [this]() {
        m_theme.applyTheme(Theme::Type::Dark);
    });

    connect(classicAction, &QAction::triggered, this, [this]() {
        m_theme.applyTheme(Theme::Type::Classic);
    });

    connect(cyberpunkAction, &QAction::triggered, this, [this]() {
        m_theme.applyTheme(Theme::Type::Cyberpunk);
    });

    ui->previous->hide();
    ui->next->hide();
    ui->licencesTextBrowser->setSource(QUrl("qrc:/src/files/licenses_qtextbrowser.html"));
    ui->licencesTextBrowser->setOpenExternalLinks(true);
    set_icon_btn();
    start();
    trackView = new TrackView(ui, this, &radioList->stationImages());
    m_resolverService = new music_data::ResolverService(new music_data::MusicBrainzClient,
                                                        new music_data::CoverArtClient,
                                                        new music_data::ExternalLinksClient, this);
    ui->license->hide();

    connect(ui->infoApp, &QPushButton::clicked, this, &MainWindow::info);
    connect(radioList,
            &RadioList::trackTitleReceived,
            m_resolverService,
            &music_data::ResolverService::resolveTrack);
    connect(m_resolverService,
            &music_data::ResolverService::trackReady,
            trackView,
            &TrackView::setTrack);
    connect(&radioList->playback(), &PlaybackController::sessionChanged,
            m_resolverService, &music_data::ResolverService::cancel);
    connect(m_resolverService, &music_data::ResolverService::cleared, trackView, &TrackView::clear);
    connect(ui->centralwidget, &QObject::destroyed, m_resolverService, &music_data::ResolverService::cancel);
    connect(&m_theme, &Theme::themeChanged, radioList, &RadioList::updateThemeAppearance);
    connect(&m_theme, &Theme::themeChanged, &country, &Country::updateThemeAppearance);
}

MainWindow::~MainWindow()
{
    radioList->playback().stop();
    delete trayIcon;
    delete radioList;
    delete m_resolverService;
    delete trackView;
    delete radioExplorer;
    delete ui;
}

void MainWindow::set_icon_btn()
{
    ui->stop->setIcon(QIcon(":/images/img/stop30.png"));
    ui->previous->setIcon(QIcon(":/images/img/prev30.png"));
    ui->playPause->setIcon(QIcon(":/images/img/play30.png"));
    ui->next->setIcon(QIcon(":/images/img/next30.png"));
    ui->volume->setIcon(QIcon(":/images/img/audioplay.png"));
    ui->favorite->setIcon(QIcon(":/images/img/bookmark-empty.png"));
    ui->record->setIcon(QIcon(":/images/img/red-save-32.png"));
}

void MainWindow::start()
{
    radioExplorer = new RadioExplorer(ui);
    favoriteManager = new FavoriteManager{this};

    radioList = new RadioList(ui, favoriteManager);
    radioList->setMainWindow(this);
    radioList->setTheme(&m_theme);

    radioExplorer->createMenu();


    trayIcon = new TrayIcon(ui, *this);
    trayIcon->setRadioList(radioList);
    connect(radioList, &RadioList::catalogPlaylistLoaded, trayIcon, &TrayIcon::loadTrayList);

    appMenu = new Menu(ui, radioList, favoriteManager);

    country.setData(ui, radioList);
    country.setTheme(&m_theme);

    radioList->loadAllData();
    country.load();
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);

    if (height() < 140 && ui->splitter->isVisible()) {
        ui->splitter->hide();
        ui->serachInput->hide();

    } else if (height() >= 140 && !ui->splitter->isVisible()) {
        ui->splitter->show();
        ui->serachInput->show();
    }
    qDebug() << ui->centralwidget->geometry().height();
}

void MainWindow::info()
{
    QFile file(":/src/files/license.txt");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        QString licenseText = in.readAll();
        licenseLabel->setText(licenseText);
        file.close();
    }

    licenseDialog->show();
}

Ui::MainWindow *MainWindow::getUi() const
{
    return ui;
}

void MainWindow::infoDialogApp()
{
    licenseDialog = new QDialog(this);
    licenseDialog->setWindowTitle("About");

    scrollArea = new QScrollArea(licenseDialog);
    scrollArea->setWidgetResizable(true);

    QWidget *scrollAreaWidget = new QWidget;
    scrollArea->setWidget(scrollAreaWidget);

    licenseLabel = new QLabel;
    licenseLabel->setWordWrap(true);

    aboutTuneScape = new QLabel;
    aboutTuneScape->setText(
        "<p style=\"font-size: 18pt; line-height: 1.5;\">TuneScape 6.0.0</p>"
        "This is a free and open-source online radio player based on the "
        "<a href=\"https://www.radio-browser.info\">Radio Browser service</a>."
        "<br><br>"
        "This is the beta version. If you want to help develop this app, visit "
        "<a href=\"https://github.com/grzesiekkedzior/TuneScape\"><u>TuneScape</u></a> "
        "or send me an email at <b>grzesiekkedzior@gmail.com</b>."
        "<br><br>"
        "If you enjoy using TuneScape and would like to support its development, "
        "you can make a donation via "
        "<a href=\"https://www.paypal.com/donate/?hosted_button_id=MW4VMJ8YHSZF2\">"
        "<u><b>PayPal</b></u></a>.");
    aboutTuneScape->setOpenExternalLinks(true);
    aboutTuneScape->setWordWrap(true);

    QVBoxLayout *scrollAreaLayout = new QVBoxLayout(scrollAreaWidget);
    scrollAreaLayout->addWidget(aboutTuneScape);
    scrollAreaLayout->addWidget(licenseLabel);

    QVBoxLayout *dialogLayout = new QVBoxLayout(licenseDialog);
    dialogLayout->addWidget(scrollArea);
}
