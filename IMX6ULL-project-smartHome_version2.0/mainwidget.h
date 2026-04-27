#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <QWidget>
#include "library/clickableslider.h"
#include "library/httpserver.h"
#include "library/sensormonitor.h"
#include "form/basicfunctions.h"
#include "form/musicPlayer.h"
#include "form/videoplayer.h"
#include "form/camera.h"
#include "form/aiassistant.h"

QT_BEGIN_NAMESPACE
namespace Ui { class mainWidget; }
QT_END_NAMESPACE

class mainWidget : public QWidget
{
    Q_OBJECT

public:
    mainWidget(QWidget *parent = nullptr);
    ~mainWidget();

private slots:
    void on_basicFunUIBtn_clicked();

    void on_musicPlayerBtn_clicked();

    void on_videoPlayerBtn_clicked();

    void onStackedWidgetChanged(int index);

    void on_cameraBtn_clicked();

    void on_aiAssistantBtn_clicked();

private:
    enum Page {
        NonePg = -1,
        BasicFunctionsPg,
        MusicPlayerPg,
        VideoPlayerPg,
        CameraPg,
        AiAssistantPg
    };
    Ui::mainWidget *ui;
    basicFunctions *m_basicFunc;
    musicPlayer *m_musicPlayer;
    videoPlayer *m_videoPlayer;
    Camera *m_camera;
    AiAssistant *m_aiAssistant;
    HttpServer   *m_httpServer;
    Page curPage = BasicFunctionsPg;
    Page prePage = NonePg;
};
#endif // MAINWIDGET_H
