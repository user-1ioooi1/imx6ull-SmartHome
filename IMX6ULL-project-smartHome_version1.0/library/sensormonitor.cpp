#include "sensormonitor.h"

SensorMonitor::SensorMonitor(QObject *parent) : QObject(parent)
{
    connect(&m_timer, &QTimer::timeout, this, &SensorMonitor::onTimeout);
}

void SensorMonitor::start(int intervalMs)
{
    m_timer.start(intervalMs);
}

void SensorMonitor::stop()
{
    m_timer.stop();
}

void SensorMonitor::onTimeout()
{
    SensorData data = HardwareManager::instance()->readSensors();
    emit sensorDataUpdated(data);
}
