#include "asr.h"
#include <QDebug>

Asr::Asr(QObject *parent)
    : QObject(parent)
{
    networkAccessManager = new QNetworkAccessManager(this);
}

Asr::~Asr()
{
}

void Asr::initialize(const QString &apiKey, const QString &secretKey)
{
    // 构建 token URL
    QString url = QString(TOKEN_URL).arg(apiKey).arg(secretKey);

    // 发送空数据获取 token
    requestNetwork(url, QByteArray());

    qDebug() << "正在获取 access token...";
}

void Asr::recognizePcm(const QByteArray &pcmData)
{
    if (pcmData.isEmpty()) {
        emit error("PCM 数据为空");
        return;
    }

    if (accessToken.isEmpty()) {
        emit error("请先调用 initialize() 获取 access token");
        return;
    }

    qDebug() << "开始识别 PCM 数据，大小:" << pcmData.size() << "字节";

    // 构建识别 URL
    QString cuid = QHostInfo::localHostName();
    serverApiUrl = QString(SERVER_API).arg(cuid).arg(accessToken);

    // 直接发送 PCM 数据
    requestNetwork(serverApiUrl, pcmData);
}

void Asr::recognizeFile(const QString &fileName)
{
    QFile file(fileName);
    if (!file.exists()) {
        emit error("文件不存在: " + fileName);
        return;
    }

    if (accessToken.isEmpty()) {
        emit error("请先调用 initialize() 获取 access token");
        return;
    }

    // 读取文件
    if (!file.open(QIODevice::ReadOnly)) {
        emit error("无法打开文件: " + fileName);
        return;
    }

    QByteArray fileData = file.readAll();
    file.close();

    qDebug() << "开始识别文件:" << fileName << "大小:" << fileData.size() << "字节";

    // 调用识别 PCM 数据的方法
    recognizePcm(fileData);
}

void Asr::requestNetwork(const QString &url, const QByteArray &requestData)
{
    QNetworkRequest request;
    /*
    // SSL 配置（https需要）
    QSslConfiguration config;
    config.setPeerVerifyMode(QSslSocket::VerifyNone);
    config.setProtocol(QSsl::TlsV1SslV3);
    request.setSslConfiguration(config);
*/
    // 设置PCM请求头
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      "audio/pcm;rate=16000");


    request.setUrl(QUrl(url));

    // 发送请求 全新对象
    QNetworkReply *reply = networkAccessManager->post(request, requestData);

    // 连接信号
    connect(reply, &QNetworkReply::finished, this, &Asr::replyFinished);
    connect(reply, &QNetworkReply::readyRead, this, &Asr::readyReadData);
}

void Asr::readyReadData()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;

    QByteArray data = reply->readAll();
    QString urlString = reply->url().toString();

    if (urlString.contains("oauth")) {
        // Token 响应
        QString token = getJsonValue(data, "access_token");
        if (!token.isEmpty()) {
            accessToken = token;
            emit initialized();
            qDebug() << "获取 access token 成功";
        } else {
            QString errorMsg = getJsonValue(data, "error_description");
            if (errorMsg.isEmpty()) {
                errorMsg = "获取 token 失败";
            }
            emit error(errorMsg);
            qDebug() << "获取 token 失败:" << data;
        }
    }
    else if (urlString.contains("server_api")) {
        // 识别结果
        QString result = getJsonValue(data, "result");
        if (!result.isEmpty()) {
            emit asrReadyData(result);
            qDebug() << "识别结果:" << result;
        } else {
            // 尝试解析错误信息
            QString errMsg = getJsonValue(data, "err_msg");
            if (!errMsg.isEmpty()) {
                emit error("识别失败: " + errMsg);
            } else {
                emit error("识别失败: " + data);
            }
            qDebug() << "识别失败:" << data;
        }
    }
}

void Asr::replyFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;

    // 检查网络错误
    if (reply->error() != QNetworkReply::NoError) {
        emit error("网络错误: " + reply->errorString());
        qDebug() << "网络错误:" << reply->errorString();
    }

    reply->deleteLater();
}

QString Asr::getJsonValue(const QByteArray &ba, const QString &key)
{
    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(ba, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        qDebug() << "JSON 解析错误:" << parseError.errorString();
        return QString();
    }

    if (!jsonDoc.isObject()) {
        return QString();
    }

    QJsonObject jsonObj = jsonDoc.object();

    if (!jsonObj.contains(key)) {
        return QString();
    }

    QJsonValue jsonVal = jsonObj.value(key);

    if (jsonVal.isString()) {
        return jsonVal.toString();
    }
    else if (jsonVal.isArray()) {
        QJsonArray arr = jsonVal.toArray();
        if (!arr.isEmpty()) {
            QJsonValue first = arr.at(0);
            if (first.isString()) {
                return first.toString();
            }
        }
    }

    return QString();
}
