#ifndef _PCM_MEDIA_SUBSESSION_HH
#define _PCM_MEDIA_SUBSESSION_HH
#include "OnDemandServerMediaSubsession.hh"

class XXPCMServerMediaSubssion : public OnDemandServerMediaSubsession
{

public:
        static XXPCMServerMediaSubssion *createNew(UsageEnvironment &env, Boolean reuseFirstSource);

protected:
        XXPCMServerMediaSubssion(UsageEnvironment &env, Boolean reuseFirstSource);
        ~XXPCMServerMediaSubssion();

protected: // redefined virtual functions
        FramedSource *createNewStreamSource(unsigned clientSessionId, unsigned &estBitrate);
        RTPSink *createNewRTPSink(Groupsock *rtpGroupsock,
                                  unsigned char rtpPayloadTypeIfDynamic,
                                  FramedSource *inputSource);

public:
};

#endif