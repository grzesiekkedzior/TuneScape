#include "include/radiolist.h"
#include <QFile>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonObject>
#include <QScrollBar>
#include "qpainter.h"

RadioList::RadioList(QObject *parent)
    : QObject{parent}
{}

RadioList::RadioList(Ui::MainWindow *ui)
    : ui(ui)
    , model(new QStandardItemModel(this))
{
    jsonListProcesor.setUi(ui);
    jsonListProcesor.setRadioList(this);
    radioInfo = new RadioInfo(ui);
    flowLayout = new FlowLayout(ui->iconTiles);
    streamRecorder->setUI(ui);
    iceCastXmlData = new IceCastXmlData(ui);
    iceCastXmlData->setJsonListProcessor(jsonListProcesor);
    iceCastXmlData->setRadioAudioManager(radioManager);
    iceCastXmlData->setRadioList(this);
    iceCastXmlData->setRadioInfo(radioInfo);
    iceCastXmlData->makeShareStreamRecorder(streamRecorder);
    ui->playPause->setShortcut(QKeySequence(Qt::Key_Space));

    connect(ui->treeView, &QTreeView::clicked, this, &RadioList::onTreeViewItemClicked);
    // for list
    //    connect(ui->tableView->verticalScrollBar(),
    //            &QScrollBar::valueChanged,
    //            this,
    //            &RadioList::loadMoreStationsIfNeeded);
    //    // for icons
    //    connect(ui->scrollArea->verticalScrollBar(),
    //            &QScrollBar::valueChanged,
    //            this,
    //            &RadioList::loadMoreStationsIfNeeded);
    //connect(ui->tableView, &QTableView::doubleClicked, this, &RadioList::onTableViewDoubleClicked);
    connect(ui->tableView, &QTableView::doubleClicked, this, &RadioList::setRadioImage);
    connect(ui->tableView, &QTableView::activated, this, &RadioList::setRadioImage);

    connect(ui->playPause, &QPushButton::clicked, this, &RadioList::onPlayPauseButtonCliced);
    connect(ui->next, &QPushButton::clicked, this, &RadioList::onNextButtonClicked);
    connect(ui->previous, &QPushButton::clicked, this, &RadioList::onPrevButtonClicked);
    connect(ui->stop, &QPushButton::clicked, this, &RadioList::onStopButtonClicked);
    connect(ui->tableView, &QTableView::clicked, this, &RadioList::onTableViewClicked);
    connect(ui->tableView, &QTableView::activated, this, &RadioList::tableViewActivated);

    connect(ui->horizontalVolumeSlider, &QSlider::sliderMoved, this, &RadioList::sliderMoved);
    connect(ui->horizontalVolumeSlider, &QSlider::valueChanged, this, &RadioList::sliderMoved);
    connect(miniPlayer.getMui()->dial, &QDial::sliderMoved, this, &RadioList::sliderMoved);
    connect(miniPlayer.getMui()->dial, &QDial::valueChanged, this, &RadioList::sliderMoved);

    connect(&streamReader, &StreamReader::dataReceived, this, &RadioList::handleDataReceived);
    //connect(ui->record, &QPushButton::clicked, this, &RadioList::setMp3FileName);
    connect(ui->record, &QPushButton::clicked, this, &RadioList::startStopRecord);
    connect(ui->serachInput, &QLineEdit::returnPressed, this, &RadioList::searchStations);
    connect(ui->favorite, &QPushButton::clicked, this, &RadioList::addRadioToFavorite);
    connect(this,
            &RadioList::playIconButtonDoubleClicked,
            this,
            &RadioList::handleIconPlayButtonDoubleClick);
    connect(this, &RadioList::allIconsLoaded, this, &RadioList::onAllIconsLoaded);
    connect(ui->themeButton, &QPushButton::clicked, this, &RadioList::setDarkMode);
    connect(ui->minplr, &QPushButton::clicked, this, &RadioList::showMiniplayer);
    connect(miniPlayer.getMui()->maxWindow, &QPushButton::clicked, this, &RadioList::maximizeWindow);

    header = ui->tableView->horizontalHeader();
    headers << STATION << COUNTRY << GENRE << HOMEPAGE;
    header->setSectionResizeMode(QHeaderView::Interactive);
    model->setHorizontalHeaderLabels(headers);
    ui->tableView->verticalHeader()->setDefaultSectionSize(ui->tableView->fontMetrics().height()
                                                           + 2);

    // audioProcessor.setFixedSize(97, 28);
    // audioProcessor.setPlayer(radioManager.getMediaPlayer());
    // audioProcessor.show();

    // ui->fft->addWidget(&audioProcessor);
    audioProcessor.setUi(ui);
    audioProcessor.setPlayer(radioManager.getMediaPlayer());
    miniPlayer.setUi(ui);
    miniPlayer.setRadioList(this);
}

void RadioList::clearFlowLayout()
{
    // I think here must be better solution!!!
    //    for (int i = 0; i < flowLayout->count(); i++) {
    //        QLayoutItem *item = flowLayout->takeAt(i);
    //        if (item) {
    //            flowLayout->removeItem(item);
    //            delete item->widget();
    //            delete item;
    //        }
    //    }
    QLayoutItem *item;
    while ((item = flowLayout->takeAt(0)) != nullptr) {
        while (QWidget *widget = item->widget()) {
            delete widget;
        }
        delete item;
    }
}

void RadioList::clearAll()
{
    progressLoading = 1;
    clearFlowLayout();
    networkReplies.clear();
    buttonCache.clear();
}

void RadioList::clearIconLabelColor()
{
    for (QWidget *buttonContainer : buttonCache) {
        if (buttonContainer) {
            QLabel *label = buttonContainer->findChild<QLabel *>();
            if (label) {
                // Reset to normal style here
                label->setStyleSheet(""); // This will clear any existing style
            }
        }
    }
    isIconFlowlayoutFull = false;
}

