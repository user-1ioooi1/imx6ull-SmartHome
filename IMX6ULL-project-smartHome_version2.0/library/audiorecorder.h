#ifndef AUDIORECORDER_H
#define AUDIORECORDER_H

#include <QObject>
#include <QAudioRecorder>
#include <QAudioProbe>
#include <QByteArray>
#include <QFile>
#include <QUrl>
#include <QAudioEncoderSettings>
#include <QVideoEncoderSettings>

class AudioRecorder : public QObject
{
    Q_OBJECT

public:
    explicit AudioRecorder(QObject *parent = nullptr);
    ~AudioRecorder();

    // 核心接口
    void startRecording();
    void stopRecording();
    bool isRecording() const { return m_isRecording; }

    // 缓冲区操作 - 直接返回PCM数据
    QByteArray getPcmData() const { return m_audioBuffer; }
    void clearBuffer();
    int getBufferSize() const { return m_audioBuffer.size(); }

    // 录音时长（秒）- 根据PCM数据大小计算
    double getDuration() const;

    // 保存为PCM文件（可选，用于调试）
    bool saveToPcmFile(const QString &fileName);

signals:
    void recordingStarted();
    void recordingStopped();
    void recordingError(const QString &error);
    void bufferUpdated(int size);
    void durationChanged(qint64 duration);

private slots:
    void handleDurationChanged(qint64 duration);
    void handleError(QMediaRecorder::Error error);
    void processBuffer(const QAudioBuffer& buffer);

private:
    QAudioRecorder *m_audioRecorder;
    QAudioProbe *m_audioProbe;
    QByteArray m_audioBuffer;        // PCM缓冲区
    bool m_isRecording;

    // PCM格式参数（固定，用于计算）
    static const int SAMPLE_RATE = 16000;
    static const int CHANNEL_COUNT = 1;
    static const int SAMPLE_SIZE = 16;  // 16位
    static const int BYTES_PER_SECOND = SAMPLE_RATE * CHANNEL_COUNT * (SAMPLE_SIZE / 8);
};

#endif // AUDIORECORDER_H
