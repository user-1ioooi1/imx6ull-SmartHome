#include "networkapibase.h"
#include <QRegularExpression>
#include <QSslConfiguration>

// ==================== 公有函数 ====================

NetworkApiBase::NetworkApiBase(QObject *parent)
    : QObject(parent)
    , m_manager(nullptr)
    , m_currentReply(nullptr)
    , m_timeoutTimer(nullptr)
    , m_isStreamMode(false)
    , m_debugEnabled(true)
    , m_isRunning(false)
    , m_maxRetries(0)
    , m_currentRetry(0)
    , m_httpStatusCode(0)
    , m_currentOperation(QNetworkAccessManager::UnknownOperation)
{

    init();
}

NetworkApiBase::~NetworkApiBase()
{
    cancel();
    if (m_manager) {
        delete m_manager;
        m_manager = nullptr;
    }
    if (m_timeoutTimer) {
        delete m_timeoutTimer;
        m_timeoutTimer = nullptr;
    }
}

bool NetworkApiBase::post(const QString& url, const QJsonObject& jsonData,
                          bool isStream, int timeoutMs)
{
    QJsonDocument doc(jsonData);
    QByteArray postData = doc.toJson();

    log(QString("POST请求准备: %1, 数据大小: %2, 流式: %3")
        .arg(url).arg(postData.size()).arg(isStream));

    return postRaw(url, postData, isStream, timeoutMs);
}

bool NetworkApiBase::postRaw(const QString& url, const QByteArray& data,
                             bool isStream, int timeoutMs)
{
    if (m_isRunning) {
        log("已有请求正在进行，自动取消当前请求");
        cancel();
    }

    m_postData = data;
    m_currentOperation = QNetworkAccessManager::PostOperation;

    QNetworkRequest request = createRequest(url);

    // 设置Content-Length（如果需要）
    if (!request.hasRawHeader("Content-Length")) {
        request.setHeader(QNetworkRequest::ContentLengthHeader, data.size());
    }

    log(QString("发送POST请求到: %1, 数据大小: %2")
        .arg(request.url().toString()).arg(data.size()));

    sendRequest(request, data, isStream,timeoutMs);
    return true;
}

bool NetworkApiBase::get(const QString& url, bool isStream, int timeoutMs)
{
    if (m_isRunning) {
        log("已有请求正在进行，自动取消当前请求");
        cancel();
    }

    m_postData.clear();
    m_currentOperation = QNetworkAccessManager::GetOperation;

    QNetworkRequest request = createRequest(url);

    log(QString("发送GET请求到: %1").arg(request.url().toString()));

    sendRequest(request, QByteArray(), isStream,timeoutMs);
    return true;
}

void NetworkApiBase::setHeader(const QString& headerName, const QString& headerValue)
{
    m_defaultHeaders[headerName] = headerValue;
    log(QString("设置请求头: %1 = %2").arg(headerName).arg(headerValue));
}

void NetworkApiBase::setHeaders(const QMap<QString, QString>& headers)
{
    for (auto it = headers.begin(); it != headers.end(); ++it) {
        m_defaultHeaders[it.key()] = it.value();
    }
    log(QString("批量设置%1个请求头").arg(headers.size()));
}

void NetworkApiBase::setBearerToken(const QString& token)
{
    setHeader("Authorization", "Bearer " + token);
}

void NetworkApiBase::cancel()
{
    if (m_currentReply) {
        log("取消当前请求");
        m_currentReply->abort();
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
    }
    reset();
}

// ==================== 保护函数 ====================

void NetworkApiBase::handleResponse(const QByteArray& data, const QString& url)
{
    Q_UNUSED(data)
    Q_UNUSED(url)
    // 基类空实现，子类可重写
}

void NetworkApiBase::handleStreamMessage(const QByteArray& message, const QString& url)
{
    Q_UNUSED(message)
    Q_UNUSED(url)
    // 基类空实现，子类可重写
}

void NetworkApiBase::handleStreamFinished(const QString& url)
{
    Q_UNUSED(url)
    // 基类空实现，子类可重写
}

void NetworkApiBase::handleError(const QString& errorString, const QString& url, int httpCode)
{
    Q_UNUSED(errorString)
    Q_UNUSED(url)
    Q_UNUSED(httpCode)
    // 基类空实现，子类可重写
}

