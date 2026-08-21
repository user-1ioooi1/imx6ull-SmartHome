#include "clockwidget.h"


#include "clockwidget.h"
#include <QResizeEvent>
#include <QDebug>

ClockWidget::ClockWidget(QWidget *parent)
    : QWidget(parent)
    , m_showSeconds(false)
    , m_showDate(true)
    , m_weekdayFormat(AutoDetect)
    , m_autoScale(true)
    , m_manualTimeFontSize(32)
    , m_manualWeekFontSize(18)
{
    setupUI();
    initTimer();
}

ClockWidget::~ClockWidget()
{
    if (m_timer) {
        m_timer->stop();
    }
}

void ClockWidget::setupUI()
{
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setSpacing(8);
    mainLayout->setContentsMargins(12, 6, 12, 6);

    // ===== 日期 =====
    m_dateLabel = new QLabel(this);
    m_dateLabel->setAlignment(Qt::AlignCenter);

    // ===== 星期 =====
    m_weekLabel = new QLabel(this);
    m_weekLabel->setAlignment(Qt::AlignCenter);

    // ===== 时间 =====
    m_timeLabel = new QLabel(this);
    m_timeLabel->setAlignment(Qt::AlignCenter);


    // ===== 分隔符 "|" =====
    QLabel *sepLabel1 = new QLabel(this);
    sepLabel1->setText("|");
    sepLabel1->setAlignment(Qt::AlignCenter);
    sepLabel1->setStyleSheet(
        "QLabel {"
        "    font-size: 28px;"
        "    color: #718096;"
        "}"
    );

    // ===== 分隔符 "|" =====
    QLabel *sepLabel2 = new QLabel(this);
    sepLabel2->setText("|");
    sepLabel2->setAlignment(Qt::AlignCenter);
    sepLabel2->setStyleSheet(
        "QLabel {"
        "    font-size: 28px;"
        "    color: #718096;"
        "}"
    );

    // 添加到水平布局（顺序：日期 | 星期 | 时间）
    mainLayout->addWidget(m_dateLabel);
    mainLayout->addWidget(sepLabel1);
    mainLayout->addWidget(m_weekLabel);
    mainLayout->addWidget(sepLabel2);
    mainLayout->addWidget(m_timeLabel);

    // 时间占主要空间，日期和星期均等
    mainLayout->setStretchFactor(m_dateLabel, 1);
    mainLayout->setStretchFactor(sepLabel1, 0);
    mainLayout->setStretchFactor(m_weekLabel, 1);
    mainLayout->setStretchFactor(sepLabel2, 0);
    mainLayout->setStretchFactor(m_timeLabel, 4);

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    updateSizes();
}
void ClockWidget::initTimer()
{
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &ClockWidget::updateTime);
    m_timer->start(1000);
    updateTime();
}

void ClockWidget::updateTime()
{
    QDateTime now = QDateTime::currentDateTime();

    // 更新时间显示
    QString timeFormat = m_showSeconds ? "hh:mm:ss" : "hh:mm";
    m_timeLabel->setText(now.toString(timeFormat));

    // 更新星期显示（根据格式设置）
    QString weekStr;
    switch (m_weekdayFormat) {
    case Chinese:
        weekStr = getChineseWeekday(now.date().dayOfWeek());
        break;
    case English:
        weekStr = getEnglishWeekday(now.date().dayOfWeek());
        break;
    case System:
        weekStr = now.toString("dddd");
        break;
    case AutoDetect:
    default:
        weekStr = getAutoWeekday(now.date().dayOfWeek());
        break;
    }
    m_weekLabel->setText(weekStr);

    // 更新日期显示
    if (m_showDate) {
        QDate date = now.date();
        m_dateLabel->setText(QString("%1年%2月%3日")
            .arg(date.year())
            .arg(date.month())
            .arg(date.day()));
        m_dateLabel->setVisible(true);
    } else {
        m_dateLabel->setVisible(false);
    }
}

void ClockWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (m_autoScale) {
        updateSizes();  // 自动缩放模式下，每次大小变化重新计算
    }
}

