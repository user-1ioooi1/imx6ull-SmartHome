#include "camera.h"
#include "ui_camera.h"
#include <QDateTime>
#include <QSizePolicy>
#include <QFileDialog>
#include <QImage>
#include <QTimer>
#include <QFile>


#define DETECT_PERIOD_MS  2000
#define NOCONNECTED_PAGE  0
#define CONNECTED_PAGE 1



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
            if(m_state != NODETECTED)
                m_camera->start();
            ui->stackedWidget->setCurrentIndex(0);
    });

    // 创建 Viewfinder
    if (!m_viewfinder) {
        m_viewfinder = new QCameraViewfinder(this);
    }

    // 检查并设置 stackedWidget
    /*
    QWidget *firstPage = ui->stackedWidget->widget(0);
    if (firstPage) {
        ui->stackedWidget->removeWidget(firstPage);
        delete firstPage;
    }*/

    if (ui->stackedWidget->indexOf(m_viewfinder) == -1) {
        ui->stackedWidget->insertWidget(CONNECTED_PAGE, m_viewfinder);
    }
    ui->stackedWidget->setCurrentIndex(NOCONNECTED_PAGE);

    m_checkTimer = new QTimer(this);
    connect(m_checkTimer, &QTimer::timeout, this, &Camera::onCameraConnection);

    onCameraConnection();
    m_checkTimer->start(DETECT_PERIOD_MS);
}

Camera::~Camera()
{
    delete ui;
}

void Camera::initCamera(const QCameraInfo &selectedCamera){
    qDebug() << "[Camera] 初始化摄像头...";

    // 清理旧资源
    cleanupCamera();

    // 创建新的摄像头对象
    m_camera = new QCamera(selectedCamera, this);
    m_lastDeviceName = selectedCamera.deviceName();

    m_camera->setViewfinder(m_viewfinder);

    // 创建 ImageCapture
    m_imageCapture = new QCameraImageCapture(m_camera, this);

    // 创建 Recorder
    m_recorder = new QMediaRecorder(m_camera, this);
    connect(m_recorder, &QMediaRecorder::stateChanged,
            this, &Camera::onRecordingStateChanged);


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
    m_recorder->setAudioSettings(audioSettings);*/

    // 创建 VideoProbe (用于web视频流)
    m_videoProbe = new QVideoProbe(this);
    if (m_videoProbe->setSource(m_camera)) {
        connect(m_videoProbe, &QVideoProbe::videoFrameProbed,
                this, &Camera::onVideoFrame, Qt::DirectConnection);
        qDebug() << "[Camera] QVideoProbe 已连接";
    } else {
        qWarning() << "[Camera] QVideoProbe 不被当前后端支持";
    }

    updateUIForCameraState(true);

    connect(m_camera, QOverload<QCamera::Error>::of(&QCamera::error), this, &Camera::handleCameraError);

    m_state = SHOOT;

    start();

    qDebug() << "[Camera] 摄像头初始化成功:" << selectedCamera.description()
             << "设备名:" << selectedCamera.deviceName();
};


void Camera::cleanupCamera(){

        if (m_videoProbe) {
            delete m_videoProbe;
            m_videoProbe = nullptr;
        }
        if (m_recorder) {
            delete m_recorder;
            m_recorder = nullptr;
        }
        if (m_imageCapture) {
            delete m_imageCapture;
            m_imageCapture = nullptr;
        }
        if (m_camera) {
            m_camera->deleteLater();
            m_camera = nullptr;
        }

        m_state = NODETECTED;
        updateUIForCameraState(false);
}

void Camera::updateUIForCameraState(bool ready){
    if(ready){
        ui->stackedWidget->setCurrentIndex(CONNECTED_PAGE);
    }else{
        ui->stackedWidget->setCurrentIndex(NOCONNECTED_PAGE);
    }
}