void RadioList::markIconPlayingStation(int radioNumber)
{
    QWidget *buttonContainer = nullptr;
    QLabel *label = nullptr;
    clearIconLabelColor();

    qDebug() << "nullptr " << radioNumber;
    qDebug() << "flowLayout->count() " << flowLayout->count();
    qDebug() << "allIconsAddresses.size() " << jsonListProcesor.getIconAddresses().size();
    if (buttonCache.size() >= radioNumber
        && flowLayout->count() == jsonListProcesor.getIconAddresses().size()) {
        buttonContainer = buttonCache.at(radioNumber);
        label = buttonContainer->findChild<QLabel *>();
    }

    if (label) {
        //if (isDarkMode) {
        label->setStyleSheet("background-color: #deffdf; color: black; font-weight: bold;");
        //} else {
        //label->setStyleSheet("background-color: #deffdf; color: white; font-weight: bold;");
        //}
    } else {
        qDebug() << "All icons are not loaded";
    }
}

void RadioList::setRawDarkRadioImage()
{
    ui->infoLabel->setPixmap(QPixmap(":/images/img/radiodark-10-96.png"));
    ui->radioIcon->setPixmap(QPixmap(":/images/img/radiodark-10-96.png"));
    miniPlayer.getMui()->radioImage->setPixmap(QPixmap(":/images/img/radiodark-10-96.png"));
}

void RadioList::showMiniplayer()
{
    miniPlayer.showMiniPlayer();
    int value = ui->horizontalVolumeSlider->value();
    miniPlayer.getMui()->dial->setValue(value);
    mainWindow->hide();
}

void RadioList::maximizeWindow()
{
    miniPlayer.window()->close();
    mainWindow->show();
    int value = miniPlayer.getMui()->dial->value();
    ui->horizontalVolumeSlider->setValue(value);
}

void RadioList::setRawRadioImage()
{
    ui->infoLabel->setPixmap(QPixmap(":/images/img/radio-10-96.png"));
    ui->radioIcon->setPixmap(QPixmap(":/images/img/radio-10-96.png"));
    miniPlayer.getMui()->radioImage->setPixmap(QPixmap(":/images/img/radio-10-96.png"));
}

void RadioList::setDarkMode()
{
    // if not play !radioManager.getMediaPlayer()->isPlaying()
    // if play
    isDarkMode = (isDarkMode) ? false : true;
    if (isDarkMode) {
        ui->tableView->setAlternatingRowColors(false);
        ui->icecastTable->setAlternatingRowColors(false);
        clearTableViewColor();
        iceCastXmlData->clearTableViewColor();
        if (!isStopClicked)
            setIndexColor();
        iceCastXmlData->setIndexColor(iceCastXmlData->getIndexPlayingStation());
        if (!radioManager.getMediaPlayer()->isPlaying())
            setRawDarkRadioImage();
        if (radioInfo->getInfoData().favicon == "")
            setRawDarkRadioImage();

    } else {
        ui->tableView->setAlternatingRowColors(true);
        ui->icecastTable->setAlternatingRowColors(true);
        clearTableViewColor();
        iceCastXmlData->clearTableViewColor();
        if (!isStopClicked)
            setIndexColor();
        iceCastXmlData->setIndexColor(iceCastXmlData->getIndexPlayingStation());

        if (!radioManager.getMediaPlayer()->isPlaying())
            setRawRadioImage();
        if (radioInfo->getInfoData().favicon == "")
            setRawRadioImage();
    }

    loadRadioIconList();
    if (radioIndexNumber != -1)
        markIconPlayingStation(radioIndexNumber);
}

void RadioList::isDark() {}

void RadioList::setMp3FileName()
{
    if (isPlaying) {
        QString title = ui->tableWidget->item(0, 1)->text();
        QString extention = ui->tableWidget->item(5, 1)->text().toLower();
        extention.replace("+", "");

        streamRecorder->setFileName(title, extention);

    } else if (iceCastXmlData->getPlaying() || iceCastXmlData->getIsFavoritePlaying()) {
        QString iceCastTitle = ui->icecastTable->item(iceCastXmlData->getCurrentPlayingStation(), 0)
                                   ->text();
        QString iceCastExtention = ui->icecastTable
                                       ->item(iceCastXmlData->getCurrentPlayingStation(), 2)
                                       ->text()
                                       .toLower();
        iceCastExtention.erase(iceCastExtention.constBegin(), iceCastExtention.constBegin() + 6);
        iceCastExtention.replace("+", "");
        qDebug() << iceCastExtention;
        streamRecorder->setFileName(iceCastTitle, iceCastExtention);
    } else {
    }
}

void RadioList::onAllIconsLoaded()
{
    if (currentPlayListPlaying == currentPlaylistIndex && isIconFlowlayoutFull) {
        if (getIsPlaying()) {
            qDebug() << "Color" << iceCastXmlData->getPlaying();
            qDebug() << "radioIndex: " << radioIndexNumber;
            if (treeItem == "Search")
                return;
            if (radioIndexNumber >= buttonCache.size())
                return;
            QWidget *buttonContainer = buttonCache.at(radioIndexNumber);
            if (buttonContainer) {
                QLabel *label = buttonContainer->findChild<QLabel *>();
                if (label) {
                    label->setStyleSheet(
                        "background-color: #deffdf; color: black; font-weight: bold;");
                } else {
                    // handle the case when label is not found
                    // todo
                }
            }
        }
    }
}

bool RadioList::getIsStopClicked() const
{
    return isStopClicked;
}

void RadioList::setIsStopClicked(bool newIsStopClicked)
{
    isStopClicked = newIsStopClicked;
}

RadioInfo *RadioList::getRadioInfo() const
{
    return radioInfo;
}

QMainWindow *RadioList::getMainWindow() const
{
    return mainWindow;
}

void RadioList::setMainWindow(QMainWindow *newMainWindow)
{
    mainWindow = newMainWindow;
}

void RadioList::setFavoriteLibrary()
{
    QModelIndex radioBrowserIndex = ui->treeView->model()->index(0, 0);
    QModelIndex favoriteIndex = ui->treeView->model()->index(3, 0, radioBrowserIndex);
    qDebug() << "index" << favoriteIndex.data().toString();
    if (favoriteIndex.isValid()) {
        onTreeViewItemClicked(favoriteIndex);
        ui->treeView->selectionModel()->select(favoriteIndex, QItemSelectionModel::Select);

    } else {
        qDebug() << "Error";
    }
}

IceCastXmlData *RadioList::getIceCastXmlData() const
{
    return iceCastXmlData;
}

