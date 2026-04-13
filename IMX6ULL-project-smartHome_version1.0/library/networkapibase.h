#ifndef NETWORKAPIBASE_H
#define NETWORKAPIBASE_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>
#include <QUrl>
#include <QMap>
#include <QDebug>

/**
 * @brief 网络API基类 - 封装通用的HTTP请求处理
 *
 * 功能特性：
 * - 支持普通响应和流式响应(SSE)
 * - 自动处理数据累积和分包
 * - 超时控制
 * - 错误重试
 * - 请求头管理
 * - 调试日志
 *
 * @author Your Name
 * @date 2024
 */
class NetworkApiBase : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    explicit NetworkApiBase(QObject *parent = nullptr);

    /**
     * @brief 析构函数
     */
    virtual ~NetworkApiBase();

    /**
     * @brief 发送POST请求
     * @param url 请求URL
     * @param jsonData JSON数据
     * @param isStream 是否为流式响应
     * @param timeoutMs 超时时间（毫秒），默认30秒
     * @return 是否成功发起请求
     */
    bool post(const QString& url, const QJsonObject& jsonData,
              bool isStream = false, int timeoutMs = 30000);

    /**
     * @brief 发送POST请求（原始数据）
     * @param url 请求URL
     * @param data 原始数据
     * @param isStream 是否为流式响应
     * @param timeoutMs 超时时间（毫秒）
     * @return 是否成功发起请求
     */
    bool postRaw(const QString& url, const QByteArray& data,
                 bool isStream = false, int timeoutMs = 30000);

    /**
     * @brief 发送GET请求
     * @param url 请求URL
     * @param isStream 是否为流式响应
     * @param timeoutMs 超时时间（毫秒）
     * @return 是否成功发起请求
     */
    bool get(const QString& url, bool isStream = false, int timeoutMs = 30000);

    /**
     * @brief 设置请求头
     * @param headerName 头名称
     * @param headerValue 头值
     */
    void setHeader(const QString& headerName, const QString& headerValue);

    /**
     * @brief 批量设置请求头
     * @param headers 头映射表
     */
    void setHeaders(const QMap<QString, QString>& headers);

    /**
     * @brief 设置Bearer Token认证
     * @param token 访问令牌
     */
    void setBearerToken(const QString& token);

    /**
     * @brief 设置基础URL（会拼接到请求URL前）
     * @param baseUrl 基础URL
     */
    void setBaseUrl(const QString& baseUrl) { m_baseUrl = baseUrl; }

    /**
     * @brief 获取当前请求的URL
     * @return 当前URL
     */
    QString currentUrl() const { return m_currentUrl; }

    /**
     * @brief 取消当前请求
     */
    void cancel();

    /**
     * @brief 设置最大重试次数
     * @param retries 重试次数，默认0不重试
     */
    void setMaxRetries(int retries) { m_maxRetries = retries; }

    /**
     * @brief 获取当前重试次数
     * @return 已重试次数
     */
    int currentRetryCount() const { return m_currentRetry; }

    /**
     * @brief 启用/禁用调试日志
     * @param enabled true启用，false禁用
     */
    void setDebugEnabled(bool enabled) { m_debugEnabled = enabled; }

    /**
     * @brief 检查是否有请求正在进行
     * @return true有请求，false空闲
     */
    bool isRunning() const { return m_isRunning; }

    /**
     * @brief 获取当前请求的HTTP状态码
     * @return HTTP状态码，0表示未知
     */
    int httpStatusCode() const { return m_httpStatusCode; }


    QString getJsonValue(const QByteArray &data, const QString &path);

signals:
    /**
     * @brief 请求完成（普通响应）
     * @param data 完整响应数据
     * @param url 请求URL
     */
    void requestFinished(const QByteArray& data, const QString& url);

    /**
     * @brief 流式数据到达（流式响应）
     * @param data 完整的消息数据（已去除SSE包装）
     * @param url 请求URL
     */
    void streamMessageReceived(const QByteArray& data, const QString& url);

    /**
     * @brief 流式响应完成
     * @param url 请求URL
     */
    void streamFinished(const QString& url);

    /**
     * @brief 请求错误
     * @param errorString 错误信息
     * @param url 请求URL
     * @param httpCode HTTP状态码
     */
    void error(const QString& errorString, const QString& url, int httpCode = 0);

    /**
     * @brief 上传进度
     * @param bytesSent 已发送字节数
     * @param bytesTotal 总字节数
     */
    void uploadProgress(qint64 bytesSent, qint64 bytesTotal);

    /**
     * @brief 下载进度
     * @param bytesReceived 已接收字节数
     * @param bytesTotal 总字节数
     */
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);

    /**
     * @brief 请求开始
     * @param url 请求URL
     */
    void requestStarted(const QString& url);

    /**
     * @brief 请求超时
     * @param url 请求URL
     */
    void timeout(const QString& url);

