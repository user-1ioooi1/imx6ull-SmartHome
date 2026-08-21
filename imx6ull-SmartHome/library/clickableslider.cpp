// clickableSlider.cpp
#include "clickableslider.h"

ClickableSlider::ClickableSlider(QWidget *parent)
    : QSlider(parent)
{
    setTracking(false);//信号 void valueChanged(int value) 在移动滑块过程中是连续触发,
                       //设置 setTracking(false) 使信号 void valueChanged(int value) 在滑动过程中不被触发
}

void ClickableSlider::mousePressEvent(QMouseEvent *ev)
{

    int value;

    if (this->orientation() == Qt::Horizontal) {
        // 水平滑动条：使用 x 坐标
        int currentX = ev->pos().x();
        value = (1.0 * currentX) / this->width() * (this->maximum() - this->minimum()) + this->minimum();
    } else {
        // 垂直滑动条：使用 y 坐标（注意：垂直方向通常是从下往上增加）
        int currentY = ev->pos().y();

        // 垂直滑动条：y=0 是顶部，y=height 是底部
        // 如果最小值在底部，最大值在顶部，需要反转
        if (this->invertedAppearance()) {
            // 反转模式：最小值在上，最大值在下
            value = (1.0 * currentY) / this->height() * (this->maximum() - this->minimum()) + this->minimum();
        } else {
            // 正常模式：最小值在下，最大值在上
            value = (1.0 * (this->height() - currentY)) / this->height() * (this->maximum() - this->minimum()) + this->minimum();
        }
    }

    // 确保值在有效范围内
    value = qBound(this->minimum(), value, this->maximum());

    // 设定滑动条位置
    this->setValue(value);

    emit clicked(value); //发送信号

    // 调用父类事件处理
    QSlider::mousePressEvent(ev); //调用父类处理（这会触发sliderMoved等）

}