bool RadioList::getIsSearchTablelDoubleCliced() const
{
    return isSearchTablelDoubleCliced;
}

void RadioList::setIsSearchTablelDoubleCliced(bool newIsSearchTablelDoubleCliced)
{
    isSearchTablelDoubleCliced = newIsSearchTablelDoubleCliced;
}

QString RadioList::getTreeItem() const
{
    return treeItem;
}

int RadioList::getRadioIndexNumber() const
{
    return radioIndexNumber;
}

QString RadioList::getItem() const
{
    return item;
}

void RadioList::setIsTreeClicked(bool newIsTreeClicked)
{
    isTreeClicked = newIsTreeClicked;
}

QVector<QVector<TableRow> > RadioList::getAllTableRows() const
{
    return allTableRows;
}

void RadioList::onTrayViewItemClicked(const QModelIndex &index)
{
    if (index.isValid()) {
        onTreeViewItemClicked(index);
    } else {
        qDebug() << "Wrong index.";
    }
}

void RadioList::onTrayClickedandPlay(const QModelIndex &index)
{
    onTableViewDoubleClicked(index);
}

void RadioList::setTrayRadioImage(const QModelIndex &index)
{
    setRadioImage(index);
}

JsonListProcessor *RadioList::getJsonListProcessor()
{
    return &jsonListProcesor;
}

bool RadioList::getIsBrowseStationLoaded() const
{
    return isBrowseStationLoaded;
}

void RadioList::setIsBrowseStationLoaded(bool newIsBrowseStationLoaded)
{
    isBrowseStationLoaded = newIsBrowseStationLoaded;
}

bool RadioList::getIsPause() const
{
    return isPause;
}

void RadioList::setIsPause(bool newIsPause)
{
    isPause = newIsPause;
}

QSharedPointer<StreamRecorder> RadioList::getStreamRecorder() const
{
    return streamRecorder;
}

bool RadioList::getIsDarkMode() const
{
    return isDarkMode;
}

void RadioList::setIsDarkMode(bool newIsDarkMode)
{
    isDarkMode = newIsDarkMode;
}

RadioAudioManager &RadioList::getRadioManager()
{
    return radioManager;
}

bool RadioList::getIsPlaying() const
{
    return isPlaying;
}

void RadioList::setIsPlaying(bool newIsPlaying)
{
    isPlaying = newIsPlaying;
}

void RadioList::handleIconPlayButtonDoubleClick(int radioNumber)
{
    QModelIndex index = model->index(radioNumber, 0);
    setRadioImage(index);
    onTableViewDoubleClicked(index);
    markIconPlayingStation(radioNumber);
}

void RadioList::loadRadioIconList()
{
    if (jsonListProcesor.isConnected) {
        clearAll();
        int dataSize = jsonListProcesor.getTableRows().size();
        qDebug() << "datasize: " << dataSize;
        ui->progressBar->setRange(0, dataSize);
        if (dataSize > 0)
            ui->progressBar->show();
        buttonCache.resize(dataSize, nullptr);
        if (!networkManager) {
            networkManager = new QNetworkAccessManager(this);
        }

        // Load buttons with empty icons first
        for (int row = 0; row < dataSize; ++row) {
            // Without this condition app will crash?! Some check analyzer suggest that it's unnecessary
            if (dataSize > row) {
                addEmptyIconButton(row);
            }
        }

        // Load icons from the internet
        for (int row = 0; row < dataSize; ++row) {
            if (dataSize > row) {
                QString imageUrl = jsonListProcesor.getIconAddresses().at(row);
                QNetworkRequest request(imageUrl);
                QNetworkReply *reply = networkManager->get(request);
                networkReplies.append(reply);

                connect(reply, &QNetworkReply::finished, [=]() { handleNetworkReply(reply, row); });
            }
        }
    }
}

void RadioList::addEmptyIconButton(int row)
{
    QWidget *itemContainer = new QWidget;
    QVBoxLayout *itemLayout = new QVBoxLayout(itemContainer);
    QPushButton *button = new QPushButton; // Allocate on the heap
    button->setFixedSize(120, 120);
    QString description = jsonListProcesor.getTableRows().at(row).station;
    QLabel *label = new QLabel(description); // Allocate on the heap
    label->setFixedWidth(120);
    label->setWordWrap(true);
    label->setAlignment(Qt::AlignCenter);

    itemLayout->addWidget(button);
    itemLayout->addWidget(label);

    // For now double clicked solution
    connect(button, &QPushButton::clicked, this, [=]() {
        // Static to prevent variable state loss!!!
        static QTimer timer;             // Timer to detect double-click
        static int lastClickedRow = -1;  // Track last clicked row
        static bool singleClick = false; // Track single click

        if (singleClick && lastClickedRow == row) {
            // Double-click detected
            emit playIconButtonDoubleClicked(row);
            singleClick = false;
            timer.stop(); // Stop the timer
        } else {
            singleClick = true;
            lastClickedRow = row;
            // Start the timer to detect double-click
            timer.singleShot(QApplication::doubleClickInterval(), [=]() { singleClick = false; });
        }
    });

    QString radioIcon = "";
    if (isDarkMode)
        radioIcon = ":/images/img/radiodark-10-96.png";
    else
        radioIcon = ":/images/img/radio-10-96.png";
    QPixmap originalPixmap(radioIcon);
    QSize buttonSize(120, 120);
    QPixmap pixmap(buttonSize);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.drawPixmap(10, 5, originalPixmap);

    button->setIcon(QIcon(pixmap));
    button->setIconSize(buttonSize);

    if (row < buttonCache.size())
        buttonCache[row] = itemContainer;

    if (!buttonCache.contains(nullptr)) {
        for (QWidget *button : buttonCache) {
            flowLayout->addWidget(button);
        }
        isIconFlowlayoutFull = true;
        ui->progressBar->hide();
        emit allIconsLoaded();
    } else {
        ++progressLoading;
        ui->progressBar->setValue(progressLoading);
    }
}

