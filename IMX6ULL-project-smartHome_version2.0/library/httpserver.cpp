#include "httpserver.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QDateTime>
#include <QUrl>

const QStringList HttpServer::VALID_COMMANDS = {
    "led_on", "led_off", "beep_on", "beep_off",
    "relay_on", "relay_off", "curtain_open", "curtain_close",
};

// 上传限制 100 MB
const qint64 HttpServer::MAX_UPLOAD_SIZE = 100LL * 1024 * 1024;

HttpServer::HttpServer(QObject *parent)
    : QObject(parent)
    , m_server(new QTcpServer(this))
{
    connect(m_server, &QTcpServer::newConnection, this, &HttpServer::onNewConnection);
    connect(SensorMonitor::instance(), &SensorMonitor::sensorDataUpdated,
            this, &HttpServer::updateSensorData);
}

bool HttpServer::start(quint16 port)
{
    if (!m_server->listen(QHostAddress::Any, port)) {
        qWarning() << "[HttpServer] 启动失败:" << m_server->errorString();
        return false;
    }
    qInfo() << "[HttpServer] 启动成功，端口:" << port;
    return true;
}

void HttpServer::stop()
{
    m_server->close();
}

void HttpServer::updateSensorData(const SensorData &data)
{
    m_lastSensorData = data;
}

void HttpServer::onNewConnection()
{
    while (m_server->hasPendingConnections()) {
        QTcpSocket *socket = m_server->nextPendingConnection();
        connect(socket, &QTcpSocket::readyRead, this, &HttpServer::onReadyRead);
        // 断开时清理缓冲
        connect(socket, &QTcpSocket::disconnected, this, [this, socket]() {
            m_pendingRequests.remove(socket);
            UploadCtx *ctx = m_uploads.value(socket, nullptr);
            if (ctx) {
                if (ctx->file) ctx->file->close();
                m_uploads.remove(socket);
                delete ctx;
            }
        });
    }
}

void HttpServer::onReadyRead()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) return;

    // socket 已在关闭流程（如 413 后 disconnectFromHost），排空数据但不处理，
        // 避免把文件体当新请求解析导致循环
        if (socket->state() != QAbstractSocket::ConnectedState) {
            socket->readAll();
            return;
        }


    // 上传进行中：把新数据直接流向磁盘
    if (m_uploads.contains(socket)) {
        feedUpload(socket, socket->readAll());
        return;
    }

    m_pendingRequests[socket] += socket->readAll();
    QByteArray &data = m_pendingRequests[socket];

    // 等待 HTTP 头部到齐
    int headerEnd = data.indexOf("\r\n\r\n");
    if (headerEnd < 0) return;

    // 检测是否是上传请求，头到齐就立刻进流式模式，不等 body
    QByteArray firstLine = data.left(data.indexOf('\n')).trimmed();
    QList<QByteArray> parts = firstLine.split(' ');
    bool isUpload = parts.size() >= 2 && parts[0] == "POST" &&
                    (parts[1] == "/api/media/upload" ||
                     parts[1].startsWith("/api/media/upload?"));

    if (isUpload) {
        qint64 contentLength = 0;
        for (const QByteArray &line : data.left(headerEnd).split('\n')) {
            if (line.toLower().startsWith("content-length:")) {
                contentLength = line.mid(15).trimmed().toLongLong();
                break;
            }
        }
        if (contentLength > MAX_UPLOAD_SIZE) {
            m_pendingRequests.remove(socket);
            sendResponse(socket, 413, "application/json", R"({"ok":false,"msg":"file too large"})");
            return;
        }
        // 从 URL 解析 type 参数
        QString type;
        if (parts.size() >= 2) {
            int q = parts[1].indexOf('?');
            if (q >= 0) {
                for (const QByteArray &p : parts[1].mid(q + 1).split('&')) {
                    if (p.startsWith("type=")) { type = QString::fromUtf8(p.mid(5)); break; }
                }
            }
        }
        QByteArray headers     = data.left(headerEnd + 4);
        QByteArray partialBody = data.mid(headerEnd + 4);
        m_pendingRequests.remove(socket);
        beginUpload(socket, headers, partialBody, type, contentLength);
        return;
    }

    // 普通请求：限 64 KB，等 body 收完再处理
    if (data.size() > 65536) {
        m_pendingRequests.remove(socket);
        sendResponse(socket, 413, "application/json", R"({"ok":false,"msg":"request too large"})");
        return;
    }
    qint64 contentLength = 0;
    for (const QByteArray &line : data.left(headerEnd).split('\n')) {
        if (line.toLower().startsWith("content-length:")) {
            contentLength = line.mid(15).trimmed().toLongLong();
            break;
        }
    }
    qint64 totalExpected = headerEnd + 4 + contentLength;
    if (data.size() < totalExpected) return;

    QByteArray request = data.left(totalExpected);
    m_pendingRequests.remove(socket);
    handleRequest(socket, request);
}

