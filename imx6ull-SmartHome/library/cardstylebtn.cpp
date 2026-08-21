#include "cardstylebtn.h"
#include <QFont>

CardStyleBtn::CardStyleBtn(QWidget *parent)
    : QPushButton(parent)
{
    // 初始化动画
    m_scaleAnimation = new QPropertyAnimation(this, "scale", this);
    m_colorAnimation = new QPropertyAnimation(this, "bgColor", this);
    m_borderAnimation = new QPropertyAnimation(this, "borderColor", this);

    m_animationGroup = new QParallelAnimationGroup(this);

    // 配置缩放动画
    m_scaleAnimation->setDuration(110);
    m_scaleAnimation->setEasingCurve(QEasingCurve::OutQuad);

    // 配置颜色动画
    m_colorAnimation->setDuration(180);
    m_colorAnimation->setEasingCurve(QEasingCurve::OutQuad);
    m_borderAnimation->setDuration(180);
    m_borderAnimation->setEasingCurve(QEasingCurve::OutQuad);

    // 加入动画组
    m_animationGroup->addAnimation(m_scaleAnimation);
    m_animationGroup->addAnimation(m_colorAnimation);
    m_animationGroup->addAnimation(m_borderAnimation);

    // 基本设置
    setFixedSize(110, 110);
    setCheckable(false);
    setCursor(Qt::PointingHandCursor);

    // 初始化视觉状态
    updateVisualState();
}

// ==================== 公共方法 ====================

void CardStyleBtn::setIconSource(const QString &path)
{
    m_iconSource = path;
    update();
}

void CardStyleBtn::setTitle(const QString &title)
{
    m_title = title;
    update();
}

void CardStyleBtn::setActiveColor(const QColor &color)
{
    m_activeColor = color;
    // 清空自定义颜色，让它们使用 activeColor 计算
    m_checkedBgColor = QColor();
    m_checkedBorderColor = QColor();
    m_checkedTextColor = QColor();
    m_checkedBarColor = QColor();
    updateVisualState();
}

void CardStyleBtn::setCheckedBgColor(const QColor &color)
{
    m_checkedBgColor = color;
    if (m_checked) {
        updateVisualState();
    }
}

void CardStyleBtn::setCheckedBorderColor(const QColor &color)
{
    m_checkedBorderColor = color;
    if (m_checked) {
        updateVisualState();
    }
}

void CardStyleBtn::setCheckedTextColor(const QColor &color)
{
    m_checkedTextColor = color;
    update();
}

void CardStyleBtn::setCheckedBarColor(const QColor &color)
{
    m_checkedBarColor = color;
    update();
}

void CardStyleBtn::setChecked(bool checked)
{
    if (m_checked == checked) return;

    m_checked = checked;
    updateIconOpacity();
    updateVisualState();

    emit checkStateChanged(m_checked);
}

void CardStyleBtn::setBgColor(const QColor &color)
{
    if (m_bgColor == color) return;
    m_bgColor = color;
    update();
}

QColor CardStyleBtn::bgColor() const
{
    return m_bgColor;
}

void CardStyleBtn::setBorderColor(const QColor &color)
{
    if (m_borderColor == color) return;
    m_borderColor = color;
    update();
}

QColor CardStyleBtn::borderColor() const
{
    return m_borderColor;
}

void CardStyleBtn::setScale(qreal scale)
{
    if (qFuzzyCompare(m_scale, scale)) return;
    m_scale = scale;
    update();
}

qreal CardStyleBtn::scale() const
{
    return m_scale;
}

// ==================== 事件处理 ====================

void CardStyleBtn::enterEvent(QEvent *event)
{
    m_hovered = true;
    updateVisualState();
    startScaleAnimation(1.02);
    QPushButton::enterEvent(event);
}

void CardStyleBtn::leaveEvent(QEvent *event)
{
    m_hovered = false;
    updateVisualState();
    startScaleAnimation(1.0);
    QPushButton::leaveEvent(event);
}

void CardStyleBtn::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_pressed = true;
        startScaleAnimation(0.96);
    }
    QPushButton::mousePressEvent(event);
}

void CardStyleBtn::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_pressed) {
        m_pressed = false;
        startScaleAnimation(m_hovered ? 1.02 : 1.0);
        setChecked(!m_checked);
    }
    QPushButton::mouseReleaseEvent(event);
}

// ==================== 绘制 ====================

