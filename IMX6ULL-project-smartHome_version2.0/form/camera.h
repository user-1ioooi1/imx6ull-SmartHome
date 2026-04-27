#ifndef CAMERA_H
#define CAMERA_H

#include <QWidget>
#include <QCamera>
#include <QCameraViewfinder>
#include <QCameraImageCapture>
#include <QCameraInfo>
#include <QMediaRecorder>
#include <QVideoProbe>
#include <QBuffer>
#include <atomic>
#include "photoalbum.h"

namespace Ui {
class Camera;
}

class Camera : public QWidget
{
    Q_OBJECT

public:
    explicit Camera(QWidget *parent = nullptr);
    ~Camera();

    void start();
    void stop();
    bool isStreamingEnabled() const { return m_streamingEnabled.load(); }

signals:
    void frameReady(const QByteArray &jpegData);

public slots:
    void setStreamingEnabled(bool enable);

private slots:
    void on_albumBtn_clicked();
    void on_takeBtn_clicked();
    void on_switchBtn_clicked();
    void onVideoFrame(const QVideoFrame &frame);

private:
    enum state{
        record = 0,
        shoot
    };
    void takePhotoBtn_clicked();
    void recordBtn_clicked();

    Ui::Camera *ui;
    QCamera *m_camera = nullptr;
    QCameraViewfinder *m_viewfinder = nullptr;
    QCameraImageCapture *m_imageCapture = nullptr;
    QMediaRecorder *m_recorder = nullptr;
    QVideoProbe *m_videoProbe = nullptr;
    bool m_isRecording = false;
    photoAlbum* m_photoAlbum;
    state m_state = shoot;

    std::atomic<bool> m_streamingEnabled{false};
    qint64 m_lastFrameMs = 0;
};

#endif // CAMERA_H
