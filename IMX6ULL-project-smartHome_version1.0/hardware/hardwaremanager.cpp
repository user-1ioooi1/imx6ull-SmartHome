#include "hardwaremanager.h"

HardwareManager* HardwareManager::m_instance = nullptr;

HardwareManager* HardwareManager::instance()
{
    if (!m_instance){
        m_instance = new HardwareManager();
    }

    return m_instance;
}

HardwareManager::HardwareManager(QObject *parent) : QObject(parent)
{
    initLed(&m_led);
    initBeep(&m_beep);
    initSg90(&m_sg90);
    initJdq(&m_jdq);
    initDht11(&m_dht11);
    initAp3216c(&m_ap3216c);
    initSr501(&m_sr501);
    initMq135(&m_mq135);
}

HardwareManager::~HardwareManager()
{
    freeLed(&m_led);
    freeBeep(&m_beep);
    freeSg90(&m_sg90);
    freeJdq(&m_jdq);
    freeDht11(&m_dht11);
    freeAp3216c(&m_ap3216c);
    freeSr501(&m_sr501);
    freeMq135(&m_mq135);
}

void HardwareManager::setLamp(bool on)
{
    ledStateSwitch(&m_led, on ? 1 : 0);
    emit lampStateChanged(on);
}

void HardwareManager::setBeep(bool on)
{
    beepStateSwitch(&m_beep, on ? 1 : 0);
    emit beepStateChanged(on);
}

void HardwareManager::setRelay(bool on)
{
    jdqStateSwitch(&m_jdq, on ? 1 : 0);
    emit relayStateChanged(on);
}

void HardwareManager::setServoAngle(unsigned char angle)
{
        sg90ChangeAngle(&m_sg90, angle);
        emit servoAngleChanged(angle);
}

SensorData HardwareManager::readSensors()
{
    SensorData data;

    ap3216cGetValue(&m_ap3216c);
    //data.ir  = ap3216cGetIr(&m_ap3216c); //红外线
    data.als = ap3216cGetAls(&m_ap3216c); //环境光亮度
   // data.ps  = ap3216cGetPs(&m_ap3216c); //distance

    dht11_getData(&m_dht11);
    data.temperature = dht11_gettempature(&m_dht11);
    data.humidity    = dht11_getHumidity(&m_dht11);

    data.motionDetected = sr501RP(&m_sr501);

    mq135RD(&m_mq135);
    data.airQuality = mq135Rscale(&m_mq135) * mq135Rraw(&m_mq135);

    return data;
}