void CardStyleBtn::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 应用缩放变换
    painter.save();
    QRectF rect(0, 0, width(), height());
    qreal cx = rect.center().x();
    qreal cy = rect.center().y();
    painter.translate(cx, cy);
    painter.scale(m_scale, m_scale);
    painter.translate(-cx, -cy);

    // 绘制圆角矩形背景
    QPainterPath path;
    path.addRoundedRect(rect, 18, 18);
    painter.fillPath(path, m_bgColor);

    // 绘制边框
    QPen pen;
    pen.setColor(m_borderColor);
    pen.setWidth(m_checked ? 3 : 2);
    painter.setPen(pen);
    painter.drawPath(path);

    painter.restore();

    // ===== 绘制图标和文字（不受缩放影响） =====
    painter.save();

    // 图标
    if (!m_iconSource.isEmpty()) {
        QIcon icon(m_iconSource);
        QPixmap pixmap = icon.pixmap(39, 39);
        qreal opacity = m_checked ? 1.0 : 0.72;
        painter.setOpacity(opacity);
        QRect iconRect((width() - 39) / 2, 15, 39, 39);
        painter.drawPixmap(iconRect, pixmap);
    }

    // 标题文字
    QFont font = painter.font();
    font.setPixelSize(12);
    font.setBold(m_checked);
    painter.setFont(font);
    painter.setOpacity(1.0);

    QColor textColor;
    if (m_checked) {
        textColor = m_checkedTextColor.isValid() ? m_checkedTextColor : m_activeColor;
    } else {
        textColor = QColor(0x64, 0x74, 0x8B);
    }
    painter.setPen(textColor);
    QRect textRect(0, 60, width(), 20);
    painter.drawText(textRect, Qt::AlignHCenter, m_title);

    // ===== 底部指示条（放在按钮底部） =====
    QColor barColor;
    if (m_checked) {
        barColor = m_checkedBarColor.isValid() ? m_checkedBarColor : m_activeColor;
    } else {
        barColor = QColor(0xCB, 0xD5, 0xE1);
    }
    painter.setBrush(barColor);
    painter.setPen(Qt::NoPen);

    // 指示条在底部，距离底边 6px
    int barWidth = 28;
    int barHeight = 4;
    int barX = (width() - barWidth) / 2;
    int barY = height() - 6 - barHeight;
    QRect barRect(barX, barY, barWidth, barHeight);
    painter.drawRoundedRect(barRect, 2, 2);

    painter.restore();
}

void CardStyleBtn::resizeEvent(QResizeEvent *event)
{
    QPushButton::resizeEvent(event);
}

// ==================== 私有方法 ====================

void CardStyleBtn::updateVisualState()
{
    QColor targetBg;
    QColor targetBorder;

    if (m_checked) {
        // 选中状态
        if (m_checkedBgColor.isValid()) {
            targetBg = m_checkedBgColor;
        } else {
            targetBg = m_activeColor.lighter(180);
        }

        if (m_checkedBorderColor.isValid()) {
            targetBorder = m_checkedBorderColor;
        } else {
            targetBorder = QColor::fromRgbF(
                m_activeColor.redF(),
                m_activeColor.greenF(),
                m_activeColor.blueF(),
                0.32
            );
        }
    } else if (m_hovered) {
        targetBg = QColor(0xF3, 0xF6, 0xFB);
        targetBorder = QColor(0xE6, 0xEB, 0xF3);
    } else {
        targetBg = QColor(0xF8, 0xFA, 0xFD);
        targetBorder = QColor(0xE6, 0xEB, 0xF3);
    }

    startColorAnimation(targetBg, targetBorder);
}

void CardStyleBtn::startScaleAnimation(qreal targetScale)
{
    if (qFuzzyCompare(m_scale, targetScale)) return;

    m_scaleAnimation->setEndValue(targetScale);

    if (m_scaleAnimation->state() != QAbstractAnimation::Running) {
        m_scaleAnimation->start();
    } else {
        m_scaleAnimation->setEndValue(targetScale);
    }
}

void CardStyleBtn::startColorAnimation(const QColor &targetBg, const QColor &targetBorder)
{
    bool bgChanged = (m_bgColor != targetBg);
    bool borderChanged = (m_borderColor != targetBorder);

    // 如果颜色完全没变，直接返回
    if (!bgChanged && !borderChanged) {
        return;
    }

    // 如果只有一个变化，另一个保持不变
    QColor startBg = m_bgColor;
    QColor startBorder = m_borderColor;

    // 停止当前动画并记录当前值作为起始值
    if (m_colorAnimation->state() == QAbstractAnimation::Running) {
        m_colorAnimation->stop();
        startBg = m_bgColor;
    }
    if (m_borderAnimation->state() == QAbstractAnimation::Running) {
        m_borderAnimation->stop();
        startBorder = m_borderColor;
    }

    // 设置起始值
    m_colorAnimation->setStartValue(startBg);
    m_borderAnimation->setStartValue(startBorder);

    // 设置目标值
    m_colorAnimation->setEndValue(targetBg);
    m_borderAnimation->setEndValue(targetBorder);

    // 启动需要变化的动画
    if (bgChanged) {
        m_colorAnimation->start();
    }
    if (borderChanged) {
        m_borderAnimation->start();
    }
}

void CardStyleBtn::updateIconOpacity()
{
    // 图标透明度变化由 paintEvent 根据 m_checked 直接计算
    // 这里只需要触发重绘
    update();
}

