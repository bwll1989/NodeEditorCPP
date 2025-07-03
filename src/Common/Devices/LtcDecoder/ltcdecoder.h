#ifndef LTCDECODER_H
#define LTCDECODER_H

#include <QIODevice>

#include "TimeCodeDefines.h"

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
};

#endif // LTCDECODER_H