void RadioList::handleNetworkReply(QNetworkReply *reply, int row)
{
    if (row >= buttonCache.size()) {
        reply->deleteLater();
        return;
    }

    QWidget *itemContainer = buttonCache[row];
    if (!itemContainer) {
        reply->deleteLater();
        return;
    }

    QPushButton *button = qobject_cast<QPushButton *>(itemContainer->layout()->itemAt(0)->widget());
    if (!button) {
        reply->deleteLater();
        return;
    }

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray imageData = reply->readAll();
        QPixmap pixmap;
        pixmap.loadFromData(imageData);
        QSize buttonSize = button->size();

        pixmap = pixmap.scaled(buttonSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        button->setIcon(QIcon(pixmap));
        button->setIconSize(buttonSize);
    } else {
        // Handle error...
    }

    reply->deleteLater();
}

void RadioList::loadRadioList()
{
    int rowCount = model->rowCount();
    qDebug() << "loadRadioList" << treeItem;
    if (rowCount > 0 && treeItem != "Search") {
        model->removeRows(0, rowCount);
    }
    model->setHorizontalHeaderLabels(headers);
    int dataSize = jsonListProcesor.getTableRows().size();
    //int batchSize = 50;
    //    for (int row = 0; row < qMin(loadedStationsCount + batchSize, dataSize);
    //         ++row)
    for (int row = 0; row < dataSize; ++row) {
        QList<QStandardItem *> rowItems;
        rowItems.append(new QStandardItem(jsonListProcesor.getTableRows().at(row).station));
        rowItems.append(new QStandardItem(jsonListProcesor.getTableRows().at(row).country));
        rowItems.append(new QStandardItem(jsonListProcesor.getTableRows().at(row).genre));
        rowItems.append(new QStandardItem(jsonListProcesor.getTableRows().at(row).stationUrl));
        model->appendRow(rowItems);
    }

    ui->tableView->setModel(model);
    //this->treeItem = "Search";
    //ui->tableView->resizeRowsToContents();
    //loadedStationsCount += batchSize;
}

void RadioList::setTopListOnStart()
{
    QModelIndex libraryIndex = ui->treeView->model()->index(0, 0);
    QModelIndex topIndex = ui->treeView->model()->index(0, 0, libraryIndex);
    onTreeViewItemClicked(topIndex);
    ui->treeView->selectionModel()->select(topIndex, QItemSelectionModel::Select);
}

bool RadioList::isRadioAdded(const QString data, const QString playlist)
{
    QFile file(playlist);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine();
        if (line == data)
            return true;
    }

    return false;
}

void RadioList::removeRadio(const QString data, const QString playlist)
{
    QFile inputFile(playlist);
    if (!inputFile.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    QFile outputFile("temp_playlist.txt");
    if (!outputFile.open(QIODevice::WriteOnly | QIODevice::Text))
        return;

    QTextStream in(&inputFile);
    QTextStream out(&outputFile);

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed(); // Usuwanie znaków białych
        qDebug() << data;
        qDebug() << line;
        if (line != data) {
            out << line << "\n";
        }
    }

    inputFile.close();
    outputFile.close();

    // swap file
    if (QFile::remove(playlist) && QFile::rename("temp_playlist.txt", playlist)) {
        qDebug() << "Correct" << data;
    } else {
        qDebug() << "Error " << data;
    }
}

bool RadioList::isAddressExists(const QString address, const QString playlist)
{
    QFile file(playlist);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Error opening the file: " << file.errorString();
        return false;
    }

    QTextStream in(&file);
    const QString lowerCaseAddress = address.toLower();

    while (!in.atEnd()) {
        QString line = in.readLine();
        if (line.toLower().contains(lowerCaseAddress)) {
            file.close();
            qDebug() << "true";
            return true;
        }
    }

    file.close();
    return false;
}

void RadioList::setFavoriteStatons()
{
    QVector<TableRow> tableRows;
    QVector<QString> streamAddresses;
    QVector<QString> iconAddresses;
    // Read favorite radio from file
    QFile file(RADIO_BROWSER_PLAYLIST);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = in.readLine();
            // divide line and add to vectors
            QStringList fields = line.split(",");
            if (fields.size() >= 6) {
                TableRow row;
                row.station = fields[2];
                row.country = fields[3];
                row.genre = fields[4];
                row.stationUrl = fields[5];
                tableRows.append(row);
                streamAddresses.push_back(fields[1]);
                iconAddresses.push_back(fields[0]);
            }
        }
        file.close();
    }

    // I think here is better solution to do
    if (allTableRows.size() > Stations::FAVORITE) {
        allTableRows[Stations::FAVORITE] = tableRows;
        allStreamAddresses[Stations::FAVORITE] = streamAddresses;
        allIconsAddresses[Stations::FAVORITE] = iconAddresses;
    } else {
        allTableRows.push_back(tableRows);
        allStreamAddresses.push_back(streamAddresses);
        allIconsAddresses.push_back(iconAddresses);
    }
}

void RadioList::loadAllData()
{
    QStringList endpoints = {JSON_ENDPOINT_TOP,
                             //JSON_ENDPOINT_DISCOVER,
                             JSON_ENDPOINT_POPULAR,
                             JSON_ENDPOINT_NEW};

    for (const QString &endpoint : endpoints) {
        setVectorsOfStation(endpoint);
    }

    if (!iceCastXmlData->getIsStationsLoaded()) {
        iceCastXmlData->loadXmlAsync();
        clearTableViewColor();
        iceCastXmlData->clearTableViewColor();
    }

    setFavoriteStatons();
    if (!isDarkMode) {
        ui->tableView->setAlternatingRowColors(true);
        ui->icecastTable->setAlternatingRowColors(true);
    }

    setTopListOnStart();
    //loadRadioIconList();
}

void RadioList::setLoadedStationsCount(int num)
{
    this->loadedStationsCount = num;
}

void RadioList::loadMoreStationsIfNeeded()
{
    if (ui->tabRadioListWidget->currentIndex() == 0) {
        QScrollBar *scrollBar = ui->tableView->verticalScrollBar();
        int currentPosition = scrollBar->value();
        int maximumPosition = scrollBar->maximum();
        qDebug() << "Scrollup";
        if (currentPosition >= maximumPosition * 0.8) {
            loadRadioList();
        }
        if (currentPlayListPlaying == currentPlaylistIndex) {
            setIndexColor();
        }
    } else {
        QScrollBar *scrollIconBar = ui->scrollArea->verticalScrollBar();
        int currentIconPosition = scrollIconBar->value();
        int maximumIconPosition = scrollIconBar->maximum();
        qDebug() << "Scrolldown";
        if (currentIconPosition >= maximumIconPosition * 0.8) {
            loadRadioList();
        }
        if (currentPlayListPlaying == currentPlaylistIndex) {
            setIndexColor();
        }
    }
}