void HttpServer::handleRequest(QTcpSocket *socket, const QByteArray &request)
{
    QString req = QString::fromUtf8(request.left(4096)); // 只解析头部
    QString firstLine = req.left(req.indexOf('\n')).trimmed();
    QStringList parts = firstLine.split(' ');
    if (parts.size() < 2) {
        sendResponse(socket, 400, "text/plain", "Bad Request");
        return;
    }

    QString method  = parts[0];
    QString rawPath = parts[1];
    QString path    = rawPath.contains('?') ? rawPath.left(rawPath.indexOf('?')) : rawPath;

    // 解析查询参数 type=
    auto queryParam = [&](const QString &key) -> QString {
        int q = rawPath.indexOf('?');
        if (q < 0) return {};
        for (const QString &p : rawPath.mid(q + 1).split('&')) {
            if (p.startsWith(key + "=")) return p.mid(key.size() + 1);
        }
        return {};
    };

    if (method == "GET" && path == "/") {
        handleGetDashboard(socket);
    } else if (method == "GET" && path == "/api/status") {
        handleGetStatus(socket);
    } else if (method == "POST" && path == "/api/control") {
        int bodyStart = request.indexOf("\r\n\r\n");
        handlePostControl(socket, bodyStart >= 0 ? request.mid(bodyStart + 4) : QByteArray());
    } else if (method == "GET" && path == "/api/camera/stream") {
        handleCameraStream(socket);
    }else if (method == "GET" && path == "/api/camera/stream/stop") {
        qDebug() << "[Stream] stop requested, sockets:" << m_streamSockets.size();
        bool wasStreaming = !m_streamSockets.isEmpty();
        for (QTcpSocket *s : m_streamSockets)
            s->abort();
        m_streamSockets.clear();
        if (wasStreaming)
            emit cameraStreamingChanged(false);
        sendResponse(socket, 200, "application/json", R"({"ok":true})");
    } else if (method == "GET" && rawPath.startsWith("/api/media/list")) {
        handleMediaList(socket, queryParam("type"));
    } else if (method == "GET" && path.startsWith("/api/media/photo/")) {
        handleMediaFile(socket, "photo",
            QUrl::fromPercentEncoding(path.mid(17).toUtf8()), request);
    } else if (method == "GET" && path.startsWith("/api/media/video/")) {
        handleMediaFile(socket, "video",
            QUrl::fromPercentEncoding(path.mid(17).toUtf8()), request);
    } else if (method == "GET" && path.startsWith("/api/media/music/")) {
        handleMediaFile(socket, "music",
            QUrl::fromPercentEncoding(path.mid(17).toUtf8()), request);
    } else if (method == "OPTIONS") {
        sendResponse(socket, 200, "text/plain", "");
    } else {
        sendResponse(socket, 404, "text/plain", "Not Found");
    }
}

// ──────────────────────────────────────────────
//  原有端点
// ──────────────────────────────────────────────

