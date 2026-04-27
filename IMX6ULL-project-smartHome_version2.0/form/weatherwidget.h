#ifndef WEATHERWIDGET_H
#define WEATHERWIDGET_H

#include <QWidget>
#include "../library/weatherapi.h"
#include "../library/iplocationapi.h"

namespace Ui {
class WeatherWidget;
}

class WeatherWidget : public QWidget
{
    Q_OBJECT

public:
    explicit WeatherWidget(QWidget *parent = nullptr);
    ~WeatherWidget();

private slots:
    void on_refreshButton_clicked();

private:
    Ui::WeatherWidget *ui;
    WeatherApi m_weather;
    IpLocationApi m_location;
    QString cityCode;
    QTimer        m_weatherTimer;
};

#endif // WEATHERWIDGET_H