auto checkItem = [](const QString &item, const QString &target) { return item == target; };

void RadioList::setRadioListVectors(Stations s)
{
    jsonListProcesor.setTableRows(allTableRows[s]);
    jsonListProcesor.setStreamAddresses(allStreamAddresses[s]);
    jsonListProcesor.setIconAddresses(allIconsAddresses[s]);
}

void RadioList::onTreeViewItemClicked(const QModelIndex &index)
{
    isTreeClicked = true;
    item = index.data().toString();
    qDebug() << "onTreeViewItemClicked " << item << " ROW " << index.row();
    if (!checkItem(item, LIBRARY_TREE)) {
        if ((!checkItem(item, "Discover") && ui->tabRadioListWidget->currentIndex() == 2)
            || ui->tabRadioListWidget->currentIndex() == 3) {
            if (this->treeItem == "Search")
                this->treeItem = "";
            ui->tabRadioListWidget->setCurrentIndex(0);
        }
        if (checkItem(item, "Top")) {
            if (this->treeItem == "Search")
                this->treeItem = "";
            setRadioListVectors(Stations::TOP);
            currentPlaylistIndex = Stations::TOP;
        } else if (checkItem(item, "Popular")) {
            if (this->treeItem == "Search")
                this->treeItem = "";
            setRadioListVectors(Stations::POPULAR);
            currentPlaylistIndex = Stations::POPULAR;
        } else if (checkItem(item, "New")) {
            if (this->treeItem == "Search")
                this->treeItem = "";
            setRadioListVectors(Stations::NEW);
            currentPlaylistIndex = Stations::NEW;
        } else if (checkItem(item, "Favorite")) {
            if (this->treeItem == "Search")
                this->treeItem = "";
            setRadioListVectors(Stations::FAVORITE);
            currentPlaylistIndex = Stations::FAVORITE;
            qDebug() << "HELLO";
        } else if (checkItem(item, "Discover")) {
            if (this->treeItem == "Search")
                this->treeItem = "";
            ui->tabRadioListWidget->setCurrentIndex(2);
            // TODO connection checker
            iceCastXmlData->setIsFavoriteOnTreeCliced(false);
            iceCastXmlData->loadDiscoveryStations();
            qDebug() << "Discover";
        } else if (checkItem(item, "Ice-Favorite")) {
            if (this->treeItem == "Search")
                this->treeItem = "";
            ui->tabRadioListWidget->setCurrentIndex(2);
            qDebug() << "Ice-Cast favorite";
            iceCastXmlData->setIsFavoriteOnTreeCliced(true);
            iceCastXmlData->loadFavoriteIceCastStations();
            //iceCastXmlData->setFavoriteList();
        }
        if (jsonListProcesor.checkInternetConnection()) {
            loadedStationsCount = 0;
            model->clear();
            loadRadioList();
            loadRadioIconList();
        }

        if (currentPlayListPlaying == currentPlaylistIndex) {
            if (getIsPlaying())
                setIndexColor();
        } else {
            // Clear the color-marked station when changing tree items.
            if (customColor)
                customColor->clearRowColor();
        }
    }
}

void RadioList::getSongTitle(const QString &url)
{
    streamReader.startStreaming(url);
}

void RadioList::checkIsRadioOnPlaylist()
{
    if (isAddressExists(currentRadioPlayingAddress, RADIO_BROWSER_PLAYLIST)) {
        ui->favorite->setIcon(QIcon(":/images/img/bookmark-file.png"));
    } else {
        ui->favorite->setIcon(QIcon(":/images/img/bookmark-empty.png"));
    }
}

void RadioList::onInternetConnectionRestored()
{
    allIconsAddresses.clear();
    allStreamAddresses.clear();
    allTableRows.clear();
    //this->onStopButtonClicked();
    message.setText("The connection has been restored. Select a station.");
    message.setIcon(QMessageBox::Information);
    message.setWindowIcon(QIcon(":/images/img/radio30.png"));
    clearTableViewColor();
    ui->treeView->clearSelection();
    iceCastXmlData->clearTableViewColor();
    setIndexColor();
    loadAllData();
    if (getMainWindow()->isHidden())
        getMainWindow()->show();
    message.show();
}

void RadioList::playStream(int radioNumber)
{
    radioIndexCurrentPlaying = radioNumber;
    radioPlaylistCurrentPlaying = currentPlaylistIndex;
    currentRadioPlayingAddress = jsonListProcesor.getStreamAddresses(radioNumber);
    checkIsRadioOnPlaylist();
    getSongTitle(currentRadioPlayingAddress);
    QUrl streamUrl(currentRadioPlayingAddress);
    radioManager.loadStream(streamUrl);
    radioManager.playStream();
    audioProcessor.start(currentRadioPlayingAddress);
}

void RadioList::setIndexColor()
{
    customColor.reset(new CustomColorDelegate(radioIndexNumber, QColor(222, 255, 223), this));
    ui->tableView->setItemDelegate(customColor.get());
}

void RadioList::sliderMoved(int move)
{
    radioManager.setVolume(move);
    if (move == 0)
        ui->volume->setIcon(QIcon(":/images/img/audiostop.png"));
    if (move > 0 && move < 5)
        ui->volume->setIcon(QIcon(":/images/img/audioplay.png"));
}

