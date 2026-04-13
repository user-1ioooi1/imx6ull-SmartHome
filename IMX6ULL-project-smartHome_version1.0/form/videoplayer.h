#ifndef VIDEOPLAYER_H
#define VIDEOPLAYER_H

#include <QWidget>
#include <QMediaPlayer>
#include <QVideoWidget>

namespace Ui {
class videoPlayer;
}

class videoPlayer : public QWidget
{
    Q_OBJECT

public:
    explicit videoPlayer(QWidget *parent = nullptr);
    ~videoPlayer();
    void showWidget();

    void hideWidget();



private slots:
    void on_playBtn_clicked();

    void on_fileBtn_clicked();

    void on_prevBtn_clicked();

    void on_nextBtn_clicked();

private:
    Ui::videoPlayer *ui;
    QMediaPlayer *m_mediaPlayer;
    int m_currentIndex = -1;
    QStringList *m_videoList;


};

#endif // VIDEOPLAYER_H
