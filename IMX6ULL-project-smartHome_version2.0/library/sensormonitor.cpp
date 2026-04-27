#include "sensormonitor.h"
#include <QtConcurrent/QtConcurrent>
SensorMonitor* SensorMonitor::instance(){
    static SensorMonitor m_monitor;
    return &m_monitor;
}

SensorMonitor::SensorMonitor(QObject *parent) : QObject(parent)
{
    connect(&m_timer, &QTimer::timeout, this, &SensorMonitor::onTimeout);
    connect(&m_watcher, &QFutureWatcher<SensorData>::finished, this, &SensorMonitor::onReadFinished);
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
    if (m_busy) return;
    m_busy = true;
    m_watcher.setFuture(QtConcurrent::run(HardwareManager::instance(), &HardwareManager::readSensors));
}

void SensorMonitor::onReadFinished()
{
    m_busy = false;
    emit sensorDataUpdated(m_watcher.result());
}