bool NetworkApiBase::shouldRetry(QNetworkReply::NetworkError error)
{
    // 可以重试的错误类型
    return error == QNetworkReply::TimeoutError ||
           error == QNetworkReply::TemporaryNetworkFailureError ||
           error == QNetworkReply::ProxyConnectionRefusedError ||
           error == QNetworkReply::ServiceUnavailableError ||
           error == QNetworkReply::InternalServerError ||
           error == QNetworkReply::UnknownNetworkError;
}

void NetworkApiBase::log(const QString& message)
{
    if (m_debugEnabled) {
        qDebug() << "[NetworkApi]" << QDateTime::currentDateTime().toString("hh:mm:ss.zzz")
                 << message;
    }
}

QString NetworkApiBase::buildFullUrl(const QString& url) const
{
    if (url.startsWith("http://") || url.startsWith("https://")) {
        return url;
    }

    if (m_baseUrl.isEmpty()) {
        return url;
    }

    QString base = m_baseUrl;
    if (base.endsWith('/') && url.startsWith('/')) {
        return base + url.mid(1);
    } else if (!base.endsWith('/') && !url.startsWith('/')) {
        return base + '/' + url;
    } else {
        return base + url;
    }
}

// ==================== 私有槽函数 ====================

void NetworkApiBase::onReadyRead()
{
    if (!m_currentReply) return;

    QByteArray data = m_currentReply->readAll();

    if (m_isStreamMode) {
        // 流式模式：追加到缓冲区，尝试解析
        m_streamBuffer.append(data);
        processStreamBuffer();
    } else {
        // 非流式模式：只累积，不解析
        m_responseBuffer.append(data);
        log(QString("收到数据块，当前总大小: %1").arg(m_responseBuffer.size()));
    }
}

void NetworkApiBase::onFinished()
{
    if (!m_currentReply) return;

    // 停止超时定时器
    if (m_timeoutTimer && m_timeoutTimer->isActive()) {
        m_timeoutTimer->stop();
    }

    QNetworkReply* reply = m_currentReply;
    QString url = m_currentUrl;
    QNetworkReply::NetworkError errorCode = reply->error();
    m_httpStatusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    log(QString("请求完成: %1, 错误码: %2, HTTP状态码: %3, 重试次数: %4")
        .arg(url).arg(errorCode).arg(m_httpStatusCode).arg(m_currentRetry));

    if (errorCode == QNetworkReply::NoError) {
        if (m_isStreamMode) {
            // 流式模式：处理可能剩余的缓冲区数据
            if (!m_streamBuffer.isEmpty()) {
                processStreamBuffer();
            }

            // 发出流式完成信号
            log("流式响应完成");
            emit streamFinished(url);
            handleStreamFinished(url);
        } else {
            // 非流式模式：发出完整响应
            log(QString("非流式响应完成，总数据大小: %1").arg(m_responseBuffer.size()));
            emit requestFinished(m_responseBuffer, url);
            handleResponse(m_responseBuffer, url);
        }

        m_isRunning = false;
    } else {
        // 错误处理
        QString errorString = reply->errorString();

        // 重试逻辑
        if (m_currentRetry < m_maxRetries && shouldRetry(errorCode)) {
            m_currentRetry++;
            log(QString("第%1次重试...").arg(m_currentRetry));

            // 根据原始操作类型重试
            if (m_currentOperation == QNetworkAccessManager::PostOperation) {
                postRaw(url, m_postData, m_isStreamMode, m_timeoutTimer->interval());
            } else {
                get(url, m_isStreamMode, m_timeoutTimer->interval());
            }
        } else {
            log(QString("请求错误: %1").arg(errorString));
            emit error(errorString, url, m_httpStatusCode);
            handleError(errorString, url, m_httpStatusCode);
            m_isRunning = false;
        }
    }

    // 清理
    if (errorCode != QNetworkReply::NoError || !m_isRunning) {
        reply->deleteLater();
        m_currentReply = nullptr;
        reset();
    }
}

void NetworkApiBase::onErrorOccurred(QNetworkReply::NetworkError code)
{
    if (!m_currentReply) return;

    log(QString("网络错误发生: %1, %2")
        .arg(code).arg(m_currentReply->errorString()));

    // 错误会在 onFinished 中统一处理
}

