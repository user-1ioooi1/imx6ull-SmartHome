#ifndef ASR_H
#define ASR_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QFile>
#include <QUrl>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QHostInfo>
#include <QSslConfiguration>

class Asr : public QObject
{
    Q_OBJECT

public:
    explicit Asr(QObject *parent = nullptr);
    ~Asr();

    // 初始化（获取 access token）"
    void initialize(const QString &apiKey = "vaXnAXRcJ1XEoasWCyLdfGDv", const QString &secretKey = "s1rXeCFqCZmQtdfAz0ze81siGYyKyfgz");

    // 识别 PCM 数据（直接从缓冲区）
    void recognizePcm(const QByteArray &pcmData);

    // 识别文件（兼容旧代码）
    void recognizeFile(const QString &fileName);

    QString getToken(){ return accessToken; }

signals:
    void initialized();                 // 初始化完成
    void asrReadyData(const QString &text);  // 识别结果
    void error(const QString &message);      // 错误信息

private slots:
    void replyFinished();
    void readyReadData();

private:
    void requestNetwork(const QString &url, const QByteArray &requestData);
    QString getJsonValue(const QByteArray &ba, const QString &key);

private:
    QNetworkAccessManager *networkAccessManager;
    QString accessToken;
    QString serverApiUrl;

    // 常量
    /* 获取token的接口*/
    const QString TOKEN_URL = "http://aip.baidubce.com/oauth/2.0/token?grant_type=client_credentials&client_id=%1&client_secret=%2";

    /* 百度服务器API接口，发送语音可返回识别结果 */
    const QString SERVER_API = "http://vop.baidu.com/server_api?dev_pid=1537&cuid=%1&token=%2";
};

#endif // ASR_H
