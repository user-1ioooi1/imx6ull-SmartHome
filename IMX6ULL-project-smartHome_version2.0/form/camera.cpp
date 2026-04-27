#include "camera.h"
#include "ui_camera.h"
#include <QDateTime>
#include <QSizePolicy>
#include <QFileDialog>
#include <QImage>

Camera::Camera(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Camera),
    m_photoAlbum(new photoAlbum(this))
{
    ui->setupUi(this);

    ui->stackedWidget->addWidget(m_photoAlbum);

    connect(m_photoAlbum,&photoAlbum::back,[this](){
            ui->takeBtn->show();
            ui->albumBtn->show();
            ui->switchBtn->show();
            if(m_camera)
                m_camera->start();
            ui->stackedWidget->setCurrentIndex(0);
    });

    const QList<QCameraInfo> cameras = QCameraInfo::availableCameras();
    if (cameras.isEmpty()) {
         qDebug() << "没有找到相机设备";
         return;
     }

    QWidget *firstPage = ui->stackedWidget->widget(0);
    if (firstPage) {
        ui->stackedWidget->removeWidget(firstPage);
        delete firstPage;
    }

    m_camera = new QCamera(cameras.first(), this);

    m_viewfinder = new QCameraViewfinder(this);
    ui->stackedWidget->insertWidget(0, m_viewfinder);
    ui->stackedWidget->setCurrentIndex(0);
    m_camera->setViewfinder(m_viewfinder);

    m_imageCapture = new QCameraImageCapture(m_camera, this);

    m_recorder = new QMediaRecorder(m_camera, this);

    /*性能考虑,默认更好
    m_recorder->setContainerFormat("mp4");
    QVideoEncoderSettings videoSettings;
    videoSettings.setCodec("video/x-h264");
    videoSettings.setFrameRate(15.0);
    videoSettings.setBitRate(500000);
    m_recorder->setVideoSettings(videoSettings);

    QAudioEncoderSettings audioSettings;
    audioSettings.setCodec("audio/mpeg, mpegversion=(int)1, layer=(int)3");
    audioSettings.setSampleRate(8000);
    audioSettings.setBitRate(64000);
    m_recorder->setAudioSettings(audioSettings);
*/
    connect(m_recorder, &QMediaRecorder::stateChanged, [this](QMediaRecorder::State state){
            if(state == QMediaRecorder::RecordingState){
                ui->takeBtn->setStyleSheet("image: url(:/icon/icon/录像2.png);");
            }else if(state == QMediaRecorder::StoppedState){
                ui->takeBtn->setStyleSheet("image: url(:/icon/icon/录像.png);");
            }
    });

    // Attach QVideoProbe for web streaming
    m_videoProbe = new QVideoProbe(this);
    if (m_videoProbe->setSource(m_camera)) {
        connect(m_videoProbe, &QVideoProbe::videoFrameProbed,
                this, &Camera::onVideoFrame, Qt::DirectConnection);
        qDebug() << "[Camera] QVideoProbe attached";
    } else {
        qWarning() << "[Camera] QVideoProbe not supported on this backend";
    }
}

Camera::~Camera()
{
    delete ui;
}

void Camera::start()
{
   if (m_camera)
        m_camera->start();
}

void Camera::stop()
{
    if (m_camera)
        m_camera->stop();
}

void Camera::setStreamingEnabled(bool enable)
{
    m_streamingEnabled.store(enable);
    if (!m_camera) return;

    if (enable) {
        // 从摄像头管线断开 viewfinder，停止 YUV→RGB 转换和渲染
        m_camera->setViewfinder(static_cast<QCameraViewfinder*>(nullptr));
        if (m_camera->state() != QCamera::ActiveState)
            m_camera->start();
    } else {
        // 先停相机再接入 viewfinder，保证管线重建时 sink 已就位
        m_camera->stop();
        if (m_viewfinder)
            m_camera->setViewfinder(m_viewfinder);
    }
}

