#include "videoplayer.h"
#include "ui_videoplayer.h"
#include <QFileDialog>
videoPlayer::videoPlayer(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::videoPlayer),
    m_mediaPlayer(new QMediaPlayer(this))
{
    ui->setupUi(this);
    ui->m_videoWidget->setAspectRatioMode(Qt::KeepAspectRatio);
    m_mediaPlayer->setVideoOutput(ui->m_videoWidget);

    //视频时间改变
    connect(m_mediaPlayer,&QMediaPlayer::durationChanged,[this](qint64 duration){
        ui->durationLab->setText(QString("%1:%2").arg(duration / 1000 / 60,2,10,QChar('0')).arg(duration/1000 % 60));
        ui->playtimeSlider->setRange(0,duration);

    });

    /*进度改变*/

    connect(m_mediaPlayer,&QMediaPlayer::positionChanged,[this](qint64 duration){
        ui->curtimeLab->setText(QString("%1:%2").arg(duration / 1000 / 60,2,10,QChar('0')).arg(duration/1000 % 60));
        ui->playtimeSlider->setValue(duration);
     });

    /*state Change*/
    connect(m_mediaPlayer, &QMediaPlayer::stateChanged, this, [this](QMediaPlayer::State state) {
        if (state == QMediaPlayer::PlayingState) {
            // 正在播放
            ui->playBtn->setStyleSheet("image: url(:/icon/icon/播放.png);");
        } else if (state == QMediaPlayer::PausedState) {
            // 已暂停
            ui->playBtn->setStyleSheet("image: url(:/icon/icon/暂停.png);");
        } else if (state == QMediaPlayer::StoppedState) {
            // 已停止
            ui->playBtn->setStyleSheet("image: url(:/icon/icon/暂停.png);");
        }
    });


    connect(ui->playtimeSlider,&QSlider::sliderMoved,m_mediaPlayer,&QMediaPlayer::setPosition); //只有在用户主动拖动滑块时才触发
    connect(ui->playtimeSlider,&ClickableSlider::clicked,m_mediaPlayer,&QMediaPlayer::setPosition); // slidermoved + clicked instead of valuechanged防止循环卡顿

    ui->volumeSlider->setRange(0,100);
    ui->volumeSlider->setValue(50);
    m_mediaPlayer->setVolume(50);
    connect(ui->volumeSlider,&ClickableSlider::valueChanged,m_mediaPlayer,&QMediaPlayer::setVolume);
}

videoPlayer::~videoPlayer()
{
    m_mediaPlayer->stop();
    delete ui;
}


void videoPlayer::showWidget(){
    //m_videoWidget
    ui->m_videoWidget->show();
}

void videoPlayer::hideWidget(){
    ui->m_videoWidget->hide();
}



void videoPlayer::on_playBtn_clicked()
{
    switch(m_mediaPlayer->state()){
           case QMediaPlayer::PlayingState:
               m_mediaPlayer->pause();
               break;
           case QMediaPlayer::StoppedState:
               m_mediaPlayer->play();
               break;
           case QMediaPlayer::PausedState:
               m_mediaPlayer->play();
               break;
           default:
               break;
    }
}

void videoPlayer::on_fileBtn_clicked()
{

    m_videoList.clear();

    QString folderPath = QFileDialog::getExistingDirectory(this,"choose video dir","./video");
    QDir dir(folderPath);
    QStringList filters;
    filters << "*.mp4" << "*.avi";
    dir.setNameFilters(filters);
    dir.setSorting(QDir::Name);  // 按名字排序

    QFileInfoList fileList = dir.entryInfoList();
    for (const QFileInfo &fileInfo : fileList) {
         m_videoList.append(fileInfo.absoluteFilePath());
    }

    qDebug() << "找到" << m_videoList.size() << "条视频";
    if(m_videoList.size() != 0){
        m_currentIndex = 0;
        m_mediaPlayer->setMedia(QUrl::fromLocalFile(m_videoList[m_currentIndex]));
        m_mediaPlayer->play();
    }

}

void videoPlayer::on_prevBtn_clicked()
{
    if (m_currentIndex > 0) {
        m_currentIndex -= 1;
        m_mediaPlayer->stop();
        m_mediaPlayer->setMedia(QUrl::fromLocalFile(m_videoList[m_currentIndex]));
     }
}

void videoPlayer::on_nextBtn_clicked()
{
    if (m_currentIndex < m_videoList.size() - 1) {
           m_currentIndex += 1;
           m_mediaPlayer->stop();
           m_mediaPlayer->setMedia(QUrl::fromLocalFile(m_videoList[m_currentIndex]));
     }
}