void RadioList::setRadioImage(const QModelIndex &index)
{
    if (!(jsonListProcesor.isConnected))
        return;
    QEventLoop loop;
    if (jsonListProcesor.getIconAddresses().size() == 0)
        return;
    QUrl imageUrl(jsonListProcesor.getIconAddresses(index.row()));

    QNetworkAccessManager manager;

    QNetworkRequest request(imageUrl);
    QNetworkReply *reply = manager.get(request);

    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() == QNetworkReply::NoError) {
        QVariant contentType = reply->header(QNetworkRequest::ContentTypeHeader);
        QString contentTypeString = contentType.toString();

        if (contentTypeString.startsWith("image/")) {
            QByteArray imageData = reply->readAll();
            QPixmap pixmap;
            pixmap.loadFromData(imageData);

            if (!pixmap.isNull()) {
                QSize imageSize(120, 120);
                pixmap = pixmap.scaled(imageSize, Qt::KeepAspectRatio);
                ui->infoLabel->setPixmap(pixmap);
                miniPlayer.getMui()->radioImage->setPixmap(ui->infoLabel->pixmap());
                ui->infoLabel->show();
            }
        } else {
            if (isDarkMode) {
                ui->infoLabel->setPixmap(QPixmap(":/images/img/radiodark-10-96.png"));
                miniPlayer.getMui()->radioImage->setPixmap(
                    QPixmap(":/images/img/radiodark-10-96.png"));
            } else {
                ui->infoLabel->setPixmap(QPixmap(":/images/img/radio-10-96.png"));
                miniPlayer.getMui()->radioImage->setPixmap(QPixmap(":/images/img/radio-10-96.png"));
            }
            ui->infoLabel->show();
        }
    } else {
        qDebug() << "Error:" << reply->errorString();
        if (isDarkMode) {
            ui->infoLabel->setPixmap(QPixmap(":/images/img/radiodark-10-96.png"));
            miniPlayer.getMui()->radioImage->setPixmap(QPixmap(":/images/img/radiodark-10-96.png"));
        } else {
            ui->infoLabel->setPixmap(QPixmap(":/images/img/radio-10-96.png"));
            miniPlayer.getMui()->radioImage->setPixmap(QPixmap(":/images/img/radio-10-96.png"));
        }
        ui->infoLabel->show();
    }

    reply->deleteLater();
}

void RadioList::onTableViewDoubleClicked(const QModelIndex &index)
{
    if (jsonListProcesor.checkInternetConnection()) {
        radioIndexNumber = index.row();
        currentStationIndex = index.row();
        currentPlayListPlaying = currentPlaylistIndex;

        // change bookmark
        playStream(radioIndexNumber);
        clearTableViewColor();
        iceCastXmlData->clearTableViewColor();
        country.clearTableColor();
        country.setIsPlaying(false);
        setIndexColor();
        markIconPlayingStation(index.row());

        setIsPlaying(true);
        setIsBrowseStationLoaded(true);
        if (getIsPlaying()) {
            ui->playPause->setIcon(QIcon(":/images/img/pause30.png"));
            miniPlayer.getMui()->play->setIcon(QIcon(":/images/img/pause30.png"));
        }

        isStopClicked = false;
        ui->infoData->clear();
        if (isPlaying) {
            radioInfo->loadEndpoint(jsonListProcesor.getTableRows().at(currentStationIndex).station);
            radioInfo->processInfoJsonQuery();
            radioInfo->setDataOnTable();
        }
        radioIndexNumber = index.row();
        setIndexColor();
        iceCastXmlData->setPlaying(false);
    }

    if (streamRecorder->getIsRecording()) {
        streamRecorder->stopRecording();
        streamRecorder->setIsRecording(false);
    }

    if (this->treeItem == "Search")
        setIsSearchTablelDoubleCliced(true);
    else
        setIsSearchTablelDoubleCliced(false);
}

void RadioList::onPlayPauseButtonCliced()
{
    if (!isTreeClicked || !jsonListProcesor.isConnected) {
        return;
    }
    if (getIsPause())
        setIsPause(false);
    else
        setIsPause(true);
    if (ui->tabRadioListWidget->currentIndex() == 2 && !iceCastXmlData->getPlaying()
        && iceCastXmlData->getCurrentPlayingStation() == -1 && !getIsBrowseStationLoaded()
        && iceCastXmlData->getIsDownloadFinish()) {
        qDebug() << "1";
        QModelIndex newIndex = ui->tableView->model()->index(0, 0);
        iceCastXmlData->setPlaying(true);
        iceCastXmlData->playStreamOnStart(newIndex);
    } else if (iceCastXmlData->getPlaying() && !getIsBrowseStationLoaded()) {
        qDebug() << "2";
        iceCastXmlData->setPlaying(false);
        iceCastXmlData->playPauseIcon();
        radioManager.stopStream();
        audioProcessor.getUpdateTimer()->stop();
    } else if (!iceCastXmlData->getPlaying() && iceCastXmlData->getCurrentPlayingStation() != -1
               && !getIsBrowseStationLoaded()) {
        qDebug() << "3";
        radioManager.playStream();
        audioProcessor.getUpdateTimer()->start();
        iceCastXmlData->setPlaying(true);
        iceCastXmlData->playPauseIcon();

        //radio browser station below
    } else {
        if (radioManager.getMediaPlayer()->isPlaying()) {
            radioManager.stopStream();
            audioProcessor.getUpdateTimer()->stop();
            setIsPlaying(false);
        } else if (currentRadioPlayingAddress != ""
                   && radioManager.getMediaPlayer()->isAvailable()) {
            radioManager.playStream();
            audioProcessor.getUpdateTimer()->start();
            setIsPlaying(true);

        } else if (currentRadioPlayingAddress.isEmpty()
                   && !jsonListProcesor.getTableRows().isEmpty()) {
            currentPlayListPlaying = currentPlaylistIndex;
            playStream(radioIndexNumber);
            radioInfo->loadEndpoint(jsonListProcesor.getTableRows().at(radioIndexNumber).station);
            radioInfo->processInfoJsonQuery();
            radioInfo->setDataOnTable();
            QModelIndex newIndex = ui->tableView->model()->index(0, 0);
            setRadioImage(newIndex);
            ui->radioIcon->setPixmap(ui->infoLabel->pixmap());
            /***************************************************/
            miniPlayer.getMui()->radioImage->setPixmap(ui->infoLabel->pixmap());
            /***************************************************/
            markIconPlayingStation(newIndex.row());
            setIsPlaying(true);
        } else if (!radioManager.getMediaPlayer()->isPlaying() && currentRadioPlayingAddress == ""
                   && ui->tableView->currentIndex().row() > 0) {
            playStream(radioIndexNumber);
            setIsPlaying(true);
        }

        if (currentPlayListPlaying == currentPlaylistIndex) {
            setIndexColor();
        }
        ui->playPause->setIcon(
            QIcon(getIsPlaying() ? ":/images/img/pause30.png" : ":/images/img/play30.png"));
        miniPlayer.getMui()->play->setIcon(
            QIcon(getIsPlaying() ? ":/images/img/pause30.png" : ":/images/img/play30.png"));
        setIsBrowseStationLoaded(true);
    }
    if (isStopClicked) {
        setRadioImage(model->index(0, 0));
        isStopClicked = false;
    }
}

