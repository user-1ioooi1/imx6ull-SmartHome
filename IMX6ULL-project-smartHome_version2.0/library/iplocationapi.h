#ifndef IPLOCATIONAPI_H
#define IPLOCATIONAPI_H

#include "networkapibase.h"

class IpLocationApi : public NetworkApiBase
{
    Q_OBJECT

public:
    explicit IpLocationApi(QObject *parent = nullptr);

    // 利用IP查询城市，得到城市adcode
    void queryLocation();

signals:
    void cityReady(const QString adcode);
    void Error(const QString error);

private:
    void handleResponse(const QByteArray &data, const QString &url) override;

    QString m_apiKey;
    const QString URL = "http://restapi.amap.com/v3/ip";
};

#endif // IPLOCATIONAPI_H
