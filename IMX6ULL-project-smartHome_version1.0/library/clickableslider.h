// clickableSlider.h
#ifndef CLICKABLESLIDER_H
#define CLICKABLESLIDER_H

#include <QSlider>
#include <QMouseEvent>

class ClickableSlider : public QSlider
{
    Q_OBJECT
public:
    explicit ClickableSlider(QWidget *parent = nullptr);




protected:
    void mousePressEvent(QMouseEvent *ev) override;



signals:
    void clicked(int value);  // 点击信号

};

#endif // CLICKABLESLIDER_H
