#include "ttsapi.h"

ttsApi::ttsApi(QObject *parent):
    NetworkApiBase(parent),
    m_mediaPlayer(new QMediaPlayer(this))
{

}


ttsApi::~ttsApi() {

}



void ttsApi::tts_post(const QString & userMessage, const QString &token){
    QString secondEncode = DoubleEncoding(userMessage);
    // 构建POST数据
    QUrlQuery postData;
    postData.addQueryItem("tex", secondEncode);
    postData.addQueryItem("lan", "zh");
    postData.addQueryItem("cuid", "HAwrdDtJDBAWggrUg2QM4xLrWkhwDGY4"); //random
    postData.addQueryItem("ctp", "1");
    postData.addQueryItem("aue", "3"); //mp3
    postData.addQueryItem("tok",token);
    postData.addQueryItem("audio_ctrl", "{\"sampling_rate\":16000}");

    QByteArray postDataByteArray = postData.toString(QUrl::FullyEncoded).toUtf8();
    this->postRaw(URL,postDataByteArray);
}

QString ttsApi::DoubleEncoding(const QString & userMessage){
    QString firstEncode = QString::fromUtf8(QUrl::toPercentEncoding(userMessage));
    //qDebug() << "第一次编码:" << firstEncode;
     // 输出: "%e7%99%be%e5%ba%a6%e4%bd%a0%e5%a5%bd"

    // 第二次URL编码 - 对第一次编码的结果再次编码
    QString secondEncode = QString::fromUtf8(QUrl::toPercentEncoding(firstEncode));
    //qDebug() << "第二次编码:" << secondEncode;
    // 输出: "%25e7%2599%25be%25e5%25ba%25a6%25e4%25bd%25a0%25e5%25a5%25bd"

    return secondEncode;
}

void ttsApi::handleResponse(const QByteArray &data, const QString &url){
    if(data.size() == 0)
        return;

    m_mediaPlayer->stop();
    m_buf.close();
    m_buf.setData(data);
    if(m_buf.open(QIODevice::ReadOnly) == true){
        ;
    }else{
        qDebug() << "open err\n";
    };
    m_mediaPlayer->setMedia(QMediaContent(), &m_buf);
    m_mediaPlayer->play();
}







