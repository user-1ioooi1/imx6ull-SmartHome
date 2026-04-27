#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QJsonObject>
#include <QList>
#include <QHash>
#include <QFile>
#include "../hardware/hardwaremanager.h"
#include "../library/sensormonitor.h"

class HttpServer : public QObject
{
    Q_OBJECT

public:
    explicit HttpServer(QObject *parent = nullptr);

    bool start(quint16 port = 8080);
    void stop();

    void updateSensorData(const SensorData &data);

signals:
    void cameraStreamingChanged(bool enabled);

public slots:
    void onCameraFrame(const QByteArray &jpegData);

private slots:
    void onNewConnection();
    void onReadyRead();

private:
    struct UploadCtx {
        enum Phase { Init, FileData, Done } phase = Init;
        QFile      *file          = nullptr;
        qint64      contentLength = 0;
        QByteArray  boundary;
        QByteArray  endMarker;  // "\r\n--boundary--"
        QByteArray  buf;        // Init 阶段积累 part 头；FileData 阶段保存尾部窗口
        QString     filename;
        QString     dir;
        bool        error = false;
        ~UploadCtx() { delete file; }
    };

    void handleRequest(QTcpSocket *socket, const QByteArray &request);
    void sendResponse(QTcpSocket *socket, int statusCode,
                      const QString &contentType, const QByteArray &body);

    void handleGetStatus(QTcpSocket *socket);
    void handlePostControl(QTcpSocket *socket, const QByteArray &body);
    void handleGetDashboard(QTcpSocket *socket);
    void handleCameraStream(QTcpSocket *socket);
    void handleMediaList(QTcpSocket *socket, const QString &type);
    void handleMediaFile(QTcpSocket *socket, const QString &type,
                         const QString &filename, const QByteArray &request);

    void beginUpload(QTcpSocket *socket, const QByteArray &headers,
                     const QByteArray &partialBody, const QString &type,
                     qint64 contentLength);
    void feedUpload(QTcpSocket *socket, const QByteArray &data);
    void finalizeUpload(QTcpSocket *socket, bool success);

    static QString mimeForFilename(const QString &filename);

    QTcpServer  *m_server;
    SensorData   m_lastSensorData;
    QList<QTcpSocket*>             m_streamSockets;
    QHash<QTcpSocket*, QByteArray> m_pendingRequests;
    QHash<QTcpSocket*, UploadCtx*> m_uploads;

    static const QStringList VALID_COMMANDS;
    static const qint64      MAX_UPLOAD_SIZE;
};

#endif // HTTPSERVER_H
