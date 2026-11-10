#ifndef LTCDECODER_H
#define LTCDECODER_H

#include <QIODevice>

#include "Common/AppConfig/TimeCodeDefines.h"

class LtcDecoder: public QIODevice
{
  Q_OBJECT

public:
  LtcDecoder(QObject *parent=nullptr);
  ~LtcDecoder();

  virtual inline qint64 readData(char *, qint64) override { return 0; }
  virtual qint64 writeData(const char *data, qint64 len) override;

signals:
  void newFrame(TimeCodeFrame frame);

private:
  void *_private {nullptr};

  // 帧率检测状态必须是实例成员：函数级 static 会在多个解码器实例间共享，
  // 导致互相污染（如 LTCDecoderNode 与其他 LtcDecoder 并存时类型误判）
  int m_lastFrame = 0;        ///< 上一帧的帧号（用于检测帧号回卷）
  int m_frameRateGuess = 0;   ///< 待确认的帧率猜测
  int m_frameRate = 0;        ///< 已确认的帧率（24/25/30，标称值）
};

#endif // LTCDECODER_H
