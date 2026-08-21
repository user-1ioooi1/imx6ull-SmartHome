#include "aipipeline.h"
#include <QCoreApplication>

AiPipeline::AiPipeline(QObject *parent) : QObject(parent),
    m_recorder(new AudioRecorder(this)),
    m_asr(new asrApi(this)),
    m_tts(new ttsApi(this))
{

    QSettings cfg(":/config.ini", QSettings::IniFormat);

    QString asrKey    = cfg.value("ASR/api_key").toString();
    QString asrSecret = cfg.value("ASR/secret_key").toString();
    QString dsKey     = cfg.value("Deepseek/api_key").toString();
    QString dsUrl     = cfg.value("Deepseek/url").toString();

    m_deepseek = new deepseekApi(dsKey, dsUrl, this);
    m_asr->initialize(asrKey, asrSecret);

    connect(m_asr, &asrApi::asrReadyData, this, &AiPipeline::onAsrResult);
    connect(m_deepseek, &deepseekApi::responseHanled, this, &AiPipeline::onLlmResponse);
}

void AiPipeline::startRecording()
{
    m_recorder->startRecording();
}

void AiPipeline::stopRecordingAndProcess()
{
    m_recorder->stopRecording();
    m_asr->recognizePcm(m_recorder->getPcmData());
}

void AiPipeline::saveRecording(const QString &filePath)
{
    m_recorder->saveToPcmFile(filePath);
}

void AiPipeline::onAsrResult(const QString &text)
{
    m_deepseek->ds_post(text);
}

void AiPipeline::onLlmResponse(const QString &text)
{
    if (text.size() <= 2)
        return;

    QPair<QString,QString> pair = parseResponse(text);
    m_tts->tts_post(pair.first, m_asr->getToken());
    emit replyReady(pair.first);
    emit commandParsed(pair.second);
}

QPair<QString,QString> AiPipeline::parseResponse(const QString &text)
{
    QStringList parts = text.split("/");
    if (parts.size() < 2)
        return {text, ""};
    qDebug() << parts[0] << " "<<parts[1];
    return {parts[0], parts[1]};
}
