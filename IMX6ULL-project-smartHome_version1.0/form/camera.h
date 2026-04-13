#ifndef CAMERA_H
#define CAMERA_H

#include <QWidget>
#include <QCamera>
#include <QCameraViewfinder>
#include <QCameraImageCapture>
#include <QCameraInfo>
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

private slots:
    void on_takePhotoBtn_clicked();

    void on_albumBtn_clicked();

private:
    Ui::Camera *ui;
    QCamera *m_camera = nullptr;
    QCameraViewfinder *m_viewfinder;
    QCameraImageCapture *m_imageCapture;

    photoAlbum* m_photoAlbum;
};

#endif // CAMERA_H