void HttpServer::handleGetStatus(QTcpSocket *socket)
{
    QJsonObject sensors;
    sensors["temperature"] = m_lastSensorData.temperature;
    sensors["humidity"]    = m_lastSensorData.humidity;
    sensors["light"]       = m_lastSensorData.als;
    sensors["ir"]          = m_lastSensorData.ir;
    sensors["distance"]    = m_lastSensorData.ps;
    sensors["motion"]      = m_lastSensorData.motionDetected;
    sensors["airQuality"]  = m_lastSensorData.airQuality;

    QJsonObject json;
    json["sensors"]   = sensors;
    json["timestamp"] = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");

    sendResponse(socket, 200, "application/json",
                 QJsonDocument(json).toJson(QJsonDocument::Compact));
}

void HttpServer::handlePostControl(QTcpSocket *socket, const QByteArray &body)
{
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(body, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        sendResponse(socket, 400, "application/json", R"({"ok":false,"msg":"invalid json"})");
        return;
    }

    QString cmd = doc.object().value("cmd").toString();
    if (!VALID_COMMANDS.contains(cmd)) {
        sendResponse(socket, 400, "application/json", R"({"ok":false,"msg":"unknown command"})");
        return;
    }

    auto *hw = HardwareManager::instance();
    if      (cmd == "led_on")        hw->setLamp(true);
    else if (cmd == "led_off")       hw->setLamp(false);
    else if (cmd == "beep_on")       hw->setBeep(true);
    else if (cmd == "beep_off")      hw->setBeep(false);
    else if (cmd == "relay_on")      hw->setRelay(true);
    else if (cmd == "relay_off")     hw->setRelay(false);
    else if (cmd == "curtain_open")  hw->setServoAngle(90);
    else if (cmd == "curtain_close") hw->setServoAngle(0);

    sendResponse(socket, 200, "application/json", R"({"ok":true})");
}

void HttpServer::handleGetDashboard(QTcpSocket *socket)
{
    QFile f(":/res/res/dashboard.html");
    if (!f.open(QIODevice::ReadOnly)) {
        sendResponse(socket, 500, "text/plain", "dashboard.html not found");
        return;
    }
    sendResponse(socket, 200, "text/html; charset=utf-8", f.readAll());
}

// ──────────────────────────────────────────────
//  摄像头 MJPEG 流
// ──────────────────────────────────────────────

void HttpServer::handleCameraStream(QTcpSocket *socket)
{
    disconnect(socket, &QTcpSocket::readyRead, this, &HttpServer::onReadyRead);

    QByteArray header =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Cache-Control: no-cache, no-store\r\n"
        "Connection: keep-alive\r\n"
        "\r\n";
    socket->write(header);
    socket->flush();

    m_streamSockets.append(socket);
    if (m_streamSockets.size() == 1)
        emit cameraStreamingChanged(true);

    connect(socket, &QTcpSocket::disconnected, this, [this, socket]() {
        m_streamSockets.removeAll(socket);
        socket->deleteLater();
        if (m_streamSockets.isEmpty())
            emit cameraStreamingChanged(false);
    });
}

void HttpServer::onCameraFrame(const QByteArray &jpegData)
{
    if (m_streamSockets.isEmpty() || jpegData.isEmpty()) return;

    QByteArray chunk;
    chunk += "--frame\r\n";
    chunk += "Content-Type: image/jpeg\r\n";
    chunk += "Content-Length: ";
    chunk += QByteArray::number(jpegData.size());
    chunk += "\r\n\r\n";
    chunk += jpegData;
    chunk += "\r\n";

    QList<QTcpSocket*> dead;
    for (QTcpSocket *s : m_streamSockets) {
        if (s->state() == QAbstractSocket::ConnectedState) {
            s->write(chunk);
            s->flush();
        } else {
            dead.append(s);
        }
    }
    for (QTcpSocket *s : dead)
        m_streamSockets.removeAll(s);
}

// ──────────────────────────────────────────────
//  媒体文件列表
// ──────────────────────────────────────────────