// Not use for now
void RadioList::onNextButtonClicked()
{
    if (radioManager.getMediaPlayer()->isPlaying()
        && radioIndexNumber < jsonListProcesor.getTableRows().size() - 1) {
        ++radioIndexNumber;
        clearTableViewColor();
        setIndexColor();
        playStream(radioIndexNumber);
    }
}

// Not use for now
void RadioList::onPrevButtonClicked()
{
    if (radioManager.getMediaPlayer()->isPlaying() && radioIndexNumber > 0) {
        --radioIndexNumber;
        playStream(radioIndexNumber);

        clearTableViewColor();
        setIndexColor();
    }
}

void RadioList::clearTableViewColor()
{
    // for (int row = 0; row < model->rowCount(); ++row) {
    //     for (int column = 0; column < model->columnCount(); ++column) {
    //         QModelIndex index = model->index(row, column);
    //         if (isDarkMode) {
    //             model->setData(index, QColor(60, 60, 60), Qt::BackgroundRole);
    //             model->setData(index, QColor(Qt::white), Qt::ForegroundRole);
    //         } else {
    //             model->setData(index, QColor(Qt::white), Qt::BackgroundRole);
    //         }
    //         //model->setData(index, QColor(Qt::black), Qt::ForegroundRole);
    //     }
    // }
    if (customColor) {
        customColor->clearRowColor();
        ui->tableView->update();
    }
}

void RadioList::onStopButtonClicked()
{
    // TODO when internet connection is lost then stop button is unable to turn off music!!!
    // It can irritate some users but it is very rare case
    if (radioManager.getMediaPlayer()->isPlaying() && jsonListProcesor.isConnected) {
        isStopClicked = true;
        if (radioManager.getMediaPlayer()->isPlaying()) {
            ui->playPause->setIcon(QIcon(":/images/img/play30.png"));
            miniPlayer.getMui()->play->setIcon(QIcon(":/images/img/play30.png"));
            radioManager.stopStream();
            audioProcessor.stop();
            currentRadioPlayingAddress = "";
            if (isDarkMode) {
                ui->infoLabel->setPixmap(QPixmap(":/images/img/radiodark-10-96.png"));
                ui->radioIcon->setPixmap(QPixmap(":/images/img/radiodark-10-96.png"));
                miniPlayer.getMui()->radioImage->setPixmap(
                    QPixmap(":/images/img/radiodark-10-96.png"));
            } else {
                ui->infoLabel->setPixmap(QPixmap(":/images/img/radio-10-96.png"));
                ui->radioIcon->setPixmap(QPixmap(":/images/img/radio-10-96.png"));
                miniPlayer.getMui()->radioImage->setPixmap(QPixmap(":/images/img/radio-10-96.png"));
            }

            ui->infoLabel->show();
            ui->infoData->clear();
            QModelIndex newIndex = ui->tableView->model()->index(0, 0);
            ui->tableView->setCurrentIndex(newIndex);
        } else {
            ui->playPause->setIcon(QIcon(":/images/img/play30.png"));
            miniPlayer.getMui()->play->setIcon(QIcon(":/images/img/play30.png"));
            currentRadioPlayingAddress = "";
            QModelIndex newIndex = ui->tableView->model()->index(0, 0);
            ui->tableView->setCurrentIndex(newIndex);
        }
        radioIndexNumber = 0;
        clearTableViewColor();
        clearIconLabelColor();
        radioInfo->clearInfo();
        iceCastXmlData->clearTableViewColor();
        iceCastXmlData->setPlaying(false);
        iceCastXmlData->setCurrentPlayingStation(-1);
        setIsPlaying(false);
        setIsBrowseStationLoaded(false);
        if (streamRecorder->getIsRecording()) {
            streamRecorder->stopRecording();
            streamRecorder->setIsRecording(false);
        }
        country.clearTableColor();
    } else {
        return;
    }
}

// Not use now
void RadioList::onTableViewClicked(const QModelIndex &index)
{
    //this->radioIndexNumber = index.row();
    qDebug() << this->radioIndexNumber;
}

void RadioList::tableViewActivated(const QModelIndex &index)
{
    onTableViewDoubleClicked(index);
}