void ClockWidget::updateSizes()
{
    if (!m_autoScale) {
        // 手动模式：使用 setTimeFontSize / setWeekFontSize 设置的值
        setTimeFontSize(m_manualTimeFontSize);
        setWeekFontSize(m_manualWeekFontSize);
        return;
    }

    int w = width();
    int h = height();

    // 如果控件还没显示或大小为0，跳过
    if (w <= 0 || h <= 0) {
        return;
    }

    // 根据宽度和高度取较小值作为基准
    int baseSize = qMin(w, h);

    // 如果宽度远大于高度（横条形布局），以高度为准
    if (w > h * 2) {
        baseSize = h;
    }

    // 如果高度远大于宽度（竖条形布局），以宽度为准
    if (h > w * 2) {
        baseSize = w;
    }

    // 计算各字体大小（比例可自行调整）
    int timeSize = qMax(16, baseSize / 4);      // 时间最大
    int weekSize = qMax(12, baseSize / 8);
    int dateSize = qMax(10, baseSize / 12);     // 日期最小

    // 设置时间字体
    m_timeLabel->setStyleSheet(QString(
        "QLabel {"
        "    font-size: %1px;"
        "    font-weight: bold;"
        "    color: #2c3e50;"
        "}"
    ).arg(timeSize));

    // 设置星期字体
    m_weekLabel->setStyleSheet(QString(
        "QLabel {"
        "    font-size: %1px;"
        "    color: #2D3748;"
        "}"
    ).arg(weekSize));

    // 设置日期字体
    m_dateLabel->setStyleSheet(QString(
        "QLabel {"
        "    font-size: %1px;"
        "    color: #4A5568;"
        "}"
    ).arg(dateSize));

    // 动态调整布局间距和边距
    int margin = qMax(5, baseSize / 20);
    int spacing = qMax(2, baseSize / 30);

    QVBoxLayout *layout = qobject_cast<QVBoxLayout*>(this->layout());
    if (layout) {
        layout->setContentsMargins(margin, margin, margin, margin);
        layout->setSpacing(spacing);
    }
}

QString ClockWidget::getChineseWeekday(int dayOfWeek)
{
    static const char* const weeks[] = {"星期一", "星期二", "星期三",
                                        "星期四", "星期五", "星期六", "星期日"};
    if (dayOfWeek >= 1 && dayOfWeek <= 7) {
        return weeks[dayOfWeek - 1];
    }
    return QString();
}

QString ClockWidget::getEnglishWeekday(int dayOfWeek)
{
    static const char* const weeks[] = {"Monday", "Tuesday", "Wednesday",
                                        "Thursday", "Friday", "Saturday", "Sunday"};
    if (dayOfWeek >= 1 && dayOfWeek <= 7) {
        return weeks[dayOfWeek - 1];
    }
    return QString();
}

QString ClockWidget::getAutoWeekday(int dayOfWeek)
{
    static bool isChinese = []() {
        // 检查环境变量
        QString lang = QString::fromLocal8Bit(qgetenv("LANG"));
        if (lang.contains("zh_CN") || lang.contains("zh_TW")) {
            return true;
        }
        // 实际测试
        QDate testDate(2024, 1, 1);
        return testDate.toString("dddd") == "星期一";
    }();

    if (isChinese) {
        return getChineseWeekday(dayOfWeek);
    } else {
        return getEnglishWeekday(dayOfWeek);
    }
}

void ClockWidget::setShowSeconds(bool show)
{
    m_showSeconds = show;
    updateTime();
}

void ClockWidget::setShowDate(bool show)
{
    m_showDate = show;
    updateTime();
}

void ClockWidget::setTimeFontSize(int size)
{
    m_manualTimeFontSize = size;
    if (!m_autoScale) {
        QString style = QString(
            "QLabel {"
            "    font-size: %1px;"
            "    font-weight: bold;"
            "    color: #2c3e50;"
            "    font-family: 'Consolas', 'Microsoft YaHei';"
            "}"
        ).arg(size);
        m_timeLabel->setStyleSheet(style);
    }
}

void ClockWidget::setWeekFontSize(int size)
{
    m_manualWeekFontSize = size;
    if (!m_autoScale) {
        QString style = QString(
            "QLabel {"
            "    font-size: %1px;"
            "    color: #7f8c8d;"
            "    font-family: 'Microsoft YaHei';"
            "}"
        ).arg(size);
        m_weekLabel->setStyleSheet(style);
    }
}

void ClockWidget::setWeekdayFormat(WeekdayFormat format)
{
    m_weekdayFormat = format;
    updateTime();
}

void ClockWidget::setAutoScale(bool enabled)
{
    m_autoScale = enabled;
    if (enabled) {
        updateSizes();  // 开启自动缩放时立即更新
    } else {
        // 关闭自动缩放时，恢复到手动设置的字体大小
        setTimeFontSize(m_manualTimeFontSize);
        setWeekFontSize(m_manualWeekFontSize);
    }
}