void NetworkApiBase::onUploadProgress(qint64 bytesSent, qint64 bytesTotal)
{
    emit uploadProgress(bytesSent, bytesTotal);
}

void NetworkApiBase::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    emit downloadProgress(bytesReceived, bytesTotal);
}

void NetworkApiBase::onTimeout()
{
    if (m_currentReply && m_isRunning) {
        log(QString("请求超时: %1").arg(m_currentUrl));
        emit timeout(m_currentUrl);
        handleError("请求超时", m_currentUrl, 0);
        cancel();
    }
}

void NetworkApiBase::onSslErrors(const QList<QSslError>& errors)
{
    QString errorStr;
    for (const QSslError& error : errors) {
        if (!errorStr.isEmpty()) errorStr += "; ";
        errorStr += error.errorString();
    }
    log(QString("SSL错误: %1").arg(errorStr));

    // 默认忽略非关键SSL错误（可以根据需要修改）
    if (m_currentReply) {
        m_currentReply->ignoreSslErrors();
    }
}

// ==================== 私有函数 ====================

void NetworkApiBase::init()
{
    m_manager = new QNetworkAccessManager(this);

    m_timeoutTimer = new QTimer(this);
    m_timeoutTimer->setSingleShot(true);
    connect(m_timeoutTimer, &QTimer::timeout, this, &NetworkApiBase::onTimeout);

    // 设置默认请求头
    m_defaultHeaders["Content-Type"] = "application/json";
    m_defaultHeaders["Accept"] = "application/json";
    m_defaultHeaders["User-Agent"] = QString("NetworkApiBase/1.0 (Qt %1)")
                                        .arg(QT_VERSION_STR).toUtf8();

    log("初始化完成");
}

void NetworkApiBase::reset()
{
    m_responseBuffer.clear();
    m_streamBuffer.clear();
    m_currentUrl.clear();
    m_httpStatusCode = 0;
    m_postData.clear();

    if (m_timeoutTimer && m_timeoutTimer->isActive()) {
        m_timeoutTimer->stop();
    }

    // 注意：不重置重试计数，因为可能用于下一次请求
    // m_currentRetry 在 sendRequest 中重置
}

QNetworkRequest NetworkApiBase::createRequest(const QString& url)
{
    QNetworkRequest request;
    QString fullUrl = buildFullUrl(url);
    request.setUrl(QUrl(fullUrl));

    // 设置默认头
    for (auto it = m_defaultHeaders.begin(); it != m_defaultHeaders.end(); ++it) {
        request.setRawHeader(it.key().toUtf8(), it.value().toUtf8());
    }
    /*
    // 启用自动重定向
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);

    // 设置SSL配置
    QSslConfiguration sslConfig = request.sslConfiguration();
    sslConfig.setPeerVerifyMode(QSslSocket::VerifyPeer);
    request.setSslConfiguration(sslConfig);
    */

    return request;
}

void NetworkApiBase::sendRequest(const QNetworkRequest& request, const QByteArray& data, bool isStream,int timeoutMs)
{
    m_currentUrl = request.url().toString();
    m_isStreamMode = isStream;
    m_isRunning = true;

    // 重置重试计数（新请求）
    if (m_currentRetry == 0) {
        // 只有新请求才重置，重试时会保留计数
    } else {
        // 重试时计数已在外部增加
    }

    // 根据操作类型发送请求
    if (m_currentOperation == QNetworkAccessManager::PostOperation) {
        m_currentReply = m_manager->post(request, data);
    } else {
        m_currentReply = m_manager->get(request);
    }

    // 连接信号
    connect(m_currentReply, &QNetworkReply::readyRead,
            this, &NetworkApiBase::onReadyRead);
    connect(m_currentReply, &QNetworkReply::finished,
            this, &NetworkApiBase::onFinished);
    connect(m_currentReply,QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error),
            this, &NetworkApiBase::onErrorOccurred);
    connect(m_currentReply, &QNetworkReply::uploadProgress,
            this, &NetworkApiBase::onUploadProgress);
    connect(m_currentReply, &QNetworkReply::downloadProgress,
            this, &NetworkApiBase::onDownloadProgress);
    connect(m_currentReply, &QNetworkReply::sslErrors,
            this, &NetworkApiBase::onSslErrors);

    // 启动超时定时器
    if (timeoutMs <= 0) {
        timeoutMs = 30000; // 默认30秒
    }
    m_timeoutTimer->start(timeoutMs);
    emit requestStarted(m_currentUrl);

    log(QString("请求已发送: %1").arg(m_currentUrl));
}

