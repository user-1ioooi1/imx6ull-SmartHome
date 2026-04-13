#include "basicfunctions.h"
#include "ui_basicfunctions.h"

basicFunctions::basicFunctions(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::basicFunctions)
{
    ui->setupUi(this);


    ClockWidget::instance(this);  // 创建时钟实例
    ClockWidget::instance()->setShowSeconds(false);
    ui->clockLayout->addWidget(ClockWidget::instance());  // 添加到布局

    m_beepTimer.setSingleShot(true);
    connect(&m_beepTimer, &QTimer::timeout, []{
        HardwareManager::instance()->setBeep(false);
    });

    ui->CurtainSlider->setRange(0, 180);
    connect(ui->CurtainSlider, &QSlider::valueChanged, [](int angle){
        HardwareManager::instance()->setServoAngle(static_cast<unsigned char>(angle));
    });

    connect(HardwareManager::instance(),  &HardwareManager::servoAngleChanged, [this](int angle){
        if (ui->CurtainSlider->value() != angle) {
               ui->CurtainSlider->setValue(angle);
         }
    });

    ui->curtainProgressBar->setRange(0, 180);
    ui->curtainProgressBar->setValue(0);
    connect(ui->CurtainSlider, &QSlider::valueChanged, ui->curtainProgressBar, &QProgressBar::setValue);

    connect(&m_sensorMonitor, &SensorMonitor::sensorDataUpdated,
            this, &basicFunctions::onSensorDataUpdated);

    connect(HardwareManager::instance(),  &HardwareManager::lampStateChanged,[this](bool state){
        m_ledState = state;
        this->btnIconChange(ui->lampBtnIcon,state);
    });

    connect(HardwareManager::instance(),  &HardwareManager::relayStateChanged,[this](bool state){
        m_relayState = state;
        this->btnIconChange(ui->lockBtnIcon,state);
    });

    connect(HardwareManager::instance(),  &HardwareManager::beepStateChanged,[this](bool state){
        this->btnIconChange(ui->beepBtnIcon,state);
    });

    HardwareManager::instance()->setLamp(false);
    HardwareManager::instance()->setBeep(false);
    HardwareManager::instance()->setRelay(false); //close door
    HardwareManager::instance()->setServoAngle(0); //close curtain

    openSensorTimer();// sensor update
}

basicFunctions::~basicFunctions()
{
    delete ui;
}

void basicFunctions::on_ledBtn_clicked()
{
    m_ledState = !m_ledState;
    HardwareManager::instance()->setLamp(m_ledState);
}

void basicFunctions::on_beepBtn_clicked()
{
    HardwareManager::instance()->setBeep(true);
    m_beepTimer.start(2000);
}

void basicFunctions::on_doorLockBtn_clicked()
{
    m_relayState = !m_relayState;
    HardwareManager::instance()->setRelay(m_relayState);
}

void basicFunctions::onSensorDataUpdated(const SensorData &data)
{
    QString airQuality;
    ui->lightLabelNum->setNum(data.als);
    //ui->InfraredLabelNum->setNum(data.ir);
    //ui->distanceLabelNum->setNum(data.ps);
    ui->tempatureNum->setNum(data.temperature);
    ui->humidityLabelNum->setNum(data.humidity);
    ui->hasPersonLabelNum->setNum(data.motionDetected);

    if(0 < data.airQuality && data.airQuality <= 200){
        airQuality = "优";
    }else if(data.airQuality <= 400){
        airQuality = "良";
    }else if(data.airQuality <= 600){
        airQuality = "中";
    }else{
         airQuality = "差";
    }
    ui->airQualityLabelNum->setText(airQuality);
}

void basicFunctions::closeSensorTimer()
{
    m_sensorMonitor.stop();
}

void basicFunctions::openSensorTimer()
{
    m_sensorMonitor.start(2000); //2s
}

void basicFunctions::btnIconChange(QLabel *btnStateIcon,bool on){
    if(on){
        btnStateIcon->setStyleSheet("image: url(:/icon/icon/开关_蓝.png);");
    }else{
        btnStateIcon->setStyleSheet("image: url(:/icon/icon/开关.png);");
    }
}
