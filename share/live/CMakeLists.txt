 add_definitions(
  -DSOCKLEN_T=socklen_t 
   -DNO_SSTREAM=1 
   -D_LARGEFILE_SOURCE=1 
  -D_FILE_OFFSET_BITS=64
   -DNO_OPENSSL
   -DLOCALE_NOT_USED
 #  -DNO_GETIFADDRS
 )
include_directories(
  liveMedia/include/
  groupsock/include/
  UsageEnvironment/include/
  BasicUsageEnvironment/include/
)


set(src
    testProgs/testRTSPClient.cpp
    testProgs/testOnDemandRTSPServer.cpp
    testProgs/PCMMediaSubsession.cpp
    testProgs/PCMLiveSource.cpp
  #  testProgs/testOnAmyRTSPServer.cpp
   testProgs/announceURL.cpp
  #  liveMedia/AmyH264MediaSubsession.cpp
  #  liveMedia/ByteStreamAmySource.cpp
  #  liveMedia/H264VideoStreamFramer.cpp
  #  liveMedia/AmyWAVAudioMediaSubsession.cpp

    #testProgs/testOnDemandRTSPServer.cpp
    #liveMedia/H264VideoFileServerMediaSubsession.cpp
   # liveMedia/FileServerMediaSubsession.cpp
   # liveMedia/ByteStreamFileSource.cpp
   # liveMedia/InputFile.cpp
   # liveMedia/FramedFileSource.cpp
    
    liveMedia/Media.cpp
    liveMedia/MediaSink.cpp
    liveMedia/FramedSource.cpp
    liveMedia/MediaSource.cpp
    liveMedia/RTSPClient.cpp
    liveMedia/RTSPServer.cpp
    liveMedia/RTSPServerRegister.cpp
    liveMedia/GenericMediaServer.cpp
    liveMedia/TLSState.cpp
    liveMedia/RTSPRegisterSender.cpp
    liveMedia/DigestAuthentication.cpp
    liveMedia/ourMD5.cpp
    liveMedia/ProxyServerMediaSession.cpp
    liveMedia/OnDemandServerMediaSubsession.cpp
    liveMedia/ServerMediaSession.cpp
    liveMedia/RTCP.cpp
    liveMedia/RTPInterface.cpp
    liveMedia/RTPSource.cpp
    liveMedia/RTPSink.cpp
    liveMedia/SRTPCryptographicContext.cpp
    liveMedia/BasicUDPSink.cpp
    liveMedia/MediaSession.cpp
    liveMedia/Locale.cpp
    liveMedia/MIKEY.cpp
    liveMedia/Base64.cpp
    liveMedia/SimpleRTPSource.cpp
    liveMedia/MultiFramedRTPSource.cpp
    liveMedia/JPEG2000VideoRTPSource.cpp
    liveMedia/QuickTimeGenericRTPSource.cpp
    liveMedia/JPEGVideoRTPSource.cpp
    liveMedia/DVVideoRTPSource.cpp
    liveMedia/H265VideoRTPSource.cpp
    liveMedia/H264VideoRTPSource.cpp
    liveMedia/H261VideoRTPSource.cpp
    liveMedia/H263plusVideoRTPSource.cpp
    liveMedia/MPEG2TransportStreamFramer.cpp
    liveMedia/FramedFilter.cpp
    liveMedia/MPEG1or2VideoRTPSource.cpp
    liveMedia/MPEG4GenericRTPSource.cpp
    liveMedia/MPEG4LATMAudioRTPSource.cpp
    liveMedia/MPEG4ESVideoRTPSource.cpp
    liveMedia/VP9VideoRTPSource.cpp
    liveMedia/VP8VideoRTPSource.cpp
    liveMedia/AC3AudioRTPSource.cpp
    liveMedia/BitVector.cpp
    liveMedia/RawVideoRTPSource.cpp
    liveMedia/TheoraVideoRTPSource.cpp
    liveMedia/MP3ADU.cpp
    liveMedia/MP3ADUdescriptor.cpp
    liveMedia/MP3Internals.cpp
    liveMedia/MP3InternalsHuffman.cpp
    liveMedia/MP3InternalsHuffmanTable.cpp
    liveMedia/VorbisAudioRTPSource.cpp
    liveMedia/MP3ADUinterleaving.cpp
    liveMedia/MP3ADURTPSource.cpp
    liveMedia/AMRAudioRTPSource.cpp
    liveMedia/AMRAudioSource.cpp
    liveMedia/MPEG1or2AudioRTPSource.cpp
    liveMedia/QCELPAudioRTPSource.cpp
    liveMedia/BasicUDPSource.cpp
    liveMedia/DVVideoStreamFramer.cpp
    liveMedia/MPEG1or2VideoStreamDiscreteFramer.cpp
    liveMedia/MPEG1or2VideoStreamFramer.cpp
    liveMedia/StreamParser.cpp
    liveMedia/MPEGVideoStreamFramer.cpp
    liveMedia/MPEGVideoStreamParser.cpp
    liveMedia/H265VideoStreamDiscreteFramer.cpp
    liveMedia/H264or5VideoStreamDiscreteFramer.cpp
    liveMedia/H264or5VideoStreamFramer.cpp
    liveMedia/H264VideoStreamDiscreteFramer.cpp
    liveMedia/MPEG4VideoStreamDiscreteFramer.cpp
    liveMedia/MPEG4VideoStreamFramer.cpp
    liveMedia/VP8VideoRTPSink.cpp
    liveMedia/VideoRTPSink.cpp
    liveMedia/MultiFramedRTPSink.cpp
    liveMedia/VP9VideoRTPSink.cpp
    liveMedia/VorbisAudioRTPSink.cpp
    liveMedia/AudioRTPSink.cpp
    liveMedia/TheoraVideoRTPSink.cpp
    liveMedia/T140TextRTPSink.cpp
    liveMedia/TextRTPSink.cpp
    liveMedia/SimpleRTPSink.cpp
    liveMedia/MPEG1or2VideoRTPSink.cpp
    liveMedia/MPEG4GenericRTPSink.cpp
    liveMedia/MP3ADURTPSink.cpp
    liveMedia/MPEG1or2AudioRTPSink.cpp
    liveMedia/MPEG4ESVideoRTPSink.cpp
    liveMedia/H265VideoRTPSink.cpp
    liveMedia/MPEG4ESVideoRTPSink.cpp
    liveMedia/H264VideoRTPSink.cpp
    liveMedia/H264or5VideoRTPSink.cpp
    liveMedia/RTSPCommon.cpp
    liveMedia/MPEG4LATMAudioRTPSink.cpp
    liveMedia/DVVideoRTPSink.cpp
    liveMedia/H263plusVideoRTPSink.cpp
    liveMedia/GSMAudioRTPSink.cpp
    liveMedia/AC3AudioRTPSink.cpp
    liveMedia/rtcp_from_spec.c





    BasicUsageEnvironment/BasicHashTable.cpp
    BasicUsageEnvironment/BasicTaskScheduler.cpp
    BasicUsageEnvironment/BasicTaskScheduler0.cpp
    BasicUsageEnvironment/DelayQueue.cpp
    BasicUsageEnvironment/BasicUsageEnvironment.cpp
    BasicUsageEnvironment/BasicUsageEnvironment0.cpp


    
    UsageEnvironment/HashTable.cpp
    UsageEnvironment/strDup.cpp
    UsageEnvironment/UsageEnvironment.cpp

    groupsock/inet.c
    groupsock/GroupsockHelper.cpp
    groupsock/NetAddress.cpp
    groupsock/NetInterface.cpp
    groupsock/Groupsock.cpp
    groupsock/GroupEId.cpp
)

add_library(live OBJECT
  ${src}
)
target_compile_options(live PRIVATE
   -Wno-unused-variable
   -Wno-unused-function
)