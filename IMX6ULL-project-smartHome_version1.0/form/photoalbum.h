#ifndef PHOTOALBUM_H
#define PHOTOALBUM_H

#include <QWidget>
#include <QPixmap>

namespace Ui {
class photoAlbum;
}

class photoAlbum : public QWidget
{
    Q_OBJECT

public:
    explicit photoAlbum(QWidget *parent = nullptr);
    ~photoAlbum();

    void loadImage(int index);
    void loadImage(const QString &filePath);

    void clearImage();

    void setFolder(const QString &folderPath);

    void showImage();

signals:
    void back();

private slots:
    void on_prevBtn_clicked();

    void on_nextBtn_clicked();

    void on_backBtn_clicked();

private:
    Ui::photoAlbum *ui;
    QPixmap m_originalPixmap;
    int m_currentIndex = -1;
    QStringList *m_imageList;

};

#endif // PHOTOALBUM_H
