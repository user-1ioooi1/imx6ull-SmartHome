#include "weatherwidget.h"
#include "ui_weatherwidget.h"

WeatherWidget::WeatherWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::WeatherWidget)
{
    ui->setupUi(this);
    connect(&m_weatherTimer,&QTimer::timeout,[this](){
        m_weather.queryWeather(cityCode);
    });
    connect(&m_location,&IpLocationApi::cityReady,[this](const QString cityCode){
        this->cityCode = cityCode;
        m_weather.queryWeather(this->cityCode);
    });
    connect(&m_weather,&WeatherApi::weatherReady,[this](WeatherData data){
        QString str = QString("image: url(:/icon/weather/weatherIcon/%1.png)").arg(data.weather);
        ui->weatherIconLabel->setStyleSheet(str);
        ui->weatherLabel->setText(data.weather);
        ui->tempatureLabel->setText(data.temperature + "°C");
        ui->cityLabel->setText(data.city);
    });

    m_weatherTimer.start(36e5);// 1e3 = 1s 3600s = 1h

    m_location.queryLocation();

}

WeatherWidget::~WeatherWidget()
{
    delete ui;
}


void WeatherWidget::on_refreshButton_clicked()
{
    if(!cityCode.isEmpty())
        m_weather.queryWeather(this->cityCode);
}
