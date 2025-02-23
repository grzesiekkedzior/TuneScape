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
            this->streamAddresses.push_back(streamUrl);
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
