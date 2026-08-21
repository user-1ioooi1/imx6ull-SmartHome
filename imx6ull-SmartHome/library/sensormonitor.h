#ifndef SENSORMONITOR_H
#define SENSORMONITOR_H

#include <QObject>
#include <QTimer>
#include "../hardware/hardwaremanager.h"
#include <QFutureWatcher>

class SensorMonitor : public QObject
{
    Q_OBJECT

public:
    SensorMonitor(const SensorMonitor &other) = delete;
    SensorMonitor &operator=(const SensorMonitor &other) = delete;

    static SensorMonitor* instance();
    void start(int intervalMs = 2000);
    void stop();

signals:
    void sensorDataUpdated(const SensorData data);

private slots:
    void onTimeout();
    void onReadFinished();

private:
    explicit SensorMonitor(QObject *parent = nullptr);
    QFutureWatcher<SensorData> m_watcher;
    QTimer m_timer;
    bool m_busy = false;
};

#endif // SENSORMONITOR_H
