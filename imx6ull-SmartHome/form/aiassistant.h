#ifndef AIASSISTANT_H
#define AIASSISTANT_H

#include <QWidget>
#include "../library/aipipeline.h"
#include "../hardware/hardwaremanager.h"

namespace Ui {
class AiAssistant;
}

class AiAssistant : public QWidget
{
    Q_OBJECT

public:
    explicit AiAssistant(QWidget *parent = nullptr);
    ~AiAssistant();

private slots:
    void on_recorderBtn_pressed();
    void on_recorderBtn_released();
    void onCommandParsed(const QString &command);

private:
    Ui::AiAssistant *ui;
    AiPipeline      m_pipeline;
};

#endif // AIASSISTANT_H