protected:
    /**
     * @brief 处理响应数据（子类可重写）
     * @param data 完整响应数据
     * @param url 请求URL
     */
    virtual void handleResponse(const QByteArray& data, const QString& url);

    /**
     * @brief 处理流式消息（子类可重写）
     * @param message 完整的消息数据
     * @param url 请求URL
     */
    virtual void handleStreamMessage(const QByteArray& message, const QString& url);

    /**
     * @brief 处理流式完成（子类可重写）
     * @param url 请求URL
     */
    virtual void handleStreamFinished(const QString& url);

    /**
     * @brief 处理错误（子类可重写）
     * @param errorString 错误信息
     * @param url 请求URL
     * @param httpCode HTTP状态码
     */
    virtual void handleError(const QString& errorString, const QString& url, int httpCode);

    /**
     * @brief 判断是否可以重试
     * @param error 网络错误类型
     * @return true可以重试
     */
    virtual bool shouldRetry(QNetworkReply::NetworkError error);

    /**
     * @brief 记录调试日志
     * @param message 日志信息
     */
    void log(const QString& message);

    /**
     * @brief 构建完整URL
     * @param url 原始URL
     * @return 完整URL
     */
    QString buildFullUrl(const QString& url) const;

    /**
     * @brief 获取当前请求的原始回复对象
     * @return QNetworkReply指针，可能为空
     */
    QNetworkReply* currentReply() const { return m_currentReply; }

    /**
     * @brief 获取当前请求模式
     * @return true流式模式，false普通模式
     */
    bool isStreamMode() const { return m_isStreamMode; }

private slots:
    void onReadyRead();
    void onFinished();
    void onErrorOccurred(QNetworkReply::NetworkError code);
    void onUploadProgress(qint64 bytesSent, qint64 bytesTotal);
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onTimeout();
    void onSslErrors(const QList<QSslError>& errors);

private:
    /**
     * @brief 初始化
     */
    void init();

    /**
     * @brief 重置状态
     */
    void reset();

    /**
     * @brief 创建请求对象
     * @param url 请求URL
     * @return 配置好的QNetworkRequest
     */
    QNetworkRequest createRequest(const QString& url);

    /**
     * @brief 处理流式缓冲区
     */
    void processStreamBuffer();

    /**
     * @brief 处理单条流式消息
     * @param message 原始SSE消息
     */
    void processStreamMessage(const QByteArray& message);

    /**
     * @brief 发送实际请求
     * @param request 请求对象
     * @param data 请求数据（POST时有效）
     * @param isStream 是否为流式
     */
    void sendRequest(const QNetworkRequest& request, const QByteArray& data, bool isStream,int timeoutms);

private:
    // 网络相关
    QNetworkAccessManager* m_manager;
    QNetworkReply* m_currentReply;

    // 定时器
    QTimer* m_timeoutTimer;

    // 请求头
    QMap<QString, QString> m_defaultHeaders;

    // URL相关
    QString m_baseUrl;
    QString m_currentUrl;

    // 数据缓冲区
    QByteArray m_responseBuffer;    // 非流式：累积完整响应
    QByteArray m_streamBuffer;       // 流式：处理不完整的SSE数据

    // 状态标志
    bool m_isStreamMode;
    bool m_debugEnabled;
    bool m_isRunning;

    // 重试相关
    int m_maxRetries;
    int m_currentRetry;

    // HTTP状态码
    int m_httpStatusCode;

    // 请求类型
    QNetworkAccessManager::Operation m_currentOperation;
    QByteArray m_postData;  // 用于重试时保存POST数据
};

#endif // NETWORKAPIBASE_H
