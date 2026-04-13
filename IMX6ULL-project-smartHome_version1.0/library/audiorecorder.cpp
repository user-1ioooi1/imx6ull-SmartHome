#include "audiorecorder.h"
#include <QAudioEncoderSettings>
#include <QUrl>
#include <QDebug>
#include <QDir>
#include <QDateTime>

AudioRecorder::AudioRecorder(QObject *parent)
    : QObject(parent)
    , m_isRecording(false)
{
    m_audioRecorder = new QAudioRecorder(this);
    m_audioProbe = new QAudioProbe(this);

    // 设置探针，捕获音频数据
    m_audioProbe->setSource(m_audioRecorder);
    connect(m_audioProbe, &QAudioProbe::audioBufferProbed,
            this, &AudioRecorder::processBuffer);

    // 连接信号槽
    connect(m_audioRecorder, &QAudioRecorder::durationChanged,
            this, &AudioRecorder::handleDurationChanged);
    connect(m_audioRecorder, QOverload<QMediaRecorder::Error>::of(&QAudioRecorder::error),
            this, &AudioRecorder::handleError);

    // 设置默认音频输入设备
    QStringList inputs = m_audioRecorder->audioInputs();
    if (!inputs.isEmpty()) {
        m_audioRecorder->setAudioInput(inputs.first());
        qDebug() << "使用音频设备:" << inputs.first();
    } else {
        qWarning() << "未找到音频输入设备！";
    }
    /*
     // 获取所有支持的音频编解码器列表
     QStringList codecs = m_audioRecorder->supportedAudioCodecs();

     qDebug() << "系统支持的音频编解码器:";
     for (int i = 0; i < codecs.count(); i++) {
         // 还可以获取每个编解码器的描述信息
         QString description = m_audioRecorder->audioCodecDescription(codecs[i]);
         qDebug() << "  " << codecs[i] << " - " << description;
     }*/

}

AudioRecorder::~AudioRecorder()
{
    if (m_isRecording) {
        stopRecording();
    }
}

void AudioRecorder::startRecording()
{
    if (m_isRecording) {
        emit recordingError("已经在录音中");
        return;
    }

    // 清空之前的缓冲区
    m_audioBuffer.clear();
    // 配置录音参数 - 纯PCM格式
    QAudioEncoderSettings settings;
    //settings.setCodec("audio/x-raw, layout=(string)interleaved");  // unbantu PCM编码
    settings.setCodec("audio/x-raw");  // imx6ull PCM编码
    settings.setSampleRate(SAMPLE_RATE);
    settings.setChannelCount(CHANNEL_COUNT);
    settings.setQuality(QMultimedia::HighQuality);
    settings.setEncodingMode(QMultimedia::ConstantQualityEncoding);

    // 设置编码格式 - 仍然用WAV容器，但数据会被探针捕获
    m_audioRecorder->setEncodingSettings(
        settings,
        QVideoEncoderSettings(),
        "audio/x-wav"
    );

    // Linux用/dev/null避免磁盘写入，其他系统用临时文件
#ifdef Q_OS_LINUX
    m_audioRecorder->setOutputLocation(QUrl::fromLocalFile("/dev/null"));
#else
    QString tempFile = QDir::tempPath() + "/temp_audio_" +
                       QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") +
                       ".wav";
    m_audioRecorder->setOutputLocation(QUrl::fromLocalFile(tempFile));
#endif

    // 开始录音
    m_audioRecorder->record();
    m_isRecording = true;

    emit recordingStarted();
    qDebug() << "开始录音到缓冲区(PCM)...";
}

void AudioRecorder::stopRecording()
{
    if (!m_isRecording) {
        return;
    }

    m_audioRecorder->stop();
    m_isRecording = false;

    // 删除临时文件（非Linux系统）
#ifndef Q_OS_LINUX
    QString tempFile = m_audioRecorder->outputLocation().toLocalFile();
    if (QFile::exists(tempFile)) {
        QFile::remove(tempFile);
    }
#endif

    emit recordingStopped();
    qDebug() << "录音已停止，PCM数据大小:" << m_audioBuffer.size() << "字节"
             << "时长:" << getDuration() << "秒";
}

bool AudioRecorder::saveToPcmFile(const QString &fileName)
{
    if (m_audioBuffer.isEmpty()) {
        emit recordingError("缓冲区为空，没有可保存的数据");
        return false;
    }

    // 检查并创建目录
    QFileInfo fileInfo(fileName);
    QDir dir = fileInfo.absoluteDir();
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            emit recordingError("无法创建目录: " + dir.path());
            return false;
        }
    }

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) {
        emit recordingError("无法打开文件写入: " + fileName);
        return false;
    }

    // 直接写入PCM数据，不加任何头
    qint64 written = file.write(m_audioBuffer);
    file.close();

    if (written == m_audioBuffer.size()) {
        qDebug() << "成功保存PCM文件:" << fileName
                 << "大小:" << written << "字节"
                 << "时长:" << getDuration() << "秒";
        return true;
    } else {
        emit recordingError("文件写入不完整");
        return false;
    }
}

void AudioRecorder::clearBuffer()
{
    m_audioBuffer.clear();
    qDebug() << "PCM缓冲区已清空";
}

double AudioRecorder::getDuration() const
{
    if (m_audioBuffer.isEmpty()) return 0;

    // PCM数据大小 / 每秒字节数 = 时长(秒)
    return static_cast<double>(m_audioBuffer.size()) / BYTES_PER_SECOND;
}

void AudioRecorder::handleDurationChanged(qint64 duration)
{
    emit durationChanged(duration);
}

void AudioRecorder::handleError(QMediaRecorder::Error error)
{
    if (error != QMediaRecorder::NoError) {
        QString errorMsg = m_audioRecorder->errorString();
        emit recordingError(errorMsg);
        qWarning() << "录音错误:" << errorMsg;
        m_isRecording = false;
    }
}

void AudioRecorder::processBuffer(const QAudioBuffer& buffer)
{
    if (!m_isRecording) return;

    // 将音频数据追加到缓冲区
    const char *data = buffer.constData<char>();
    int size = buffer.byteCount();
    m_audioBuffer.append(data, size);

    // 每收到一定数据就发射信号
    if (m_audioBuffer.size() % BYTES_PER_SECOND == 0) {  // 每秒触发一次
        emit bufferUpdated(m_audioBuffer.size());
    }
}
