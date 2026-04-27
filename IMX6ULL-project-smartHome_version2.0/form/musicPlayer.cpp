#include "musicPlayer.h"
#include "ui_musicPlayer.h"
#include <QDirIterator>

musicPlayer::musicPlayer(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::musicPlayer),
    m_listMoel(new QStandardItemModel(this)),
    m_mediaPlyer(new QMediaPlayer(this)),
    m_playlist(new QList<QUrl>)
{
    ui->setupUi(this);
    ui->songlist->setModel(m_listMoel);
    connect(m_mediaPlyer,&QMediaPlayer::durationChanged,[this](qint64 duration){
        ui->durationLab->setText(QString("%1:%2").arg(duration / 1000 / 60,2,10,QChar('0')).arg(duration/1000 % 60));
        ui->playtimeSlider->setRange(0,duration);

    });

    /*state Change*/
    connect(m_mediaPlyer, &QMediaPlayer::stateChanged, this, [this](QMediaPlayer::State state) {
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

    /*进度改变*/
    connect(m_mediaPlyer,&QMediaPlayer::positionChanged,[this](qint64 duration){
        ui->curPlayLab->setText(QString("%1:%2").arg(duration / 1000 / 60,2,10,QChar('0')).arg(duration/1000 % 60));
        ui->playtimeSlider->setValue(duration);
     });

    connect(ui->playtimeSlider,&QSlider::sliderMoved,m_mediaPlyer,&QMediaPlayer::setPosition);
    connect(ui->playtimeSlider,&ClickableSlider::clicked,m_mediaPlyer,&QMediaPlayer::setPosition);


    ui->volumeSlider->setRange(0,100);

    ui->volumeSlider->setValue(50);


    m_mediaPlyer->setVolume(50);

    connect(ui->volumeSlider,&QSlider::valueChanged,m_mediaPlyer,&QMediaPlayer::setVolume);
}

musicPlayer::~musicPlayer()
{
    delete m_playlist;
    delete ui;
}

void musicPlayer::on_openDirBtn_clicked()
{


    // 1. 清空 UI 显示
    m_listMoel->clear();

    // 2. 清空播放列表
    m_playlist->clear();

    // 3. 重置当前索引
    m_currentIndex = -1;

    // 4. 停止当前播放
    if (m_mediaPlyer->state() != QMediaPlayer::StoppedState) {
        m_mediaPlyer->stop();
    }

    QString filepath = QFileDialog::getExistingDirectory(this,"choose music dir","./music");
    QDirIterator it(filepath,{"*.mp3","*.wav","*.ogg"});
    while(it.hasNext()){
        it.next();
        qInfo() << it.fileName();
        QStandardItem *item = new QStandardItem(it.fileName());
        item->setData(it.fileInfo().canonicalFilePath());
        m_listMoel->appendRow(item);
        m_playlist->append(QUrl::fromLocalFile(item->data(Qt::UserRole + 1).toString()));
    }
}

void musicPlayer::on_prevBtn_clicked()
{
        if(m_playlist->size() == 0)
            return;
        m_currentIndex--;
        if(m_currentIndex < 0){
            m_currentIndex = m_playlist->size() - 1;
        }
        QModelIndex index = m_listMoel->index(m_currentIndex, 0);  // 第0列
        ui->songlist->setCurrentIndex(index);
        m_curSong = m_playlist->at(m_currentIndex);
        if(m_mediaPlyer->state() != QMediaPlayer::StoppedState){
             m_mediaPlyer->stop();
        }
        m_mediaPlyer->setMedia(m_curSong);
        m_mediaPlyer->play();

}

void musicPlayer::on_playBtn_clicked()
{
    if(m_playlist->size() == 0)
        return;
    switch(m_mediaPlyer->state()){
           case QMediaPlayer::PlayingState:
               m_mediaPlyer->pause();
               break;
           case QMediaPlayer::StoppedState:
               m_mediaPlyer->setMedia(m_curSong);
               m_mediaPlyer->play();
               break;
           case QMediaPlayer::PausedState:
               m_mediaPlyer->play();
               break;
           default:
               break;
    }
}

void musicPlayer::on_nextBtn_clicked()
{
       if(m_playlist->size() == 0)
            return;

       m_currentIndex++;
       if(m_currentIndex == m_playlist->size()){
                m_currentIndex = 0;
       }
       m_curSong = m_playlist->at(m_currentIndex);
       QModelIndex index = m_listMoel->index(m_currentIndex, 0);  // 第0列
       ui->songlist->setCurrentIndex(index);

       if(m_mediaPlyer->state() != QMediaPlayer::StoppedState){
            m_mediaPlyer->stop();
       }
       m_mediaPlyer->setMedia(m_curSong);
       m_mediaPlyer->play();

}

void musicPlayer::on_songlist_clicked(const QModelIndex &index)
{
    m_currentIndex = index.row();
    m_curSong = m_playlist->at(m_currentIndex);
    if(m_mediaPlyer->state() != QMediaPlayer::StoppedState){
         m_mediaPlyer->stop();
    }
    m_mediaPlyer->setMedia(m_curSong);
    m_mediaPlyer->play();
}