void HttpServer::handleMediaList(QTcpSocket *socket, const QString &type)
{
    QString dir;
    QStringList filters;
    if (type == "photo") {
        dir = "./photo";
        filters = QStringList() << "*.jpg" << "*.jpeg" << "*.png";
    } else if (type == "video") {
        dir = "./video";
        filters = QStringList() << "*.mp4" << "*.mov";
    } else if (type == "music") {
        dir = "./music";
        filters = QStringList() << "*.mp3";
    } else {
        sendResponse(socket, 400, "application/json", R"({"error":"invalid type"})");
        return;
    }

    QDir d(dir);
    QJsonArray files;
    for (const QString &f : d.entryList(filters, QDir::Files))
        files.append(f);

    QJsonObject obj;
    obj["files"] = files;
    obj["type"]  = type;
    sendResponse(socket, 200, "application/json",
                 QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

// ──────────────────────────────────────────────
//  媒体文件下载（支持 Range）
// ──────────────────────────────────────────────

void HttpServer::handleMediaFile(QTcpSocket *socket, const QString &type,
                                  const QString &filename, const QByteArray &request)
{
    if (filename.isEmpty() || filename.contains('/') ||
        filename.contains('\\') || filename.contains("..")) {
        sendResponse(socket, 403, "text/plain", "Forbidden");
        return;
    }

    QString dir;
    if      (type == "photo") dir = "./photo";
    else if (type == "video") dir = "./video";
    else if (type == "music") dir = "./music";
    else { sendResponse(socket, 400, "text/plain", "Bad Request"); return; }

    QFile file(dir + "/" + filename);
    if (!file.exists()) { sendResponse(socket, 404, "text/plain", "File Not Found"); return; }

    qint64 fileSize = file.size();
    QString mimeType = mimeForFilename(filename);

    qint64 startByte = 0, endByte = fileSize - 1;
    bool hasRange = false;
    for (const QByteArray &line : request.split('\n')) {
        QString l = QString::fromUtf8(line).trimmed();
        if (l.startsWith("Range:", Qt::CaseInsensitive)) {
            QString val = l.mid(6).trimmed();
            if (val.startsWith("bytes=")) {
                QStringList p = val.mid(6).split('-');
                if (p.size() == 2) {
                    if (!p[0].isEmpty()) startByte = p[0].toLongLong();
                    endByte  = p[1].isEmpty() ? fileSize - 1 : p[1].toLongLong();
                    hasRange = true;
                }
            }
            break;
        }
    }

    startByte = qMax(0LL, qMin(startByte, fileSize - 1));
    endByte   = qMax(startByte, qMin(endByte, fileSize - 1));
    qint64 length = endByte - startByte + 1;

    if (!file.open(QIODevice::ReadOnly)) {
        sendResponse(socket, 500, "text/plain", "Cannot open file");
        return;
    }
    file.seek(startByte);

    QString contentRange;
    if (hasRange)
        contentRange = QString("Content-Range: bytes %1-%2/%3\r\n").arg(startByte).arg(endByte).arg(fileSize);

    QByteArray header = QString(
        "HTTP/1.1 %1 %2\r\n"
        "Content-Type: %3\r\n"
        "Content-Length: %4\r\n"
        "Accept-Ranges: bytes\r\n"
        "%5"
        "Access-Control-Allow-Origin: *\r\n"
        "Connection: close\r\n"
        "\r\n"
    ).arg(hasRange ? 206 : 200)
     .arg(hasRange ? "Partial Content" : "OK")
     .arg(mimeType).arg(length).arg(contentRange).toUtf8();

    socket->write(header);
    const qint64 CHUNK = 65536;
    qint64 remaining = length;
    while (remaining > 0 && file.isOpen()) {
        if (socket->state() != QAbstractSocket::ConnectedState) break;
        QByteArray chunk = file.read(qMin(remaining, CHUNK));
        if (chunk.isEmpty()) break;
        if (socket->write(chunk) < 0) break;
        remaining -= chunk.size();
    }
    file.close();
    if (socket->state() == QAbstractSocket::ConnectedState) {
        socket->flush();
        socket->disconnectFromHost();
    }
    connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);
}

// ──────────────────────────────────────────────
//  流式上传（边收边写磁盘，峰值内存 ≈ 几十字节）
// ──────────────────────────────────────────────

void HttpServer::beginUpload(QTcpSocket *socket, const QByteArray &headers,
                              const QByteArray &partialBody, const QString &type,
                              qint64 contentLength)
{
    QString dir;
    if      (type == "photo") dir = "./photo";
    else if (type == "video") dir = "./video";
    else if (type == "music") dir = "./music";
    else {
        sendResponse(socket, 400, "application/json", R"({"ok":false,"msg":"invalid type"})");
        return;
    }

    // 从 Content-Type 头提取 multipart boundary
    QByteArray boundary;
    int hEnd = headers.indexOf("\r\n\r\n");
    for (const QByteArray &line : headers.left(hEnd).split('\n')) {
        if (line.toLower().contains("content-type:") &&
            line.toLower().contains("multipart")) {
            int b = line.indexOf("boundary=");
            if (b >= 0) {
                boundary = line.mid(b + 9).trimmed();
                if (boundary.endsWith('\r')) boundary.chop(1);
            }
        }
    }
    if (boundary.isEmpty()) {
        sendResponse(socket, 400, "application/json", R"({"ok":false,"msg":"no boundary"})");
        return;
    }

    UploadCtx *ctx    = new UploadCtx();
    ctx->contentLength = contentLength;
    ctx->dir           = dir;
    ctx->boundary      = boundary;
    ctx->endMarker     = "\r\n--" + boundary + "--";
    ctx->phase         = UploadCtx::Init;
    m_uploads[socket]  = ctx;

    if (!partialBody.isEmpty())
        feedUpload(socket, partialBody);
}

void HttpServer::feedUpload(QTcpSocket *socket, const QByteArray &data)
{
    UploadCtx *ctx = m_uploads.value(socket, nullptr);
    if (!ctx || ctx->error || ctx->phase == UploadCtx::Done) return;

    // ── Init 阶段：积累直到找到 part 头结束位置 ──
    if (ctx->phase == UploadCtx::Init) {
        ctx->buf += data;

        QByteArray openBound = "--" + ctx->boundary + "\r\n";
        int partStart = ctx->buf.indexOf(openBound);
        if (partStart < 0) return;

        int partHeaderStart = partStart + openBound.size();
        int partHeaderEnd   = ctx->buf.indexOf("\r\n\r\n", partHeaderStart);
        if (partHeaderEnd < 0) return;

        // 提取文件名
        QString filename;
        QByteArray partHeaders = ctx->buf.mid(partHeaderStart, partHeaderEnd - partHeaderStart);
        for (const QByteArray &line : partHeaders.split('\n')) {
            if (line.toLower().startsWith("content-disposition:")) {
                int fn = line.indexOf("filename=");
                if (fn >= 0) {
                    QByteArray rest = line.mid(fn + 9).trimmed();
                    if (rest.startsWith('"')) { rest = rest.mid(1); rest = rest.left(rest.indexOf('"')); }
                    filename = QString::fromUtf8(rest).trimmed();
                    if (filename.endsWith('\r')) filename.chop(1);
                }
            }
        }
        if (filename.isEmpty())
            filename = "upload_" + QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
        filename = QFileInfo(filename).fileName();
        if (filename.isEmpty() || filename.contains("..")) {
            ctx->error = true;
            m_uploads.remove(socket);
            delete ctx;
            sendResponse(socket, 400, "application/json", R"({"ok":false,"msg":"invalid filename"})");
            return;
        }

        // 处理文件名冲突
        QDir().mkpath(ctx->dir);
        QString savePath = ctx->dir + "/" + filename;
        if (QFile::exists(savePath)) {
            QFileInfo fi(filename);
            QString ts = QDateTime::currentDateTime().toString("_yyyyMMdd_hhmmss");
            filename = fi.baseName() + ts + (fi.suffix().isEmpty() ? "" : "." + fi.suffix());
            savePath = ctx->dir + "/" + filename;
        }
        ctx->filename = filename;

        ctx->file = new QFile(savePath);
        if (!ctx->file->open(QIODevice::WriteOnly)) {
            ctx->error = true;
            m_uploads.remove(socket);
            delete ctx;
            sendResponse(socket, 500, "application/json", R"({"ok":false,"msg":"cannot write file"})");
            return;
        }

        // 文件数据从 part 头后面开始
        QByteArray fileDataSoFar = ctx->buf.mid(partHeaderEnd + 4);
        ctx->buf.clear();
        ctx->phase = UploadCtx::FileData;
        feedUpload(socket, fileDataSoFar); // 递归处理已到的文件数据
        return;
    }

    // ── FileData 阶段：写磁盘，尾部窗口检测结束边界 ──
    QByteArray combined = ctx->buf + data;
    int endPos = combined.indexOf(ctx->endMarker);
    if (endPos >= 0) {
        // 找到结束边界，写完剩余文件数据
        if (endPos > 0)
            ctx->file->write(combined.constData(), endPos);
        finalizeUpload(socket, true);
    } else {
        // 还没结束，安全写入，保留尾部窗口
        int safe = combined.size() - ctx->endMarker.size();
        if (safe > 0) {
            ctx->file->write(combined.constData(), safe);
            ctx->buf = combined.mid(safe);
        } else {
            ctx->buf = combined;
        }
    }
}

void HttpServer::finalizeUpload(QTcpSocket *socket, bool success)
{
    UploadCtx *ctx = m_uploads.value(socket, nullptr);
    if (!ctx) return;

    ctx->phase = UploadCtx::Done;
    qint64  written  = ctx->file ? ctx->file->pos() : 0;
    QString filename = ctx->filename;
    if (ctx->file) ctx->file->close();
    m_uploads.remove(socket);
    delete ctx;

    if (success) {
        qInfo() << "[Upload] 已保存:" << filename << "大小:" << written;
        QJsonObject resp;
        resp["ok"]       = true;
        resp["filename"] = filename;
        resp["size"]     = written;
        sendResponse(socket, 200, "application/json",
                     QJsonDocument(resp).toJson(QJsonDocument::Compact));
    } else {
        sendResponse(socket, 500, "application/json", R"({"ok":false,"msg":"upload failed"})");
    }
}

// ──────────────────────────────────────────────
//  工具函数
// ──────────────────────────────────────────────

void HttpServer::sendResponse(QTcpSocket *socket, int statusCode,
                               const QString &contentType, const QByteArray &body)
{
    QString statusText = (statusCode == 200) ? "OK" :
                         (statusCode == 400) ? "Bad Request" :
                         (statusCode == 403) ? "Forbidden" :
                         (statusCode == 404) ? "Not Found" :
                         (statusCode == 413) ? "Payload Too Large" : "Internal Server Error";

    QByteArray header = QString(
        "HTTP/1.1 %1 %2\r\n"
        "Content-Type: %3\r\n"
        "Content-Length: %4\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Connection: close\r\n"
        "\r\n"
    ).arg(statusCode).arg(statusText).arg(contentType).arg(body.size()).toUtf8();

    socket->write(header + body);
    socket->flush();
    connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);
    socket->disconnectFromHost();

}

QString HttpServer::mimeForFilename(const QString &filename)
{
    QString lower = filename.toLower();
    if (lower.endsWith(".jpg") || lower.endsWith(".jpeg")) return "image/jpeg";
    if (lower.endsWith(".png"))  return "image/png";
    if (lower.endsWith(".bmp"))  return "image/bmp";
    if (lower.endsWith(".mp4"))  return "video/mp4";
    if (lower.endsWith(".avi"))  return "video/x-msvideo";
    if (lower.endsWith(".mkv"))  return "video/x-matroska";
    if (lower.endsWith(".mov"))  return "video/quicktime";
    if (lower.endsWith(".mp3"))  return "audio/mpeg";
    if (lower.endsWith(".wav"))  return "audio/wav";
    if (lower.endsWith(".flac")) return "audio/flac";
    if (lower.endsWith(".aac"))  return "audio/aac";
    return "application/octet-stream";
}
