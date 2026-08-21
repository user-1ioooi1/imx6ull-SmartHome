#ifndef CardStyleBtn_H
#define CardStyleBtn_H

#include <QPushButton>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QColor>
#include <QPainter>
#include <QPainterPath>
#include <QIcon>
#include <QMouseEvent>

class CardStyleBtn : public QPushButton
{
    Q_OBJECT
    Q_PROPERTY(qreal scale READ scale WRITE setScale)
    Q_PROPERTY(QColor bgColor READ bgColor WRITE setBgColor)
    Q_PROPERTY(QColor borderColor READ borderColor WRITE setBorderColor)

public:
    explicit CardStyleBtn(QWidget *parent = nullptr);

    // 基础设置
    void setIconSource(const QString &path);
    void setTitle(const QString &title);

    // 颜色设置
    void setActiveColor(const QColor &color);           // 主题色（影响选中状态）
    void setCheckedBgColor(const QColor &color);        // 自定义选中背景色
    void setCheckedBorderColor(const QColor &color);    // 自定义选中边框色
    void setCheckedTextColor(const QColor &color);      // 自定义选中文字色
    void setCheckedBarColor(const QColor &color);       // 自定义选中指示条色

    // 重写 setChecked
    void setChecked(bool checked);

    // 颜色属性（供动画使用）
    void setBgColor(const QColor &color);
    QColor bgColor() const;
    void setBorderColor(const QColor &color);
    QColor borderColor() const;

    // 缩放属性（供动画使用）
    void setScale(qreal scale);
    qreal scale() const;

signals:
    void checkStateChanged(bool checked);

protected:
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void updateVisualState();
    void startScaleAnimation(qreal targetScale);
    void startColorAnimation(const QColor &targetBg, const QColor &targetBorder);
    void updateIconOpacity();

private:
    // 成员变量
    QString m_iconSource;
    QString m_title;
    QColor m_activeColor = QColor(0x4A, 0x90, 0xE2);  // 默认蓝色

    // 自定义颜色（用于选中状态精确控制）
    QColor m_checkedBgColor;      // 无效时使用 activeColor.lighter(180)
    QColor m_checkedBorderColor;  // 无效时使用 activeColor 半透明
    QColor m_checkedTextColor;    // 无效时使用 activeColor
    QColor m_checkedBarColor;     // 无效时使用 activeColor

    // 状态标志
    bool m_checked = false;
    bool m_hovered = false;
    bool m_pressed = false;

    // 当前颜色（由动画驱动）
    QColor m_bgColor = QColor(0xF8, 0xFA, 0xFD);
    QColor m_borderColor = QColor(0xE6, 0xEB, 0xF3);

    // 当前缩放（由动画驱动）
    qreal m_scale = 1.0;

    // 动画对象
    QPropertyAnimation *m_scaleAnimation = nullptr;
    QPropertyAnimation *m_colorAnimation = nullptr;
    QPropertyAnimation *m_borderAnimation = nullptr;
    QParallelAnimationGroup *m_animationGroup = nullptr;
};

#endif // CARDSTYLEBTN_H
