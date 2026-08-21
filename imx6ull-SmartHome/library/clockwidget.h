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

    // 设置字体大小（手动固定模式，与自动缩放互斥）
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

    // 设置自动缩放开关
    void setAutoScale(bool enabled);
    bool autoScale() const { return m_autoScale; }

protected:
    void resizeEvent(QResizeEvent *event) override;

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
    bool m_autoScale;         // 是否自动缩放

    // 手动设置的字体大小（自动缩放关闭时使用）
    int m_manualTimeFontSize;
    int m_manualWeekFontSize;

    void setupUI();
    void initTimer();
    void updateSizes();       // 根据当前尺寸更新字体和布局

    // 辅助函数
    QString getChineseWeekday(int dayOfWeek);
    QString getEnglishWeekday(int dayOfWeek);
    QString getAutoWeekday(int dayOfWeek);
};

#endif // CLOCKWIDGET_H
