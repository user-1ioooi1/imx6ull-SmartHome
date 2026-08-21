#include "aiassistant.h"
#include "ui_aiassistant.h"
#include <functional>

AiAssistant::AiAssistant(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AiAssistant)
{
    ui->setupUi(this);

    connect(&m_pipeline, &AiPipeline::replyReady, [this](const QString &text){
        ui->responseLab->setText(text);
    });
    connect(&m_pipeline, &AiPipeline::commandParsed, this, &AiAssistant::onCommandParsed);
}

AiAssistant::~AiAssistant()
{
    delete ui;
}

void AiAssistant::on_recorderBtn_pressed()
{
    m_pipeline.startRecording();
}

void AiAssistant::on_recorderBtn_released()
{
    m_pipeline.stopRecordingAndProcess();
}

void AiAssistant::onCommandParsed(const QString &command)
{
        if (command.isEmpty()) return;

        auto *hw = HardwareManager::instance();

        using Action = std::function<void()>;
        static const QList<QPair<QString, Action>> COMMANDS = {
            {"开灯",   [hw]{ hw->setLamp(true);         }},
            {"关灯",   [hw]{ hw->setLamp(false);        }},
            {"开门铃", [hw]{ hw->setBeep(true);        }},
            {"关门铃", [hw]{ hw->setBeep(false);       }},
            {"开门",   [hw]{ hw->setRelay(true);       }},
            {"关门",   [hw]{ hw->setRelay(false);      }},
            {"开窗帘", [hw]{ hw->setServoAngle(90);    }},
            {"关窗帘", [hw]{ hw->setServoAngle(0);     }},
        };

        for (const auto &entry : COMMANDS) {
            if (command.contains(entry.first)) {
                entry.second();
                return;
            }
        }
}
