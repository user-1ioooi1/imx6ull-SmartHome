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
    // 禁止拷贝和赋值
    ClockWidget(const ClockWidget&) = delete;
    ClockWidget& operator=(const ClockWidget&) = delete;

    // 获取单例实例
    static ClockWidget* instance(QWidget *parent = nullptr);

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

protected:
    // 构造函数声明为 protected（或 private）
    explicit ClockWidget(QWidget *parent = nullptr);
    ~ClockWidget();

private slots:
    void updateTime();

private:
    static ClockWidget* m_instance;  // 静态实例指针

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