void RadioList::addRadioToFavorite()
{
    qDebug() << getIsPlaying();
    if (getIsPlaying() == false
        && (ui->tabRadioListWidget->currentIndex() == 2 || iceCastXmlData->getPlaying())) {
        if (radioManager.getMediaPlayer()->isPlaying()
            && iceCastXmlData->getCurrentPlayingStation()
                   < iceCastXmlData->getIceCastStationTableRows().size()) {
            QString station = iceCastXmlData
                                  ->getIceCastTableRow(iceCastXmlData->getCurrentPlayingStation())
                                  .station;

            if (isRadioAdded(station, ICECAST_PLAYLIST)) {
                removeRadio(station, ICECAST_PLAYLIST);
                ui->favorite->setIcon(QIcon(":/images/img/bookmark-empty.png"));
            } else if (!station.isEmpty()) {
                QFile file(ICECAST_PLAYLIST);

                if (!file.open(QIODevice::WriteOnly | QIODevice::Append)) {
                    qDebug() << "Error";
                    return;
                }

                QTextStream out(&file);
                out << station << "\n";
                file.close();
                ui->favorite->setIcon(QIcon(":/images/img/bookmark-file.png"));
                iceCastXmlData->addToFavoriteStations();
            }
            iceCastXmlData->setFavoriteStations();
            iceCastXmlData->clearTableViewColor();
        }
        if (iceCastXmlData->getIsFavoriteOnTreeCliced())
            iceCastXmlData->loadFavoriteIceCastStations();
        iceCastXmlData->setIndexColor(iceCastXmlData->getIndexPlayingStation());

    } else if (getIsPlaying()) {
        if (radioManager.getMediaPlayer()->isPlaying()) {
            if (radioPlaylistCurrentPlaying < allTableRows.size()
                && radioIndexCurrentPlaying < allTableRows[radioPlaylistCurrentPlaying].size()) {
                QString data
                    = allIconsAddresses[radioPlaylistCurrentPlaying].at(radioIndexCurrentPlaying)
                      + ","
                      + allStreamAddresses[radioPlaylistCurrentPlaying].at(radioIndexCurrentPlaying)
                      + ","
                      + allTableRows[radioPlaylistCurrentPlaying].at(radioIndexCurrentPlaying).station
                      + ","
                      + allTableRows[radioPlaylistCurrentPlaying].at(radioIndexCurrentPlaying).country
                      + ","
                      + allTableRows[radioPlaylistCurrentPlaying].at(radioIndexCurrentPlaying).genre
                      + ","
                      + allTableRows[radioPlaylistCurrentPlaying]
                            .at(radioIndexCurrentPlaying)
                            .stationUrl;

                if (isRadioAdded(data, RADIO_BROWSER_PLAYLIST)) {
                    qDebug() << "remove";
                    removeRadio(data, RADIO_BROWSER_PLAYLIST);
                    ui->favorite->setIcon(QIcon(":/images/img/bookmark-empty.png"));
                } else if (!data.isEmpty()) {
                    QFile file(RADIO_BROWSER_PLAYLIST);

                    if (!file.open(QIODevice::WriteOnly | QIODevice::Append)) {
                        qDebug() << "Error";
                        return;
                    }

                    QTextStream out(&file);
                    out << data << "\n";
                    file.close();
                    ui->favorite->setIcon(QIcon(":/images/img/bookmark-file.png"));
                }
            }
            setFavoriteStatons();
        }
    } else if (country.getIsPlaying()) {
        if (radioManager.getMediaPlayer()->isPlaying()) {
            QString data = country.dtoFavorite.icon + "," + country.dtoFavorite.stream + ","
                           + country.dtoFavorite.station + "," + country.dtoFavorite.country + ","
                           + country.dtoFavorite.genre + "," + country.dtoFavorite.stationUrl;

            if (isRadioAdded(data, RADIO_BROWSER_PLAYLIST)) {
                qDebug() << "remove";
                removeRadio(data, RADIO_BROWSER_PLAYLIST);
                ui->favorite->setIcon(QIcon(":/images/img/bookmark-empty.png"));
            } else if (!data.isEmpty()) {
                QFile file(RADIO_BROWSER_PLAYLIST);

                if (!file.open(QIODevice::WriteOnly | QIODevice::Append)) {
                    qDebug() << "Error";
                    return;
                }

                QTextStream out(&file);
                out << data << "\n";
                file.close();
                ui->favorite->setIcon(QIcon(":/images/img/bookmark-file.png"));
            }
        }
        setFavoriteStatons();
    }
}

void RadioList::handleDataReceived(const QString &data)
{
    QString metaData = data;
    int titleStart = metaData.indexOf("StreamTitle='");
    int titleEnd = metaData.indexOf("';", titleStart);

    if (titleStart != -1 && titleEnd != -1) {
        QString title = metaData.mid(titleStart + 13, titleEnd - (titleStart + 13));
        qDebug() << title;
        ui->infoData->clear();
        ui->infoData->setText(title);
        miniPlayer.getMui()->radioText->clear();
        miniPlayer.getMui()->radioText->setText(title);

        emit sendTitleToTray(title);
    }
    //metaData = "";
}

void RadioList::startStopRecord()
{
    if (radioManager.getMediaPlayer()->isPlaying() && jsonListProcesor.isConnected) {
        qDebug() << "Hello recording";
        if (!streamRecorder->getIsRecording()) {
            setMp3FileName();

            if (isPlaying)
                streamRecorder->loadCurrentAddress(currentRadioPlayingAddress);
            else
                streamRecorder->loadCurrentAddress(
                    iceCastXmlData->getIceCastStationTableRows()
                        .at(iceCastXmlData->getCurrentPlayingStation())
                        .listen_url);
            streamRecorder->startRecording();
            streamRecorder->setIsRecording(true);
        } else {
            qDebug() << "record false";
            streamRecorder->stopRecording();
            streamRecorder->setIsRecording(false);
        }
    }
}

void RadioList::setVectorsOfStation(const QString endpoint)
{
    jsonListProcesor.loadEndpoint(endpoint);
    jsonListProcesor.processJsonQuery();

    // Wait for processing data
    QCoreApplication::processEvents();

    QVector<TableRow> tableRows = jsonListProcesor.getTableRows();
    QVector<QString> streamAddresses = jsonListProcesor.getStreamAddresses();
    QVector<QString> iconAddresses = jsonListProcesor.getIconAddresses();
    allTableRows.push_back(tableRows);
    allStreamAddresses.push_back(streamAddresses);
    allIconsAddresses.push_back(iconAddresses);
}

void RadioList::searchStations()
{
    qDebug() << "Size list: " << allTableRows.size();
    // This is ugly but I dont change it to don't complicate code
    //**********************************************************
    if (allTableRows.size() > 4 && allIconsAddresses.size() > 4 && allStreamAddresses.size() > 4) {
        allTableRows.pop_back();
        allIconsAddresses.pop_back();
        allStreamAddresses.pop_back();
    }
    //**********************************************************
    int rowCount = model->rowCount();
    model->removeRows(0, rowCount);

    QString data = ui->serachInput->text();
    const QString endpoint = JSON_ENDPOINT_SEARCH + data;
    qDebug() << allTableRows.size();

    setVectorsOfStation(endpoint);

    setRadioListVectors(Stations::SEARCH);
    currentPlaylistIndex = Stations::SEARCH;

    if (jsonListProcesor.checkInternetConnection()) {
        loadedStationsCount = 0;
        loadRadioList();
        loadRadioIconList();
    }

    this->treeItem = "Search";
    this->item = "Search";
    ui->tabRadioListWidget->setCurrentIndex(0);
    this->clearIconLabelColor();
    this->clearTableViewColor();
}
