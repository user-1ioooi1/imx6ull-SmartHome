#include "photoalbum.h"
#include "ui_photoalbum.h"
#include <QDebug>
#include <QDirIterator>

photoAlbum::photoAlbum(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::photoAlbum),
    m_imageList(new QStringList)
{
    ui->setupUi(this);

    ui->pictureArea->setStyleSheet("background-color: #1e1e1e;");
    ui->pictureArea->setScaledContents(true);  // 关键！图片自动缩放,适应 Label


}

photoAlbum::~photoAlbum()
{
    delete m_imageList;
    delete ui;
}

void photoAlbum::loadImage(int index){

    if (index < 0 || index >= m_imageList->size()) return;
    m_currentIndex = index;
    loadImage(m_imageList->operator[](index));
    //updateButtons();
}

void photoAlbum::loadImage(const QString &filePath){
        m_originalPixmap.load(filePath);
        if (!m_originalPixmap.isNull()) {
            ui->pictureArea->setPixmap(m_originalPixmap);
            ui->pictureArea->resize(m_originalPixmap.size());
            qDebug() << "加载图片:" << filePath;
        } else {
            qDebug() << "加载失败:" << filePath;
        }
}


void photoAlbum::clearImage(){

    ui->pictureArea->clear();
    m_originalPixmap = QPixmap();
}

void photoAlbum::setFolder(const QString &folderPath)
{
    QDir dir(folderPath);
    if (!dir.exists()) {
           qDebug() << "文件夹不存在:" << folderPath;
           return;
    }

       // 图片格式过滤
    QStringList filters;
    filters << "*.jpg" << "*.jpeg" << "*.png" << "*.bmp" << "*.gif";
    dir.setNameFilters(filters);
    dir.setSorting(QDir::Name);  // 按名字排序

    QFileInfoList fileList = dir.entryInfoList();
    m_imageList->clear();

    for (const QFileInfo &fileInfo : fileList) {
         m_imageList->append(fileInfo.absoluteFilePath());
    }

    qDebug() << "找到" << m_imageList->size() << "张图片";
}


void photoAlbum::on_prevBtn_clicked()
{
    if (m_currentIndex > 0) {
           loadImage(m_currentIndex - 1);
     }
}

void photoAlbum::on_nextBtn_clicked()
{
    if (m_currentIndex < m_imageList->size() - 1) {
            loadImage(m_currentIndex + 1);
      }
}

void photoAlbum::on_backBtn_clicked()
{
    clearImage();
    emit back();
}

void photoAlbum::showImage(){
    loadImage(0);
    m_currentIndex = 0;
}
