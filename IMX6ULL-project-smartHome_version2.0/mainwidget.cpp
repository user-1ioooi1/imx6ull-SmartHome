#include "mainwidget.h"
#include "ui_mainwidget.h"


mainWidget::mainWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::mainWidget)
{
    ui->setupUi(this);

    m_httpServer = new HttpServer(this);
    m_httpServer->start(8080);
    connect(SensorMonitor::instance(), &SensorMonitor::sensorDataUpdated,
               m_httpServer, &HttpServer::updateSensorData);

    SensorMonitor::instance()->start(2000);

    QWidget *firstPage = ui->stackedWidget->widget(0);
    if (firstPage) {
        ui->stackedWidget->removeWidget(firstPage);
        delete firstPage;
    }

    m_basicFunc = new basicFunctions(this);
    ui->stackedWidget->insertWidget(0, m_basicFunc);
    ui->stackedWidget->setCurrentIndex(BasicFunctionsPg);

    m_musicPlayer = new musicPlayer(this);
    ui->stackedWidget->addWidget(m_musicPlayer);

    m_videoPlayer = new videoPlayer(this);
    ui->stackedWidget->addWidget(m_videoPlayer);

    m_camera = new Camera(this);
    ui->stackedWidget->addWidget(m_camera);

    m_aiAssistant = new AiAssistant(this);
    ui->stackedWidget->addWidget(m_aiAssistant);

    // Connect camera streaming: HttpServer requests -> Camera enables/disables
    connect(m_httpServer, &HttpServer::cameraStreamingChanged,
            this, [this](bool streaming) {
        m_camera->setStreamingEnabled(streaming);
        if (streaming && curPage != CameraPg) {
            // 不在相机页也要启动，给网页推流
            m_camera->start();
        } else if (!streaming && curPage == CameraPg) {
            // 在相机页停流后重启，恢复 viewfinder 预览
            m_camera->start();
        }
        // !streaming && curPage != CameraPg：setStreamingEnabled 已 stop，不用再管
    });

    // Connect camera frames to HTTP server for MJPEG streaming
    connect(m_camera, &Camera::frameReady,
            m_httpServer, &HttpServer::onCameraFrame);

    connect(ui->stackedWidget, &QStackedWidget::currentChanged,
            this, &mainWidget::onStackedWidgetChanged);
}

mainWidget::~mainWidget()
{
    delete ui;
}


void mainWidget::on_basicFunUIBtn_clicked()
{
    ui->stackedWidget->setCurrentIndex(BasicFunctionsPg);
}

void mainWidget::on_musicPlayerBtn_clicked()
{
    ui->stackedWidget->setCurrentIndex(MusicPlayerPg);
}

void mainWidget::on_videoPlayerBtn_clicked()
{
    ui->stackedWidget->setCurrentIndex(VideoPlayerPg);
}

void mainWidget::on_cameraBtn_clicked()
{
    ui->stackedWidget->setCurrentIndex(CameraPg);
}

void mainWidget::on_aiAssistantBtn_clicked()
{
    ui->stackedWidget->setCurrentIndex(AiAssistantPg);
}

void mainWidget::onStackedWidgetChanged(int index)
{
    prePage = curPage;
    curPage = static_cast<Page>(index);

    if (prePage == VideoPlayerPg) {
        m_videoPlayer->hideWidget();
    } else if (prePage == CameraPg) {
        // Only stop camera if not streaming to web
        if (!m_camera->isStreamingEnabled()) {
            m_camera->stop();
        }
    }

    if (curPage == VideoPlayerPg) {
        m_videoPlayer->showWidget();
    } else if (curPage == CameraPg) {
        m_camera->start();
    }
}
