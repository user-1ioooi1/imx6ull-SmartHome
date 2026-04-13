#include "camera.h"
#include "ui_camera.h"
#include <QDateTime>
#include <QSizePolicy>
#include <QFileDialog>

Camera::Camera(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Camera),
    m_photoAlbum(new photoAlbum(this))
{
    ui->setupUi(this);

    ui->stackedWidget->addWidget(m_photoAlbum);

    connect(m_photoAlbum,&photoAlbum::back,[this](){
            ui->takePhotoBtn->show();
            ui->albumBtn->show();
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

    // 如果第一页是占位widget，删除它
    if (firstPage) {
        ui->stackedWidget->removeWidget(firstPage);
        delete firstPage;
    }

    // 2. 创建相机对象（用第一个）
    m_camera = new QCamera(cameras.first());

    // 3. 创建取景器
    m_viewfinder = new QCameraViewfinder(this);
    ui->stackedWidget->insertWidget(0,m_viewfinder);
    ui->stackedWidget->setCurrentIndex(0);
    m_camera->setViewfinder(m_viewfinder);

    // 4. 创建拍照捕获器
    m_imageCapture = new QCameraImageCapture(m_camera);


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

void Camera::on_takePhotoBtn_clicked()
{
   if (!m_camera || m_camera->state() != QCamera::ActiveState) {
            qDebug() << "take photo err" << endl;
            return;
        }

        // 生成文件名
        QString fileName = QString("./photo/photo_%1.jpg")
           .arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));

        // 拍照
        m_camera->searchAndLock();
        m_imageCapture->capture(fileName);
        m_camera->unlock();

}

void Camera::on_albumBtn_clicked()
{



    QString filepath = QFileDialog::getExistingDirectory(this,"choose photo dir","./photo");

    if(filepath != ""){
        ui->takePhotoBtn->hide();

        ui->albumBtn->hide();

        if (m_camera)
            m_camera->stop();

        m_photoAlbum->setFolder(filepath);

        ui->stackedWidget->setCurrentIndex(1);

        m_photoAlbum->showImage();

    }


}
