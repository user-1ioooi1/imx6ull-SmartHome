#include "asrapi.h"

asrApi::asrApi(QObject *parent):
    NetworkApiBase(parent)
{
    this->setHeader("Content-Type","audio/pcm;rate=16000");
}


asrApi::~asrApi(){

}




void asrApi::initialize(const QString &apiKey, const QString &secretKey){

    // 构建 token URL
    QString url = QString(TOKEN_URL).arg(apiKey).arg(secretKey);

    // 发送空数据获取 token
    this->postRaw(url, QByteArray());

    qDebug() << "url = "<< url;
    qDebug() << "正在获取 access token...";

}


// 识别 PCM 数据（直接从缓冲区）
void asrApi::recognizePcm(const QByteArray &pcmData){

    if (pcmData.isEmpty()) {
        qDebug() << "数据为空 \n";
        return;
    }

    if (m_accessToken.isEmpty()) {
        qDebug() <<"请先调用 initialize() 获取 access token \n";
        return;
    }

    qDebug() << "开始识别 PCM 数据，大小:" << pcmData.size() << "字节";

    // 直接发送 PCM 数据
    this->postRaw(m_serverApiUrl,pcmData);


}

void asrApi::handleResponse(const QByteArray &data, const QString &url){
    if(url.contains("oauth")){
        QString token = getJsonValue(data, "access_token");
        if(!token.isEmpty()){
            m_accessToken = token;
            qDebug() << "获取 access token 成功";
            // 构建识别 URL
            QString cuid = QHostInfo::localHostName();
            m_serverApiUrl = QString(SERVER_API).arg(cuid).arg(m_accessToken);

        }else{
            qDebug() << "获取 access token 失败";
        }
    }else if(url.contains("server_api")){
        qDebug() << QString(data) << endl;
        QString result = getJsonValue(data, "result");
        if (!result.isEmpty()) {
            qDebug() << "识别结果:" << result;
            emit asrReadyData(result);
        }else{
            qDebug() << "识别结果: void" << result;
        }
    }else{
         qDebug() << "识别结果:" << "error \n";
    }





}






