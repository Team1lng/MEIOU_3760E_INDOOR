#include "PCMMediaSubsession.hh"
#include "PCMLiveSource.hh"
#include <RTCP.hh>
#include "SimpleRTPSink.hh"

XXPCMServerMediaSubssion *XXPCMServerMediaSubssion::createNew(UsageEnvironment &env, Boolean reuseFirstSource)
{
	return new XXPCMServerMediaSubssion(env, reuseFirstSource);
}

XXPCMServerMediaSubssion::XXPCMServerMediaSubssion(UsageEnvironment &env, Boolean reuseFirstSource)
    : OnDemandServerMediaSubsession(env, reuseFirstSource)
{
}

XXPCMServerMediaSubssion::~XXPCMServerMediaSubssion()
{
}

FramedSource *XXPCMServerMediaSubssion::createNewStreamSource(unsigned clientSessionId, unsigned &estBitrate)
{
	// 创建视频源,参照H264VideoFileServerMediaSubsession
	PCMLiveSource *liveSource = PCMLiveSource::createNew(envir());
	if (liveSource == NULL)
	{
		return NULL;
	}
  return liveSource;
	// printf("[%s:%d]====================\n", __func__, __LINE__);
	// return H264VideoStreamDiscreteFramer::createNew(envir(), liveSource);
}

RTPSink *XXPCMServerMediaSubssion ::createNewRTPSink(Groupsock *rtpGroupsock,
							     unsigned char rtpPayloadTypeIfDynamic,
							     FramedSource * /*inputSource*/)
{
 return SimpleRTPSink::createNew(envir(),
                                  rtpGroupsock,
                                  rtpPayloadTypeIfDynamic, // 使用传入的动态负载类型
                                  16000,                    // 采样率，需与PCM源匹配
                                  "audio",
                                  "PCMU", // 16位线性PCM
                                  1,      // 声道数，根据实际情况修改
                                  True);}
