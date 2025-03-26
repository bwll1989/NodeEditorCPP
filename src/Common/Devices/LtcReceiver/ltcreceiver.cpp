#include "ltcreceiver.h"
#include <QDebug>

LTCReceiver::LTCReceiver(QObject *parent)
  : QObject(parent)
{
  // 初始化 PortAudio
  PaError err = Pa_Initialize();
  if (err != paNoError) {
    emit statusChanged(true, QString("PortAudio 初始化失败: %1").arg(Pa_GetErrorText(err)));
    return;
  }

  _decoder = new LTCDecoder;
  connect(_decoder, &LTCDecoder::newFrame, this, &LTCReceiver::newFrame);
}

LTCReceiver::~LTCReceiver()
{
  stop();
  Pa_Terminate();

  if(_decoder) {
    delete _decoder;
    _decoder = nullptr;
  }
}

void LTCReceiver::start(int deviceIndex)
{
  stop();
  // 查找设备
  int numDevices = Pa_GetDeviceCount();



  if (deviceIndex < 0 || deviceIndex >= numDevices) {
    emit statusChanged(true, "找不到指定的音频设备");
    return;
  }


  // 配置音频流参数
  PaStreamParameters inputParameters;
  inputParameters.device = deviceIndex;
  inputParameters.channelCount = NUM_CHANNELS;
  inputParameters.sampleFormat = paUInt8; // 使用与原来相同的格式
  inputParameters.suggestedLatency = Pa_GetDeviceInfo(deviceIndex)->defaultLowInputLatency;
  inputParameters.hostApiSpecificStreamInfo = nullptr;

  // 打开音频流
  PaError err = Pa_OpenStream(&_paStream,
                             &inputParameters,
                             nullptr,  // 无输出
                             SAMPLE_RATE,
                             FRAMES_PER_BUFFER,
                             paClipOff,
                             paCallback,
                             this);

  if (err != paNoError) {
    emit statusChanged(true, QString("无法打开音频流: %1").arg(Pa_GetErrorText(err)));
    return;
  }

  // 启动音频流
  err = Pa_StartStream(_paStream);
  if (err != paNoError) {
    emit statusChanged(true, QString("无法启动音频流: %1").arg(Pa_GetErrorText(err)));
    Pa_CloseStream(_paStream);
    _paStream = nullptr;
    return;
  }

  _isRunning = true;
  emit statusChanged(false, "listening");
}

void LTCReceiver::stop()
{
  if (!_isRunning) return;

  if (_paStream) {
    Pa_StopStream(_paStream);
    Pa_CloseStream(_paStream);
    _paStream = nullptr;
  }

  _isRunning = false;

  emit statusChanged(true, "stopped");
}

int LTCReceiver::paCallback(const void* inputBuffer, void* outputBuffer,
                          unsigned long framesPerBuffer,
                          const PaStreamCallbackTimeInfo* timeInfo,
                          PaStreamCallbackFlags statusFlags,
                          void* userData)
{
  auto* self = static_cast<LTCReceiver*>(userData);
  const char* buffer = static_cast<const char*>(inputBuffer);

  if (buffer) {
    self->processAudioData(buffer, framesPerBuffer);
  }

  return paContinue;
}

void LTCReceiver::processAudioData(const char* buffer, size_t size)
{
  if (_decoder) {
    _decoder->writeData(buffer, size);
  }
}

