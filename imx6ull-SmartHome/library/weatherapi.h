#ifndef WEATHERAPI_H
#define WEATHERAPI_H

#include <QObject>
#include "networkapibase.h"

struct WeatherData {
    QString city;
    QString weather;     // 天气现象，如"晴"
    QString temperature; // 实时温度
    QString humidity;    // 湿度
    QString windDir;     // 风向
    QString windPower;   // 风力
    QString reportTime;  // 数据发布时间
};

class WeatherApi : public NetworkApiBase
{
    Q_OBJECT

public:
    explicit WeatherApi(QObject *parent = nullptr);

    // 查询实时天气，city 为城市名或 adcode（如 "北京" 或 "110000"）
    void queryWeather(const QString &city);

signals:
    void weatherReady(const WeatherData data);
    void weatherError(const QString &error);
    void cityReady(const QString city);

private:
    void handleResponse(const QByteArray &data, const QString &url) override;

    QString m_apiKey;
    QString cityCode;
    const QString URL = "http://restapi.amap.com/v3/weather/weatherInfo";
};

#endif // WEATHERAPI_H
