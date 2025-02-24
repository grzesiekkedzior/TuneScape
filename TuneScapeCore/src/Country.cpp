#include "include/Country.h"
#include "include/radiolist.h"
#include "qjsonarray.h"
#include "qjsonobject.h"

Country::Country() {}

Country::~Country() {}

void Country::setData(Ui::MainWindow *ui, RadioList *radioList)
{
    this->ui = ui;
    this->radioList = radioList;
    connect(ui->comboBox, &QComboBox::textActivated, this, &Country::searchCountry);
    connect(ui->tableOfCoutries, &QTableWidget::doubleClicked, this, &Country::onDoubleListClicked);
    connect(ui->tableOfCoutries, &QTableWidget::activated, this, &Country::onDoubleListClicked);
}

void Country::load()
{
    createHeaders();
    reply = setConnection(COUNTRY_ENDPOINT_NAME);
    createCountryArray(reply);
    loadCountriesToComboBox();
}

void Country::createHeaders()
{
    ui->tableOfCoutries->setColumnCount(4);
    ui->tableOfCoutries->setHorizontalHeaderLabels({"Station", "Country", "Genre", "Homepage"});
}

void Country::loadCountriesToComboBox()
{
    for (CountriesData s : countriesData) {
        ui->comboBox->addItem(s.countryName);
    }
}

QNetworkReply *Country::setConnection(QString endpoint)
{
    RadioStations radioStations(endpoint);
    return jsonListProcessor.checkAvailability(radioStations.getAddresses());
}

bool Country::createCountryArray(QNetworkReply *reply)
{
    if (reply) {
        doc = jsonListProcessor.createJasonDocument(reply);
        if (doc.isArray()) {
            QJsonArray stationsArray = doc.array();

            for (const QJsonValue &value : stationsArray) {
                QJsonObject countryObject = value.toObject();
                QString countryName = countryObject[COUNTRY_NAME].toString();
                QString countryIso = countryObject[COUNTRY_ISO].toString();
                QString countryCount = QString::number(countryObject[COUNTRY_COUNT].toInt());

                CountriesData countData;
                countData.countryName = countryName;
                countData.countryIso = countryIso;
                countData.countryCount = countryCount;

                countriesData.append(countData);
            }

            return true;
        }
    }
    if (reply)
        reply->deleteLater();
    return false;
}

bool Country::createTable(QNetworkReply *reply)
{
    tableRows.clear();
    streamAddresses.clear();
    ui->tableOfCoutries->setRowCount(0);

    if (reply)
        doc = jsonListProcessor.createJasonDocument(reply);

    if (doc.isArray()) {
        QJsonArray stationsArray = doc.array();

        for (const QJsonValue &value : stationsArray) {
            QJsonObject stationObject = value.toObject();
            QString stationName = stationObject[NAME].toString();
            QString genre = stationObject[GENRE].toString();
            QString country = stationObject[COUNTRY].toString();
            QString stationUrl = stationObject[URL].toString();

            stationName = stationName.trimmed().replace(QRegularExpression("^[\\s?_.-]+"), "");
            genre = genre.left(genre.indexOf(',')).trimmed();
            country = country.trimmed();
            stationUrl = stationUrl.trimmed();

            TableRow row;
            row.station = stationName;
            row.genre = genre;
            row.country = country;
            row.stationUrl = stationUrl;

            tableRows.append(row);

            QString streamUrl = stationObject[URL_RESOLVED].toString();
            QString iconUrl = stationObject[FAVICON].toString();
            this->streamAddresses.push_back(streamUrl);
            this->iconAddresses.push_back(iconUrl);
        }

        for (const auto &row : tableRows) {
            addRowToTable(row);
        }
        return true;
    }
    if (reply)
        reply->deleteLater();
    return false;
}

void Country::searchCountry(QString country)
{
    QString path = COUNTRY_ENDPOINT_SEARCH + country;
    RadioStations radioStations(path);
    reply = jsonListProcessor.checkAvailability(radioStations.getAddresses());
    if (reply)
        createTable(reply);
}

void Country::addRowToTable(const TableRow &row)
{
    int rowPosition = ui->tableOfCoutries->rowCount();
    ui->tableOfCoutries->insertRow(rowPosition);

    ui->tableOfCoutries->setItem(rowPosition, 0, new QTableWidgetItem(row.station));
    ui->tableOfCoutries->setItem(rowPosition, 1, new QTableWidgetItem(row.country));
    ui->tableOfCoutries->setItem(rowPosition, 2, new QTableWidgetItem(row.genre));
    ui->tableOfCoutries->setItem(rowPosition, 3, new QTableWidgetItem(row.stationUrl));
}

