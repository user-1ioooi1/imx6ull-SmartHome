#include "weatherapi.h"
#include <QSettings>
#include <QUrlQuery>

WeatherApi::WeatherApi(QObject *parent) : NetworkApiBase(parent)
{
    QSettings cfg(":/config.ini", QSettings::IniFormat);
    m_apiKey = cfg.value("Weather/api_key").toString();
}

void WeatherApi::queryWeather(const QString &city)
{
    if (m_apiKey.isEmpty()) {
        emit weatherError("高德 API Key 未配置");
        return;
    }
    QString url = QString("%1?key=%2&city=%3&extensions=base&output=JSON")
                      .arg(URL).arg(m_apiKey).arg(city);
    this->get(url);
}

void WeatherApi::handleResponse(const QByteArray &data, const QString &url)
{

    QString status = getJsonValue(data, "status");
    if (status != "1") {
        QString info = getJsonValue(data, "info");
        emit weatherError(info.isEmpty() ? "天气查询失败" : info);
        return;
    }

    WeatherData wd;
    wd.city        = getJsonValue(data, "lives.0.city");
    wd.weather     = getJsonValue(data, "lives.0.weather");
    wd.temperature = getJsonValue(data, "lives.0.temperature");
    wd.humidity    = getJsonValue(data, "lives.0.humidity");
    wd.windDir     = getJsonValue(data, "lives.0.winddirection");
    wd.windPower   = getJsonValue(data, "lives.0.windpower");
    wd.reportTime  = getJsonValue(data, "lives.0.reporttime");

    emit weatherReady(wd);
}
