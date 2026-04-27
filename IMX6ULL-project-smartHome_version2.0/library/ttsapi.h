#ifndef TTSAPI_H
#define TTSAPI_H

#include <QObject>
#include "networkapibase.h"
#include <QUrlQuery>
#include <QMediaPlayer>
#include <QBuffer>

class ttsApi: public NetworkApiBase
{
    Q_OBJECT
public:
    explicit ttsApi(QObject *parent = nullptr);
    ~ttsApi() override;

    void tts_post(const QString & userMessage, const QString &token);


private:
    void handleResponse(const QByteArray &data, const QString &url) override;
    QString DoubleEncoding(const QString & userMessage);
    QMediaPlayer *m_mediaPlayer;
    QBuffer m_buf;
    const QString URL = "http://tsn.baidu.com/text2audio";

};

#endif // TTSAPI_H