void Country::setIndexColor(const QModelIndex &index)
{
    customColor.reset(new CustomColorDelegate(index.row(), QColor(222, 255, 223), this));
    ui->tableOfCoutries->setItemDelegate(customColor.get());
}

void Country::createDtoFavorites(const QModelIndex &index, QString url)
{
    dtoFavorite.stream = url;
    dtoFavorite.icon = iconAddresses[index.row()];
    dtoFavorite.country = tableRows[index.row()].country;
    dtoFavorite.genre = tableRows[index.row()].genre;
    dtoFavorite.station = tableRows[index.row()].station;
    dtoFavorite.stationUrl = tableRows[index.row()].stationUrl;
}

void Country::onDoubleListClicked(const QModelIndex &index)
{
    if (radioList->getJsonListProcessor()->isConnected) {
        QString url = streamAddresses[index.row()];
        createDtoFavorites(index, url);

        radioList->getRadioManager().loadStream(url);
        audioProcessor.start(url);
        radioList->getRadioManager().playStream();
        radioList->getIceCastXmlData()->clearTableViewColor();
        setIndexColor(index);
        ui->infoData->clear();
        radioList->getRadioInfo()->clearInfo();
        radioList->clearTableViewColor();
        radioList->clearIconLabelColor();

        if (radioList->getIsDarkMode()) {
            ui->infoLabel->setPixmap(QPixmap(":/images/img/radiodark-10-96.png"));
            ui->radioIcon->setPixmap(QPixmap(":/images/img/radiodark-10-96.png"));
            miniPlayer.getMui()->radioImage->setPixmap(QPixmap(":/images/img/radio-10-96.png"));
        } else {
            ui->infoLabel->setPixmap(QPixmap(":/images/img/radio-10-96.png"));
            ui->radioIcon->setPixmap(QPixmap(":/images/img/radio-10-96.png"));
            miniPlayer.getMui()->radioImage->setPixmap(QPixmap(":/images/img/radio-10-96.png"));
        }
        setIsPlaying(true);
        if (isPlaying) {
            radioList->getRadioInfo()->loadEndpoint(tableRows[index.row()].station);
            radioList->getRadioInfo()->processInfoJsonQuery();
            setRadioImage(index);
            radioList->getRadioInfo()->setDataOnTable();
            checkIsOnPlaylist(index, url);
        }

        radioList->setIsStopClicked(false);
        radioList->getSongTitle(url);
        //check favourite todo
        setIsPlaying(true);
        radioList->setIsPlaying(false);
        radioList->getIceCastXmlData()->setPlaying(false);
        if (radioList->getStreamRecorder()->getIsRecording()) {
            radioList->getStreamRecorder()->stopRecording();
            radioList->getStreamRecorder()->setIsRecording(false);
        }
        playPauseIcon();
    }
}

void Country::setRadioImage(const QModelIndex &index)
{
    qDebug() << "========================================";
    if (!(radioList->getJsonListProcessor()->isConnected))
        return;
    QEventLoop loop;
    if (iconAddresses.size() == 0)
        return;
    QUrl imageUrl(getIconAddresses(index.row()));

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
            if (radioList->getIsDarkMode()) {
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
        if (radioList->getIsDarkMode()) {
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

QString Country::getIconAddresses(int index) const
{
    return this->iconAddresses[index];
}

bool Country::getIsPlaying() const
{
    return isPlaying;
}

void Country::setIsPlaying(bool newIsPlaying)
{
    isPlaying = newIsPlaying;
}

void Country::clearTableColor()
{
    if (customColor)
        customColor->clearRowColor();
    ui->tableOfCoutries->update();
}

void Country::checkIsOnPlaylist(const QModelIndex &index, QString currentRadioPlayingAddress)
{
    if (radioList->isAddressExists(currentRadioPlayingAddress, RADIO_BROWSER_PLAYLIST)) {
        ui->favorite->setIcon(QIcon(":/images/img/bookmark-file.png"));
    } else {
        ui->favorite->setIcon(QIcon(":/images/img/bookmark-empty.png"));
    }
}

void Country::playPauseIcon()
{
    if (getIsPlaying()) {
        ui->playPause->setIcon(QIcon(":/images/img/pause30.png"));
        miniPlayer.getMui()->play->setIcon(QIcon(":/images/img/pause30.png"));
    } else {
        ui->playPause->setIcon(QIcon(":/images/img/play30.png"));
        miniPlayer.getMui()->play->setIcon(QIcon(":/images/img/play30.png"));
    }
}
