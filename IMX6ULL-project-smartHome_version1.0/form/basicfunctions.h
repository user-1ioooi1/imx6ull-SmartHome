#ifndef BASICFUNCTIONS_H
#define BASICFUNCTIONS_H

#include <QWidget>
#include <QTimer>
#include "../hardware/hardwaremanager.h"
#include "../library/sensormonitor.h"
#include "../library/clockwidget.h"

namespace Ui {
class basicFunctions;
}

class basicFunctions : public QWidget
{
    Q_OBJECT

public:
    explicit basicFunctions(QWidget *parent = nullptr);
    ~basicFunctions();

    void closeSensorTimer();
    void openSensorTimer();

private slots:
    void on_ledBtn_clicked();
    void on_beepBtn_clicked();
    void on_doorLockBtn_clicked();
    void onSensorDataUpdated(const SensorData &data);

private:
    void btnIconChange(QLabel *btnStateIcon,bool on);
    Ui::basicFunctions *ui;
    QTimer        m_beepTimer;
    SensorMonitor m_sensorMonitor;
    bool          m_ledState  = false;
    bool          m_relayState = false;
};

#endif // BASICFUNCTIONS_H
