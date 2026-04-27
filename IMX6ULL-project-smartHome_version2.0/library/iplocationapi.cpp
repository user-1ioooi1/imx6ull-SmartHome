#include "iplocationapi.h"
#include <QSettings>

IpLocationApi::IpLocationApi(QObject *parent): NetworkApiBase(parent)
{
    QSettings cfg(":/config.ini", QSettings::IniFormat);
    m_apiKey = cfg.value("Weather/api_key").toString();
}

void IpLocationApi::queryLocation(){
    if (m_apiKey.isEmpty()) {
        emit Error("高德 API Key 未配置");
        return;
    }
    QString url = QString("%1?key=%2").arg(URL).arg(m_apiKey);
    this->get(url);
}

void IpLocationApi::handleResponse(const QByteArray &data, const QString &url)
{
    QString status = getJsonValue(data, "status");
    if (status != "1") {
        QString info = getJsonValue(data, "info");
        emit Error(info.isEmpty() ? "ip查询失败" : info);
        return;
    }
    QString cityCode = getJsonValue(data,"adcode");
    emit cityReady(cityCode);

}
