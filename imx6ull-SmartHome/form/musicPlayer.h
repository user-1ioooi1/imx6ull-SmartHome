#ifndef MUSICPLAYER_H
#define MUSICPLAYER_H

#include <QWidget>
#include <QStandardItemModel>
#include <QMediaPlayer>
#include <QFileDialog>

namespace Ui {
class musicPlayer;
}

class musicPlayer : public QWidget
{
    Q_OBJECT

public:
    explicit musicPlayer(QWidget *parent = nullptr);
    ~musicPlayer();

private slots:
    void on_openDirBtn_clicked();

    void on_prevBtn_clicked();

    void on_playBtn_clicked();

    void on_nextBtn_clicked();

    void on_songlist_clicked(const QModelIndex &index);

private:
    Ui::musicPlayer *ui;
    QStandardItemModel* m_listMoel;
    QMediaPlayer *m_mediaPlyer;
    QUrl m_curSong;
    const QModelIndex* m_index;
    QList<QUrl> *m_playlist;
    int m_currentIndex = -1;
    QFileDialog* fileDialog;


};

#endif // MUSICPLAYER_H
