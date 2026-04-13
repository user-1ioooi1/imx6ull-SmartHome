#ifndef ASRAPI_H
#define ASRAPI_H

#include <QObject>
#include "networkapibase.h"
#include <QHostInfo>

class asrApi:public NetworkApiBase
{
    Q_OBJECT

public:
    explicit asrApi(QObject *parent = nullptr);
    ~asrApi();


    void initialize(const QString &apiKey, const QString &secretKey);
    // 识别 PCM 数据（直接从缓冲区）
    void recognizePcm(const QByteArray &pcmData);

    QString getToken(){ return m_accessToken; }

signals:
    void asrReadyData(const QString &text);
private:
    void handleResponse(const QByteArray &data, const QString &url) override;
    QString m_accessToken;
    QString m_serverApiUrl;

    // 常量
    /* 获取token的接口*/
    const QString TOKEN_URL = "http://aip.baidubce.com/oauth/2.0/token?grant_type=client_credentials&client_id=%1&client_secret=%2";

    /* 百度服务器API接口，发送语音可返回识别结果 */
    const QString SERVER_API = "http://vop.baidu.com/server_api?dev_pid=1537&cuid=%1&token=%2";

};

#endif // ASRAPI_H
