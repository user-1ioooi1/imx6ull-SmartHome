#ifndef DEEPSEEKAPI_H
#define DEEPSEEKAPI_H

#include <QObject>
#include "networkapibase.h"

class deepseekApi:public NetworkApiBase
{
    Q_OBJECT
public:
    explicit deepseekApi(const QString &apiKey, const QString &url, QObject *parent = nullptr);
    ~deepseekApi() override;

    void ds_post(const QString & userMessage);
    QString getResponse();
signals:
    void responseHanled(const QString &QString);

private:


    void handleResponse(const QByteArray &data, const QString &url) override;
    QString m_apiKey;
    QString m_dsUrl;
};

#endif // DEEPSEEKAPI_H
