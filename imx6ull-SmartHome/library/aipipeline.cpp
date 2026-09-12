#include "aipipeline.h"
#include <QCoreApplication>

AiPipeline::AiPipeline(QObject *parent) : QObject(parent),
    m_recorder(new AudioRecorder(this)),
    m_asr(new asrApi(this)),
    m_tts(new ttsApi(this))
{
    QString configPath = QCoreApplication::applicationDirPath() + "/.config.ini";
    QSettings cfg(configPath, QSettings::IniFormat);

    m_asrKey    = cfg.value("ASR/api_key").toString();
    m_asrSecret = cfg.value("ASR/secret_key").toString();
    m_dsKey     = cfg.value("Deepseek/api_key").toString();
    m_dsUrl     = cfg.value("Deepseek/url").toString();

    if(!m_dsKey.isEmpty() && !m_dsUrl.isEmpty()){
        m_deepseek = new deepseekApi(m_dsKey, m_dsUrl, this);
        connect(m_deepseek, &deepseekApi::responseHanled, this, &AiPipeline::onLlmResponse);
        connect(m_deepseek, &deepseekApi::error , this, &AiPipeline::apiErrorHandle);
    }

    connect(m_asr, &asrApi::asrReadyData, this, &AiPipeline::onAsrResult);
    connect(m_asr, &asrApi::error , this,  &AiPipeline::apiErrorHandle);

    if(!m_asrKey.isEmpty() && !m_asrSecret.isEmpty()){
        m_asr->initialize(m_asrKey, m_asrSecret);
    }
}

AiPipeline::ErrorCode AiPipeline::startRecording()
{
    if(m_asrKey.isEmpty() || m_asrSecret.isEmpty()
            || m_dsKey.isEmpty() || m_dsUrl.isEmpty()){

         m_running = false;
         return ErrorCode::KEY_EMPTY;
    }

    if(m_asr->getToken().isEmpty()){
        m_running = false;
        return ErrorCode::KEY_Error;
    }

    m_running = true;
    m_recorder->startRecording();

    return ErrorCode::NoError;
}



void AiPipeline::stopRecordingAndProcess()
{
    if(m_running == false){
        return;
    }else{
        m_recorder->stopRecording();
        m_asr->recognizePcm(m_recorder->getPcmData());
    }
}

/*
void AiPipeline::saveRecording(const QString &filePath)
{
    m_recorder->saveToPcmFile(filePath);
}
*/

void AiPipeline::onAsrResult(const QString &text)
{
    if(m_running == false){
        return;
    }
    m_deepseek->set_url(m_dsUrl);
    m_deepseek->set_apiKey(m_dsKey);
    m_deepseek->ds_post(text);
}

void AiPipeline::onLlmResponse(const QString &text)
{
    if(m_running == false){
        return;
    }

    if (text.size() <= 2)
        return;

    QPair<QString,QString> pair = parseResponse(text);
    m_tts->tts_post(pair.first, m_asr->getToken());
    emit replyReady(pair.first);
    emit commandParsed(pair.second);
    m_running = false;
}

QPair<QString,QString> AiPipeline::parseResponse(const QString &text)
{
    QStringList parts = text.split("/");
    if (parts.size() < 2)
        return {text, ""};
    qDebug() << parts[0] << " "<<parts[1];
    return {parts[0], parts[1]};
}

void AiPipeline::apiErrorHandle(const QString& errorString, const QString& url, int httpCode)
{
    m_running = false;
    emit error(errorString, httpCode);
}