void Camera::handleCameraError(QCamera::Error error){
        qDebug() << "[Camera] 错误发生:" << error;

        if (error == QCamera::CameraError ||
            error == QCamera::ServiceMissingError ||
            error == QCamera::InvalidRequestError) {

            if(m_state == RECORD){
                auto fileUrl = m_recorder->outputLocation();
                QFile::remove(fileUrl.toLocalFile());
                ui->switchBtn->setEnabled(true);
                m_isRecording = false;
            }
            stop();
            cleanupCamera();
        }
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
    if (m_state == NODETECTED) return;
    m_streamingEnabled.store(enable);

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
    if (m_state != SHOOT || m_camera->state() != QCamera::ActiveState) {
        qDebug() << "Camera not SHOOT mode";
        return;
    }

    QString baseDir = QCoreApplication::applicationDirPath();
    QString photoDir = baseDir + "/photo/";
    QString fileName = QString("%1photo_%2.jpg")
            .arg(photoDir)
            .arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));

    qDebug() << "photo save : " << fileName;
    m_imageCapture->capture(fileName);
}

void Camera::on_albumBtn_clicked()
{
    QString filepath = QFileDialog::getExistingDirectory(this, "choose photo dir", "./photo");
    if(filepath != ""){
        ui->takeBtn->hide();
        ui->albumBtn->hide();
        ui->switchBtn->hide();
        if (m_state != NODETECTED) m_camera->stop();
        m_photoAlbum->setFolder(filepath);
        ui->stackedWidget->setCurrentIndex(1);
        m_photoAlbum->showImage();
    }
}

void Camera::recordBtn_clicked()
{
    if(m_state != RECORD) return;
    if(m_recorder->state() == QMediaRecorder::StoppedState){
        m_isRecording = true;
        ui->switchBtn->setEnabled(false);

        QString baseDir = QCoreApplication::applicationDirPath();
        QString videoDir = baseDir + "/video/";
        QString fileName = QString("%1recording_%2")
                .arg(videoDir)
                .arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));

        qDebug() << "recording save : " << fileName;

        m_recorder->setOutputLocation(QUrl::fromLocalFile(fileName));
        m_recorder->record();
    } else if(m_recorder->state() == QMediaRecorder::RecordingState){
        m_recorder->stop();
        ui->switchBtn->setEnabled(true);
        m_isRecording = false;
    }
}

void Camera::on_takeBtn_clicked()
{
    if(m_state == SHOOT){
        takePhotoBtn_clicked();
    } else {
        recordBtn_clicked();
    }
}

void Camera::on_switchBtn_clicked()
{
    if(m_state == NODETECTED) return;
    m_state = (m_state == SHOOT ? RECORD : SHOOT);
    if(m_state == SHOOT){
        m_camera->setCaptureMode(QCamera::CaptureStillImage);
        ui->takeBtn->setStyleSheet("image: url(:/icon/icon/拍摄.png);");
    } else {
        m_camera->setCaptureMode(QCamera::CaptureVideo);
        ui->takeBtn->setStyleSheet("image: url(:/icon/icon/录像.png);");
    }
}


void Camera::onRecordingStateChanged(QMediaRecorder::State state){
    if(state == QMediaRecorder::RecordingState){
        ui->takeBtn->setStyleSheet("image: url(:/icon/icon/录像2.png);");
    }else if(state == QMediaRecorder::StoppedState){
        ui->takeBtn->setStyleSheet("image: url(:/icon/icon/录像.png);");
    }
}

void Camera::onCameraConnection()
{
    if(m_state != NODETECTED){
        return;
    }

    QList<QCameraInfo> currentCameras = QCameraInfo::availableCameras();

    if(currentCameras.empty()){
        qDebug() << "[Camera] 无摄像头";
        return;
    }

    QCameraInfo selectedCam =  currentCameras.first();

    for(auto & cam : currentCameras){ //优先选择上次的摄像头
        if(m_lastDeviceName == cam.deviceName()){
            selectedCam = cam;
        }
    }

    m_lastDeviceName = selectedCam.deviceName();

    initCamera(selectedCam);
}

