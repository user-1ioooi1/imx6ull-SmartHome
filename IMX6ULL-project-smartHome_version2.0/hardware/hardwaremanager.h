#ifndef HARDWAREMANAGER_H
#define HARDWAREMANAGER_H

#include <QObject>
#include <QMetaType>
#include "led.h"
#include "beep.h"
#include "sg90.h"
#include "jdq.h"
#include "dht11.h"
#include "ap3216c.h"
#include "sr501.h"
#include "mq135.h"

struct SensorData {
    unsigned short ir;
    unsigned short als;
    unsigned short ps;
    short temperature;
    short humidity;
    int motionDetected;
    double airQuality;
};
Q_DECLARE_METATYPE(SensorData)
class HardwareManager : public QObject
{
    Q_OBJECT

public:
    static HardwareManager* instance();

    // 执行器控制
    void setLamp(bool on);
    void setBeep(bool on);
    void setRelay(bool on);
    void setServoAngle(unsigned char angle);

    // 传感器读取
    SensorData readSensors();

signals:
    void servoAngleChanged(unsigned char angle);
    void lampStateChanged(bool on);
    void relayStateChanged(bool on);
    void beepStateChanged(bool on);

private:
    explicit HardwareManager(QObject *parent = nullptr);
    ~HardwareManager();
    HardwareManager(const HardwareManager&) = delete;
    HardwareManager& operator=(const HardwareManager&) = delete;

    struct led    m_led;
    struct beep   m_beep;
    struct sg90   m_sg90;
    struct jdq    m_jdq;
    struct dht11  m_dht11;
    struct ap3216c m_ap3216c;
    struct sr501  m_sr501;
    struct mq135  m_mq135;
};

#endif // HARDWAREMANAGER_H
