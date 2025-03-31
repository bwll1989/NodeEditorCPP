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

bool LTCReceiver::isASIODevice(int deviceIndex) const
{
  const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(deviceIndex);
  if (!deviceInfo) return false;
  
  const PaHostApiInfo* hostInfo = Pa_GetHostApiInfo(deviceInfo->hostApi);
  return hostInfo && hostInfo->type == paASIO;
}

void LTCReceiver::setupASIOStream(PaStreamParameters& params)
{
  PaAsioStreamInfo asioInfo;
  asioInfo.size = sizeof(PaAsioStreamInfo);
  asioInfo.hostApiType = paASIO;
  asioInfo.version = 1;
  asioInfo.flags = paAsioUseChannelSelectors;
  
  // 为 ASIO 设置通道选择器
  static int channelSelectors[1];
  channelSelectors[0] = _selectedChannel;
  asioInfo.channelSelectors = channelSelectors;
  
  params.channelCount = 1;  // ASIO 模式下只使用一个通道
  params.hostApiSpecificStreamInfo = &asioInfo;
}


void LTCReceiver::start(QString deviceName)
{
  stop();
  _currentDeviceIndex = deviceName.split(":")[0].toInt();
  _currentDeviceName = deviceName;
  // 查找设备
  int numDevices = Pa_GetDeviceCount();
  if (_currentDeviceIndex < 0 || _currentDeviceIndex >= numDevices) {
    emit statusChanged(true, "找不到指定的音频设备");
    return;
  }

  _isASIO = isASIODevice(_currentDeviceIndex);

  // 配置音频流参数
  PaStreamParameters inputParameters;
  inputParameters.device = _currentDeviceIndex;
  inputParameters.sampleFormat = paUInt8;
  inputParameters.suggestedLatency = Pa_GetDeviceInfo(_currentDeviceIndex)->defaultLowInputLatency;
  inputParameters.hostApiSpecificStreamInfo = nullptr;

  if (_isASIO) {
    setupASIOStream(inputParameters);
  } else {
    inputParameters.channelCount = NUM_CHANNELS;
  }

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
    QString errorMsg = _isASIO ? 
      QString("无法打开ASIO流(通道 %1): %2").arg(_selectedChannel).arg(Pa_GetErrorText(err)) :
      QString("无法打开音频流: %1").arg(Pa_GetErrorText(err));
    emit statusChanged(true, errorMsg);
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
    if (self->_isASIO) {
      // ASIO 模式下直接处理数据（已经是选定的通道）
      self->processAudioData(buffer, framesPerBuffer);
    } else {
      // 非 ASIO 模式下需要提取指定通道
      char* selectedChannelData = new char[framesPerBuffer];
      for (unsigned long i = 0; i < framesPerBuffer; i++) {
        selectedChannelData[i] = buffer[i * self->NUM_CHANNELS + self->_selectedChannel];
      }
      self->processAudioData(selectedChannelData, framesPerBuffer);
      delete[] selectedChannelData;
    }
  }

  return paContinue;
}

void LTCReceiver::processAudioData(const char* buffer, size_t size)
{
  if (_decoder) {
    _decoder->writeData(buffer, size);
  }
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

