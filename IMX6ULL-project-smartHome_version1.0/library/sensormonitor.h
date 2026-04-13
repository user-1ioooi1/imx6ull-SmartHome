#ifndef SENSORMONITOR_H
#define SENSORMONITOR_H

#include <QObject>
#include <QTimer>
#include "../hardware/hardwaremanager.h"

class SensorMonitor : public QObject
{
    Q_OBJECT

public:
    explicit SensorMonitor(QObject *parent = nullptr);

    void start(int intervalMs = 2000);
    void stop();

signals:
    void sensorDataUpdated(const SensorData data);

private slots:
    void onTimeout();

private:
    QTimer m_timer;
};

#endif // SENSORMONITOR_H
