
#include "ltcdecoder.h"

#include <ltc.h>

#define GETLTC static_cast<LTCDecoder *>(_private)


LTCDecoder::LTCDecoder(QObject *parent)
  :QIODevice(parent)
{
  int apv = 1920;
  int queue_size = 32;
  _private = ltc_decoder_create(apv, queue_size);

  setOpenMode(QIODevice::WriteOnly);
}


LTCDecoder::~LTCDecoder()
{
  LTCDecoder *ltc = GETLTC;
  ltc_decoder_free(ltc);
  _private = nullptr;
}


qint64 LTCDecoder::writeData(const char *data, qint64 len)
{
  static int last_frame = 0;
  static int frame_rate_g = 0;
  static int frame_rate = 0;

  LTCDecoder *ltc = GETLTC;
  LTCFrameExt ltc_frame;
  SMPTETimecode ltc_stime;
  TimeCodeFrame frame;

  ltc_decoder_write(ltc, (ltcsnd_sample_t *)data, len, 0);

  while(ltc_decoder_read(ltc, &ltc_frame))
  {
    if(ltc_frame.reverse)
      continue;

    ltc_frame_to_time(&ltc_stime, &ltc_frame.ltc, 0);

    frame.hours = ltc_stime.hours;
    frame.minutes = ltc_stime.mins;
    frame.seconds = ltc_stime.secs;
    frame.frames = ltc_stime.frame;
    frame.type = TimeCodeType::PAL; // 默认设置为PAL

    if(frame.frames == 0 &&
       (last_frame == 29 || last_frame == 24 || last_frame == 23))
    {
      // 检测帧率
      int r = last_frame + 1;

      if(r == frame_rate_g) // "frame rate guess"
      {
        // 连续两次相同的帧率，确认接受
        frame_rate = r;
        frame_rate_g = 0;
      }
      else if(r != frame_rate)
      {
        // 可能是帧率变化或首次运行
        if(r > frame_rate || frame_rate == 0)
        {
          frame_rate = r;
          frame_rate_g = 0;
        }
        else
        {
          frame_rate_g = r;
        }
      }
      else
      {
        frame_rate_g = 0;
      }
    }

    last_frame = frame.frames;

    // 根据LTC信息和检测到的帧率判断时间码类型
    if(ltc_frame.ltc.dfbit) {
      // Drop Frame位被设置
      if(frame_rate == 30 || frame_rate == 29)
        frame.type = TimeCodeType::NTSC_DF;
      else if(frame_rate == 24)
        frame.type = TimeCodeType::Film_DF;
    } else {
      // 非Drop Frame
      if(frame_rate == 30)
        frame.type = TimeCodeType::NTSC;
      else if(frame_rate == 25)
        frame.type = TimeCodeType::PAL;
      else if(frame_rate == 24)
        frame.type = TimeCodeType::Film;
    }

    emit newFrame(frame);
  }

  return len;
}
