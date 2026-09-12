#ifndef AIPIPELINE_H
#define AIPIPELINE_H

#include <QObject>
#include <QPair>
#include <QSettings>
#include "audiorecorder.h"
#include "asrapi.h"
#include "deepseekapi.h"
#include "ttsapi.h"

class AiPipeline : public QObject
{
    Q_OBJECT

public:
    enum class ErrorCode {
        NoError,
        KEY_EMPTY,
        KEY_Error,
    };
    explicit AiPipeline(QObject *parent = nullptr);

    ErrorCode startRecording();
    void stopRecordingAndProcess();
    //void saveRecording(const QString &filePath);

signals:
    void error(const QString text, int httpCode);
    void replyReady(const QString &displayText);   // 展示给用户的回复
    void commandParsed(const QString &command);    // 解析出的控制指令

private slots:
    void onAsrResult(const QString &text);
    void onLlmResponse(const QString &text);

private:
    static QPair<QString,QString> parseResponse(const QString &text);
    void apiErrorHandle(const QString& errorString, const QString& url, int httpCode);

    AudioRecorder *m_recorder;
    asrApi        *m_asr;
    deepseekApi   *m_deepseek;
    ttsApi        *m_tts;

    QString       m_asrKey;
    QString       m_asrSecret;
    QString       m_dsKey;
    QString       m_dsUrl;
    bool          m_running;
};

#endif // AIPIPELINE_H
