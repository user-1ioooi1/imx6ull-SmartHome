#include "mainwidget.h"
#include "ui_mainwidget.h"


mainWidget::mainWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::mainWidget)
{
    ui->setupUi(this);


    QWidget *firstPage = ui->stackedWidget->widget(0);

    // 如果第一页是占位widget，删除它
    if (firstPage) {
        ui->stackedWidget->removeWidget(firstPage);
        delete firstPage;
    }

    // 创建并添加basicFunctions作为第一页
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

    if(prePage == BasicFunctionsPg){
           m_basicFunc->closeSensorTimer();
       }else if(prePage == VideoPlayerPg){
           m_videoPlayer->hideWidget();
       }else if(prePage == CameraPg){
           m_camera->stop();
       }

       if(curPage == BasicFunctionsPg){
           m_basicFunc->openSensorTimer();
       }else if(curPage == VideoPlayerPg){
           m_videoPlayer->showWidget();
       }else if(curPage == CameraPg){
           m_camera->start();
       }

}

