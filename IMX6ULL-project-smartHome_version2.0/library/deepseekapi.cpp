#include "deepseekapi.h"

deepseekApi::deepseekApi(const QString &apiKey, const QString &url, QObject *parent):
    NetworkApiBase(parent),
    m_apiKey(apiKey),
    m_dsUrl(url)
{
    this->setHeader("Content-Type","application/json");
    this->setBearerToken(m_apiKey);
}

deepseekApi::~deepseekApi()
{
}

void deepseekApi::ds_post(const QString &userMessage){
    QJsonObject root;
    root["model"] = "deepseek-ai/DeepSeek-V3";

    QJsonArray messages;

    QJsonObject systemMsg;
    systemMsg["role"] = "system";
    systemMsg["content"] = "你是一个智能家居系统助手,严格按照{%s/%s}的格式回复, 第一个%s是回复用户的内容，/是分隔符 第二个%s是控制家居的指令，指令不超过4个字";
    messages.append(systemMsg);

    QJsonObject userMsg;
    userMsg["role"] = "user";
    userMsg["content"] = userMessage;
    messages.append(userMsg);
    root["messages"] = messages;

    this->post(m_dsUrl, root, false);
}

void deepseekApi::handleResponse(const QByteArray &data, const QString &url){
    Q_UNUSED(url)
    QString content = getJsonValue(data,"choices.0.message.content");
    emit responseHanled(content);
}
