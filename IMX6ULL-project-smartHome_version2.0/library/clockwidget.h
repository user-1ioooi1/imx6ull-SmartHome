#ifndef CLOCKWIDGET_H
#define CLOCKWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QTimer>
#include <QDateTime>
#include <QVBoxLayout>
#include <QHBoxLayout>

class ClockWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ClockWidget(QWidget *parent = nullptr);
    ~ClockWidget();

    // 设置显示格式
    void setShowSeconds(bool show);
    void setShowDate(bool show);

    // 设置字体大小
    void setTimeFontSize(int size);
    void setWeekFontSize(int size);

    // 设置星期显示格式
    enum WeekdayFormat {
        AutoDetect,      // 自动检测
        Chinese,         // 强制中文
        English,         // 强制英文
        System           // 使用系统格式
    };
    void setWeekdayFormat(WeekdayFormat format);

private slots:
    void updateTime();

private:
    QLabel *m_timeLabel;      // 时间显示
    QLabel *m_weekLabel;      // 星期显示
    QLabel *m_dateLabel;      // 日期显示

    QTimer *m_timer;          // 定时器

    bool m_showSeconds;       // 是否显示秒
    bool m_showDate;          // 是否显示日期
    WeekdayFormat m_weekdayFormat;  // 星期格式

    void setupUI();
    void initTimer();

    // 辅助函数
    QString getChineseWeekday(int dayOfWeek);
    QString getEnglishWeekday(int dayOfWeek);
    QString getAutoWeekday(int dayOfWeek);
};

#endif // CLOCKWIDGET_H