void Camera::onVideoFrame(const QVideoFrame &videoFrame)
{
    if (!m_streamingEnabled.load()) return;

    // 5 fps，降低 IMX6ULL 软件编码压力
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (now - m_lastFrameMs < 200) return;
    m_lastFrameMs = now;

    QVideoFrame frame(videoFrame);
    if (!frame.map(QAbstractVideoBuffer::ReadOnly)) return;

    QImage img;
    QVideoFrame::PixelFormat pf = frame.pixelFormat();
    QImage::Format imgFmt = QVideoFrame::imageFormatFromPixelFormat(pf);

    if (imgFmt != QImage::Format_Invalid) {
        // Qt 能直接映射的格式（RGB32 等）
        img = QImage(frame.bits(), frame.width(), frame.height(),
                     frame.bytesPerLine(), imgFmt).copy();
    } else if (pf == QVideoFrame::Format_YUYV) {
        // YUY2/YUYV：每 4 字节 = 2 像素 [Y0 U Y1 V]，BT.601 整数转换
        int w = frame.width(), h = frame.height(), bpl = frame.bytesPerLine();
        const uchar *src = frame.bits();
        img = QImage(w, h, QImage::Format_RGB32);
        for (int y = 0; y < h; ++y) {
            const uchar *row = src + y * bpl;
            QRgb *dst = reinterpret_cast<QRgb*>(img.scanLine(y));
            for (int x = 0; x < w; x += 2, row += 4) {
                int C0 = row[0]-16, D = row[1]-128, C1 = row[2]-16, E = row[3]-128;
                dst[x] = qRgb(
                    qBound(0, (298*C0 + 409*E          + 128) >> 8, 255),
                    qBound(0, (298*C0 - 100*D - 208*E  + 128) >> 8, 255),
                    qBound(0, (298*C0 + 516*D          + 128) >> 8, 255));
                if (x + 1 < w)
                    dst[x+1] = qRgb(
                        qBound(0, (298*C1 + 409*E         + 128) >> 8, 255),
                        qBound(0, (298*C1 - 100*D - 208*E + 128) >> 8, 255),
                        qBound(0, (298*C1 + 516*D         + 128) >> 8, 255));
            }
        }
    }
    frame.unmap();

    if (img.isNull()) return;

    QByteArray jpegData;
    QBuffer buf(&jpegData);
    buf.open(QIODevice::WriteOnly);
    img.save(&buf, "JPEG", 40); // 质量 40，减少编码时间
    buf.close();

    if (!jpegData.isEmpty()) {
        emit frameReady(jpegData);
    }
}

void Camera::takePhotoBtn_clicked()
{
    if (!m_camera || m_camera->state() != QCamera::ActiveState) {
        qDebug() << "Camera not active";
        return;
    }
    QString fileName = QString("./photo/photo_%1.jpg")
        .arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));
    m_imageCapture->capture(fileName);
}

void Camera::on_albumBtn_clicked()
{
    QString filepath = QFileDialog::getExistingDirectory(this, "choose photo dir", "./photo");
    if(filepath != ""){
        ui->takeBtn->hide();
        ui->albumBtn->hide();
        ui->switchBtn->hide();
        if (m_camera)
            m_camera->stop();
        m_photoAlbum->setFolder(filepath);
        ui->stackedWidget->setCurrentIndex(1);
        m_photoAlbum->showImage();
    }
}

void Camera::recordBtn_clicked()
{
    if(!m_recorder) return;
    if(m_recorder->state() == QMediaRecorder::StoppedState){
        m_isRecording = true;
        ui->switchBtn->setEnabled(false);
        m_recorder->setOutputLocation(QUrl::fromLocalFile(
            QString("./video/recording_%1.mp4")
            .arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"))));
        m_recorder->record();
    } else if(m_recorder->state() == QMediaRecorder::RecordingState){
        m_recorder->stop();
        ui->switchBtn->setEnabled(true);
        m_isRecording = false;
    }
}

void Camera::on_takeBtn_clicked()
{
    if(m_state == shoot){
        takePhotoBtn_clicked();
    } else {
        recordBtn_clicked();
    }
}

void Camera::on_switchBtn_clicked()
{
    if(!m_camera) return;
    m_state = m_state == shoot ? record : shoot;
    if(m_state == shoot){
        m_camera->setCaptureMode(QCamera::CaptureStillImage);
        ui->takeBtn->setStyleSheet("image: url(:/icon/icon/拍摄.png);");
    } else {
        m_camera->setCaptureMode(QCamera::CaptureVideo);
        ui->takeBtn->setStyleSheet("image: url(:/icon/icon/录像.png);");
    }
}
