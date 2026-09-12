#include "toastmessage.h"
#include <QPainter>
#include <QHBoxLayout>
#include <QScreen>
#include <QGuiApplication>

ToastMessage::ToastMessage(QWidget* parent)
    : QWidget(parent, Qt::ToolTip | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);

    m_label = new QLabel(this);
    m_label->setAlignment(Qt::AlignCenter);
    m_label->setWordWrap(true);
    m_label->setStyleSheet("color: white; font-size: 14px; font-weight: 500;");

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(24, 14, 24, 14);
    layout->addWidget(m_label);

    m_opacity = new QGraphicsOpacityEffect(this);
    m_opacity->setOpacity(0.0);
    setGraphicsEffect(m_opacity);

    m_anim = new QPropertyAnimation(m_opacity, "opacity", this);
    m_anim->setDuration(200);
}

void ToastMessage::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // 黑框：半透明黑底 + 圆角
    p.setBrush(QColor(20, 20, 20, 230));
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(rect(), 8, 8);
}

void ToastMessage::showToast(QWidget* parent,
                             const QString& text,
                             Level level,
                             int durationMs)
{
    auto* toast = new ToastMessage(parent);
    toast->m_label->setText(text);

    // 根据级别设置边框颜色（可选）
    QString color;
    switch (level) {
    case Info:    color = "#4A90E2"; break;
    case Warning: color = "#F5A623"; break;
    case Error:   color = "#E74C3C"; break;
    }

    // 计算大小
    toast->adjustSize();
    QSize sz = toast->size();
    sz.setWidth(qMin(sz.width(), 420));
    toast->resize(sz);

    // 定位到父窗口中央偏上
    QWidget* anchor = parent ? parent->window() : nullptr;
    QPoint center;
    if (anchor) {
        // mapToGlobal 需要传窗口内的局部坐标
        center = anchor->mapToGlobal(QPoint(anchor->width() / 2, anchor->height() / 4));
    } else {
        // 没有父窗口时，用屏幕中央偏上
        QRect screen = QGuiApplication::primaryScreen()->geometry();
        center = QPoint(screen.center().x(), screen.top() + screen.height() / 4);
    }

    // toast 以自身中心对齐到 center
    toast->move(center.x() - toast->width() / 2,
                center.y() - toast->height() / 2);

    toast->show();
    toast->raise();

    // 淡入
    toast->m_anim->setStartValue(0.0);
    toast->m_anim->setEndValue(1.0);
    toast->m_anim->start();

    // 定时淡出并销毁
    QTimer::singleShot(durationMs, toast, [toast]() {
        auto* fade = new QPropertyAnimation(toast->m_opacity, "opacity", toast);
        fade->setDuration(300);
        fade->setStartValue(1.0);
        fade->setEndValue(0.0);
        QObject::connect(fade, &QPropertyAnimation::finished,
                         toast, &QObject::deleteLater);
        fade->start(QAbstractAnimation::DeleteWhenStopped);
    });
}
