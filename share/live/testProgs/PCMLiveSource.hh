#ifndef _PCM_LIVE_SOURCE_HH
#define _PCM_LIVE_SOURCE_HH

#include "FramedSource.hh"

extern "C" {
#include "audio_input.h"  // C 语言头文件
#include "tuya/tuya_audio_recv.h"
}

class PCMLiveSource : public FramedSource
{

public:
  static PCMLiveSource *createNew(UsageEnvironment &env);
  // redefined virtual functions
  virtual unsigned maxFrameSize() const;

  // 公共接口：控制音频发送状态
  static int openServerSpeaker();
  static int closeServerSpeaker();
  
  // 公共接口：控制音频源类型
  static int enableTuyaAudio();
  static int disableTuyaAudio();

  audio_input_context *ai_ctx;

protected:
  PCMLiveSource(UsageEnvironment &env);

  virtual ~PCMLiveSource();

private:
  virtual void doGetNextFrame();

  // 静态成员变量：音频发送状态
  static Boolean fAudioSend;
  // 静态成员变量：音频源类型（true: 涂鸦云音频, false: 本地音频）
  static Boolean fTuyaAudio;
};


#endif