#include "clockwidget.h"

// 初始化静态成员
ClockWidget* ClockWidget::m_instance = nullptr;

ClockWidget::ClockWidget(QWidget *parent)
    : QWidget(parent)
    , m_showSeconds(true)
    , m_showDate(true)
    , m_weekdayFormat(AutoDetect)
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

ClockWidget* ClockWidget::instance(QWidget *parent)
{
    if (m_instance == nullptr) {
        m_instance = new ClockWidget(parent);
    }
    return m_instance;
}

void ClockWidget::setupUI()
{
    // 创建布局
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(5);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    // 创建时间显示标签
    m_timeLabel = new QLabel(this);
    m_timeLabel->setAlignment(Qt::AlignCenter);
    m_timeLabel->setStyleSheet(
        "QLabel {"
        "    font-size: 32px;"
        "    font-weight: bold;"
        "    color: #2c3e50;"
        "}"
    );

    // 创建星期显示标签
    m_weekLabel = new QLabel(this);
    m_weekLabel->setAlignment(Qt::AlignCenter);
    m_weekLabel->setStyleSheet(
        "QLabel {"
        "    font-size: 18px;"
        "    color: #7f8c8d;"
        "}"
    );

    // 创建日期显示标签
    m_dateLabel = new QLabel(this);
    m_dateLabel->setAlignment(Qt::AlignCenter);
    m_dateLabel->setStyleSheet(
        "QLabel {"
        "    font-size: 14px;"
        "    color: #95a5a6;"
        "}"
    );

    // 添加到布局
    mainLayout->addWidget(m_timeLabel);
    mainLayout->addWidget(m_weekLabel);
    mainLayout->addWidget(m_dateLabel);

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
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

QString ClockWidget::getChineseWeekday(int dayOfWeek)
{
    static const char* const weeks[] = {"星期一", "星期二", "星期三",
                                        "星期四", "星期五", "星期六", "星期日"};
    return weeks[dayOfWeek - 1];
}

QString ClockWidget::getEnglishWeekday(int dayOfWeek)
{
    static const char* const weeks[] = {"Monday", "Tuesday", "Wednesday",
                                        "Thursday", "Friday", "Saturday", "Sunday"};
    return weeks[dayOfWeek - 1];
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

void ClockWidget::setWeekFontSize(int size)
{
    QString style = QString(
        "QLabel {"
        "    font-size: %1px;"
        "    color: #7f8c8d;"
        "    font-family: 'Microsoft YaHei';"
        "}"
    ).arg(size);
    m_weekLabel->setStyleSheet(style);
}

void ClockWidget::setWeekdayFormat(WeekdayFormat format)
{
    m_weekdayFormat = format;
    updateTime();
}