void NetworkApiBase::processStreamBuffer()
{
    // SSE协议解析：消息以 \n\n 分隔
    int processed = 0;
    const QByteArray& buffer = m_streamBuffer;

    for (int i = 0; i < buffer.size(); ++i) {
        // 查找消息边界（连续两个换行符）
        if (i > 0 && buffer[i] == '\n' && buffer[i-1] == '\n') {
            // 提取一条完整的消息（从上一个结束到当前结束）
            QByteArray message = buffer.mid(processed, i - processed - 1);
            processed = i + 1;

            // 处理这条消息
            processStreamMessage(message);
        }
    }

    // 移除已处理的数据，保留未完成的部分
    if (processed > 0) {
        m_streamBuffer = buffer.mid(processed);
        if (m_streamBuffer.isEmpty()) {
            log("流式缓冲区已清空");
        } else {
            log(QString("流式缓冲区剩余: %1 字节").arg(m_streamBuffer.size()));
        }
    }
}

void NetworkApiBase::processStreamMessage(const QByteArray& message)
{
    // 去除首尾空白
    QByteArray trimmed = message.trimmed();
    if (trimmed.isEmpty()) {
        log("收到空消息");
        return;
    }

    // SSE格式：以 "data: " 开头
    if (trimmed.startsWith("data: ")) {
        QByteArray content = trimmed.mid(6);  // 去掉 "data: "

        // 检查是否是结束标记
        if (content == "[DONE]") {
            log("收到流式结束标记");
            return;  // 结束标记在 onFinished 中处理
        }

        // 尝试解析JSON（可选，不做强制要求）
        QJsonParseError error;
        QJsonDocument::fromJson(content, &error);

        if (error.error == QJsonParseError::NoError) {
            log(QString("收到流式消息: %1 字节").arg(content.size()));
        } else {
            log(QString("收到非JSON流式消息: %1, 大小: %2 字节")
                .arg(error.errorString()).arg(content.size()));
        }

        // 发出原始消息数据
        emit streamMessageReceived(content, m_currentUrl);
        handleStreamMessage(content, m_currentUrl);
    } else if (trimmed.startsWith("id: ")) {
        // 可以处理SSE的其他字段
        log("收到SSE id字段，忽略");
    } else if (trimmed.startsWith("event: ")) {
        log("收到SSE event字段，忽略");
    } else if (trimmed.startsWith("retry: ")) {
        log("收到SSE retry字段，忽略");
    } else {
        log(QString("收到未知格式消息: %1").arg(QString::fromUtf8(trimmed)));
    }
}


QString NetworkApiBase::getJsonValue(const QByteArray &data, const QString &path) {
        QJsonParseError parseError;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &parseError);

        if (parseError.error != QJsonParseError::NoError) {
            qDebug() << "JSON 解析错误:" << parseError.errorString();
            return QString();
        }

        QJsonValue current = jsonDoc.isObject() ? QJsonValue(jsonDoc.object()) : QJsonValue(jsonDoc.array());

        // 按点号分割路径，例如 "choices.0.message.content"
        QStringList keys = path.split('.');

        for (const QString &key : keys) {
            if (current.isObject()) {
                current = current.toObject().value(key);
            }
            else if (current.isArray()) {
                bool ok;
                int index = key.toInt(&ok);
                if (ok) {
                    QJsonArray arr = current.toArray();
                    if (index >= 0 && index < arr.size()) {
                        current = arr.at(index);
                    } else {
                        return QString();
                    }
                } else {
                    return QString();
                }
            }
            else {
                return QString();
            }

            if (current.isUndefined() || current.isNull()) {
                return QString();
            }
        }

        // 处理最终结果
        if (current.isString()) {
            return current.toString();
        }
        else if (current.isArray()) {
            QJsonArray arr = current.toArray();
            if (!arr.isEmpty()) {
                QJsonValue first = arr.at(0);
                if (first.isString()) {
                    return first.toString();
                }
            }
        }
        else if (current.isObject()) {
            // 返回整个对象的JSON字符串
            return QString(QJsonDocument(current.toObject()).toJson(QJsonDocument::Compact));
        }

        return QString();
    }


