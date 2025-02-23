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
        ui->comboBox->addItem(s.countryName + " - " + s.countryIso + " stations - "
                              + s.countryCount);
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
            reply->deleteLater();
            return true;
        }
        return false;
    }
}

void Country::countryNameJsonQuery(QNetworkReply *reply) {}
