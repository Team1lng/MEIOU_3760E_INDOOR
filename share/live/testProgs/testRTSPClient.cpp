/**********
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 3 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
**********/
// Copyright (c) 1996-2022, Live Networks, Inc.  All rights reserved
// A demo application, showing how to create and run a RTSP client (that can potentially receive multiple streams concurrently).
//
// NOTE: This code - although it builds a running application - is intended only to illustrate how to develop your own RTSP
// client application.  For a full-featured RTSP client application - with much more functionality, and many options - see
// "openRTSP": http://www.live555.com/openRTSP/
#include <atomic>
#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"

#ifdef __cplusplus
extern "C"
{
#endif
#include "Amy_sdk.h"
#include "Amy_log.h"
#include "Amy_lvgl.h"
#include "audio_output.h"
#include "audio_input.h"
#include "video_decode.h"
#include "ak_common_graphics.h"
#include "ak_common.h"
#include "ak_mem.h"
#include "mempool/mempool.h"
#include "avilib/video_record.h"
#include "tuya/tuya_sdk.h"
#include "Amy_media_mgr.h"
#include "Amy_snapshot.h"
#include "tuya/TuyaOS/vendor/ak3760e/tuyaos/components/base_hal/test/video/tuya_g711_utils.h"
#include "sat_user_common.h"
    typedef struct rtsp_client_context
    {
        Amy_mutex_t mutex;
        Amy_thread_t tid;
        TaskScheduler *g_scheduler;
        EventTriggerId g_exit_trigger;
        char not_run;

        mp_pool *mp;

        char *sps;
        int sps_size;
        char *pps;
        int pps_size;

        int request_record;
        int (*recvideo_cb)(const char *file, int);
        int request_snapshort;
        int (*snapshort_cb)(const char *data, int size);

        RTSPClient *our_rtsp_client;
        video_decode_context *video_dev;
        audio_output_context *ao_dev;
        audio_input_context *ai_dev;
        avi_record_context *avi_dev;
        int volume;
        int samplerate;
        int ao_cleared;
        char *au_rawdata;
        unsigned int au_rawsize;

        char uri[128];
        char ifname[16]; // 拉流绑定的接收网卡（door=br0 / cctv=wlan0）
        int dx, dy, dw, dh;
        int width, height;
        int wait_next_idr;             // 丢包/截断后等待下一帧IDR再恢复送解码
        int decode_ready;              // 已向解码器送入完整关键帧(SPS+PPS+IDR)，P帧可放行
        unsigned int truncated_frames; // 统计RTP截断帧

    } rtsp_client_context;
#ifdef __cplusplus
}
#endif

// setReceivingInterface 定义于 testOnDemandRTSPServer.cpp（extern "C"），
// 仅设置接收绑定接口（ReceivingInterfaceAddr），不影响服务端发送接口
extern "C" int setReceivingInterface(const char *ipAddr);

// Forward function definitions:
// RTSP 'response handlers':
void continueAfterDESCRIBE(RTSPClient *rtspClient, int resultCode, char *resultString);
void continueAfterSETUP(RTSPClient *rtspClient, int resultCode, char *resultString);
void continueAfterPLAY(RTSPClient *rtspClient, int resultCode, char *resultString);

// Other event handler functions:
void subsessionAfterPlaying(void *clientData); // called when a stream's subsession (e.g., audio or video substream) ends
void subsessionByeHandler(void *clientData, char const *reason);

// called when a RTCP "BYE" is received for a subsession
void streamTimerHandler(void *clientData);

// called at the end of a stream's expected duration (if the stream has not already signaled its end using a RTCP "BYE")
// The main streaming routine (for each "rtsp://" URL):
void openURL(UsageEnvironment &env, char const *progName, rtsp_client_context *client_ctx);

// Used to iterate through each stream's 'subsessions', setting up each one:
void setupNextSubsession(RTSPClient *rtspClient);

// Used to shut down and close a stream (including its "RTSPClient" object):
void shutdownStream(RTSPClient *rtspClient, int exitCode = 1);

// A function that outputs a string that identifies each stream (for debugging output).  Modify this if you wish:
UsageEnvironment &operator<<(UsageEnvironment &env, const RTSPClient &rtspClient)
{
    return env << "[URL:\"" << rtspClient.url() << "\"]: ";
}

// A function that outputs a string that identifies each subsession (for debugging output).  Modify this if you wish:
UsageEnvironment &operator<<(UsageEnvironment &env, const MediaSubsession &subsession)
{
    return env << subsession.mediumName() << "/" << subsession.codecName();
}

void usage(UsageEnvironment &env, char const *progName)
{
    env << "Usage: " << progName << " <rtsp-url-1> ... <rtsp-url-N>\n";
    env << "\t(where each <rtsp-url-i> is a \"rtsp://\" URL)\n";
}

// 线程函数：RTSP流处理任务
extern "C" void *rtsp_streaming_task(void *arg)
{
    // 初始化Live555调度器和环境
    TaskScheduler *scheduler = BasicTaskScheduler::createNew();
    BasicUsageEnvironment *env = BasicUsageEnvironment::createNew(*scheduler);

    rtsp_client_context *ctx = (rtsp_client_context *)arg;
    if (ctx == NULL)
    {
        env->reclaim();
        delete scheduler;
        return NULL;
    }

    // 加锁保护上下文核心变量赋值
    Amy_mutex_lock(&ctx->mutex);
    ctx->g_scheduler = scheduler;
    ctx->g_exit_trigger = env->taskScheduler().createEventTrigger(NULL);
    Amy_mutex_unlock(&ctx->mutex);

    // 打开RTSP流（传入上下文）
    // 按调用方指定网卡设置本路拉流的接收绑定接口（仅 ReceivingInterfaceAddr，
    // 不影响服务端 SendingInterfaceAddr）：door=br0 / cctv=wlan0。
    // 必须在 openURL 创建 RTP/RTCP socket 与组播 join 之前执行。
    if (ctx->ifname[0] != '\0')
    {
        char local_ip[32] = {0};
        if (sat_ip_mac_addres_get(ctx->ifname, local_ip, NULL, NULL) && local_ip[0] != '\0')
            setReceivingInterface(local_ip);
    }
    openURL(*env, NULL, ctx);
    env->setUserData(ctx);

    // 运行事件循环（直到not_run被置1）
    env->taskScheduler().doEventLoop(&ctx->not_run);

    // 关闭RTSP流（加锁保护）
    Amy_mutex_lock(&ctx->mutex);
    if (ctx->our_rtsp_client != NULL)
    {
        shutdownStream(ctx->our_rtsp_client);
        ctx->our_rtsp_client = NULL; // 主动置空，避免主线程重复操作
    }
    Amy_mutex_unlock(&ctx->mutex);

    // 释放Live555资源
    env->reclaim();
    env = NULL;

    // delete scheduler;
    // scheduler = NULL;

    // 清理上下文（加锁）
    // Amy_mutex_lock(&ctx->mutex);
    // ctx->g_scheduler = NULL;
    // ctx->g_exit_trigger = 0;
    // Amy_mutex_unlock(&ctx->mutex);

    return NULL; // 线程函数返回void*，规范返回
}

// 启动RTSP客户端
extern "C" rtsp_client_context *live555_rtsp_client_start(const char *rtsp_uri, const char *ifname, int dx, int dy, int dw, int dh)
{
    // 参数校验
    if (rtsp_uri == NULL || strlen(rtsp_uri) > 127)
    {
        AMY_LOG_ERROR("rtsp uri invalid (length over 127) \n");
        return NULL;
    }

    // 初始化内存池
    int size = sizeof(rtsp_client_context) + 1 * 1024 * 1024;
    mp_pool *mp = Amy_mempool_init(size, size);
    if (mp == NULL)
    {
        AMY_LOG_ERROR("mempool init failed\n");
        return NULL;
    }

    // 分配上下文内存
    rtsp_client_context *ctx = (rtsp_client_context *)Amy_mempool_alloc(mp, sizeof(rtsp_client_context));
    if (!ctx)
    {
        AMY_LOG_ERROR("rtsp client context allocate failed\n");
        Amy_mempool_destory(mp);
        return NULL;
    }

    // 初始化上下文
    memset(ctx, 0, sizeof(rtsp_client_context));
    strncpy(ctx->uri, rtsp_uri, sizeof(ctx->uri) - 1); // 预留结束符
    if (ifname != NULL)
        strncpy(ctx->ifname, ifname, sizeof(ctx->ifname) - 1); // 预留结束符
    ctx->mp = mp;
    ctx->dx = dx;
    ctx->dy = dy;
    ctx->dw = dw;
    ctx->dh = dh;
    ctx->not_run = 0;
    ctx->request_record = 0;
    ctx->request_snapshort = 0;
    ctx->samplerate = 16000;
    ctx->volume = 80;

    // 初始化互斥锁（核心：保护所有上下文变量）
    if (Amy_mutex_init(&ctx->mutex, NULL) != 0)
    {
        AMY_LOG_ERROR("mutex init failed\n");
        Amy_mempool_free(mp, ctx);
        Amy_mempool_destory(mp);
        return NULL;
    }
    // 初始化音频输出设备
    if (!ctx->ao_dev)
    {
        ctx->ao_dev = audio_output_open(1, ctx->samplerate, 60, 16 * 1024);
        ctx->ao_cleared = 0;
        audio_output_set_volume(ctx->ao_dev, ctx->volume * 10);
        if (!ctx->ao_dev)
        {
            AMY_LOG_WARNING("audio output open failed, continue without audio\n");
        }
    }
    else
    {
        int chn, rate;
        audio_output_param_get(&chn, &rate, NULL);
        if ((chn != 1) || (rate != ctx->samplerate))
        {
            audio_output_reset_params(ctx->ao_dev, 1, ctx->samplerate);
            audio_output_set_volume(ctx->ao_dev, ctx->volume * 10);
            ctx->ao_cleared = 0;
        }
    }

    // 创建RTSP处理线程
    Amy_pthread_create(&ctx->tid, rtsp_streaming_task, ctx, 50 * 1024, 12);

    AMY_LOG_INFO("live555 rtsp stream client start %p\n", ctx);
    return ctx;
}

// 请求开始录制
extern "C" int live555_rtsp_request_record_start(rtsp_client_context *d, int (*cb)(const char *, int))
{
    if (d == NULL)
    {
        AMY_LOG_ERROR("invalid context pointer\n");
        return -1;
    }

    Amy_mutex_lock(&d->mutex);
    if (d->avi_dev)
    {
        AMY_LOG_WARNING("record already open\n");
        Amy_mutex_unlock(&d->mutex);
        return -1;
    }
    d->recvideo_cb = cb;
    d->request_record = 1;
    Amy_mutex_unlock(&d->mutex);
    return 0;
}

// 请求停止录制
extern "C" int live555_rtsp_request_record_stop(rtsp_client_context *d)
{
    if (d == NULL)
    {
        AMY_LOG_ERROR("invalid context pointer\n");
        return -1;
    }

    Amy_mutex_lock(&d->mutex);
    d->request_record = 0;
    if (!d->avi_dev)
    {
        AMY_LOG_WARNING("record already closed\n");
        Amy_mutex_unlock(&d->mutex);
        return -1;
    }
    if (d->ai_dev)
    {
        audio_input_close(d->ai_dev);
        d->ai_dev = NULL;
    }
    avi_record_close(d->avi_dev);
    d->avi_dev = NULL;
    d->recvideo_cb = NULL;
    Amy_mutex_unlock(&d->mutex);
    return 0;
}

// 请求开始截图
extern "C" int live555_rtsp_request_snapshort_start(rtsp_client_context *d, int (*snapshort_cb)(const char *, int))
{
    if (d == NULL)
    {
        AMY_LOG_ERROR("invalid context pointer\n");
        return -1;
    }

    Amy_mutex_lock(&d->mutex);
    if (d->request_snapshort)
    {
        AMY_LOG_WARNING("snapshot already requested\n");
        Amy_mutex_unlock(&d->mutex);
        return -1;
    }
    d->snapshort_cb = snapshort_cb;
    d->request_snapshort = 1;
    Amy_mutex_unlock(&d->mutex);
    return 0;
}

extern "C" int live555_rtsp_request_snapshort_cancel(rtsp_client_context *ctx)
{
    if (!ctx)
        return -1;
    int (*callback)(const char *, int) = NULL;
    Amy_mutex_lock(&ctx->mutex);
    if (ctx->request_snapshort)
    {
        ctx->request_snapshort = 0;
        callback = ctx->snapshort_cb;
        ctx->snapshort_cb = NULL;
    }
    Amy_mutex_unlock(&ctx->mutex);
    if (callback)
        callback(NULL, 0);
    return 0;
}

extern "C" int live555_rtsp_request_volume_set(rtsp_client_context *ctx, int volume)
{
    if (ctx == NULL)
    {
        AMY_LOG_ERROR("invalid context pointer\n");
        return -1;
    }

    Amy_mutex_lock(&ctx->mutex);
    ctx->volume = volume;
    // 初始化音频输出设备
    if (!ctx->ao_dev)
    {
        ctx->ao_dev = audio_output_open(1, ctx->samplerate, 60, 16 * 1024);
        ctx->ao_cleared = 0;
        if (!ctx->ao_dev)
        {
            AMY_LOG_WARNING("audio output open failed, continue without audio\n");
            Amy_mutex_unlock(&ctx->mutex);
            return -1;
        }
    }
    else
    {
        int chn, rate;
        audio_output_param_get(&chn, &rate, NULL);
        if ((chn != 1) || (rate != ctx->samplerate))
        {
            audio_output_reset_params(ctx->ao_dev, 1, ctx->samplerate);
            ctx->ao_cleared = 0;
        }
    }
    // 无论参数是否变化都应用目标音量
    audio_output_set_volume(ctx->ao_dev, ctx->volume);
    Amy_mutex_unlock(&ctx->mutex);
    return 0;
}

// 关闭RTSP客户端
extern "C" int live555_rtsp_client_close(rtsp_client_context *d)
{
    if (d == NULL)
    {
        AMY_LOG_ERROR("invalid context pointer\n");
        return -1;
    }

    mp_pool *mp = d->mp;
    int ret = 0;

    // 加锁判断并修改退出标记，避免竞态
    Amy_mutex_lock(&d->mutex);

    // 标记退出事件循环
    d->not_run = 1;

    Amy_mutex_unlock(&d->mutex);

    // 唤醒Live555事件循环（无锁操作局部变量）
    if (d->g_scheduler && d->g_exit_trigger)
    {
        d->g_scheduler->triggerEvent(d->g_exit_trigger, NULL);
    }

    // 等待子线程退出
    Amy_pthread_join(d->tid, NULL);

    Amy_mutex_lock(&d->mutex);
    if (d->g_scheduler)
    {
        delete d->g_scheduler;
        d->g_scheduler = NULL;
        d->g_exit_trigger = 0;
    }
    Amy_mutex_unlock(&d->mutex);
    AMY_LOG_INFO("live555 rtsp stream client stop \n");

    if (d->avi_dev)
    {
        avi_record_close(d->avi_dev);
        d->avi_dev = NULL;
    }
    if (d->ai_dev)
    {
        audio_input_close(d->ai_dev);
        d->ai_dev = NULL;
    }
    if (d->ao_dev)
    {
        audio_output_close(d->ao_dev);
        d->ao_dev = NULL;
    }
    // 释放视频资源（加锁保护，避免子线程同时访问）
    Amy_mutex_lock(&d->mutex);
    if (d->video_dev)
    {
        video_decodec_close(d->video_dev);
        d->video_dev = NULL;
    }
    Amy_mutex_unlock(&d->mutex);
    // 释放内存资源
    if (d->au_rawdata)
    {
        Amy_mempool_free(mp, d->au_rawdata);
        d->au_rawdata = NULL;
    }
    Amy_mutex_destroy(&d->mutex); // 销毁互斥锁
    Amy_mempool_free(mp, d);
    Amy_mempool_destory(mp);

    return ret;
}
// Define a class to hold per-stream state that we maintain throughout each stream's lifetime:
class StreamClientState
{

public:
    StreamClientState();
    virtual ~StreamClientState();

public:
    MediaSubsessionIterator *iter;
    MediaSession *session;
    MediaSubsession *subsession;
    TaskToken streamTimerTask;
    double duration;
};

// If you're streaming just a single stream (i.e., just from a single URL, once), then you can define and use just a single
// "StreamClientState" structure, as a global variable in your application.  However, because - in this demo application - we're
// showing how to play multiple streams, concurrently, we can't do that.  Instead, we have to have a separate "StreamClientState"
// structure for each "RTSPClient".  To do this, we subclass "RTSPClient", and add a "StreamClientState" field to the subclass:
class ourRTSPClient : public RTSPClient
{

public:
    static ourRTSPClient *createNew(UsageEnvironment &env, char const *rtspURL,
                                    int verbosityLevel = 0,
                                    char const *applicationName = NULL,
                                    portNumBits tunnelOverHTTPPortNum = 0);

protected:
    ourRTSPClient(UsageEnvironment &env, char const *rtspURL,
                  int verbosityLevel, char const *applicationName, portNumBits tunnelOverHTTPPortNum);

    // called only by createNew();
    virtual ~ourRTSPClient();

public:
    StreamClientState scs;
};

// Define a data sink (a subclass of "MediaSink") to receive the data for each subsession (i.e., each audio or video 'substream').
// In practice, this might be a class (or a chain of classes) that decodes and then renders the incoming audio or video.
// Or it might be a "FileSink", for outputting the received data into a file (as is done by the "openRTSP" application).
// In this example code, however, we define a simple 'dummy' sink that receives incoming data, but does nothing with it.
class DummySink : public MediaSink
{

public:
    static DummySink *createNew(UsageEnvironment &env,
                                MediaSubsession &subsession,  // identifies the kind of data that's being received
                                char const *streamId = NULL); // identifies the stream itself (optional)
    static unsigned read_ue(const uint8_t *data, unsigned *bit_offset, unsigned max_bits);
    int get_h264_resolution_fast(const uint8_t *sps, unsigned sps_size, int &width, int &height);

private:
    DummySink(UsageEnvironment &env, MediaSubsession &subsession, char const *streamId);

    // called only by "createNew()"
    virtual ~DummySink();

    static void afterGettingFrame(void *clientData, unsigned frameSize,
                                  unsigned numTruncatedBytes,
                                  struct timeval presentationTime,
                                  unsigned durationInMicroseconds);
    void afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes,
                           struct timeval presentationTime, unsigned durationInMicroseconds);

public:
    // 公共接口：控制音频发送状态
    static int openClientSpeaker();
    static int closeClientSpeaker();

private:
    // redefined virtual functions:
    virtual Boolean continuePlaying();

    void updateSPS(rtsp_client_context *ctx, int frameSize);
    void updatePPS(rtsp_client_context *ctx, int frameSize);
    void updateDispayDevice(rtsp_client_context *ctx);
    void updateAudioDevice(rtsp_client_context *ctx, int rate);
    void sendSPSToDev(rtsp_client_context *ctx);
    void sendPPSToDev(rtsp_client_context *ctx);
    void sendIDRToDev(rtsp_client_context *ctx, unsigned int frameSize);
    void sendPPPToDev(rtsp_client_context *ctx, unsigned int frameSize);
    void sendAudioToDev(rtsp_client_context *ctx, unsigned int frameSize);
    void sendAudioToAvi(rtsp_client_context *rtsp_context);
    void *userData;
    // 静态成员变量：音频接收状态（多线程读写，需原子操作保证可见性）
    static std::atomic<Boolean> fAudioReceive;

private:
    u_int8_t *fReceiveBuffer;
    unsigned fReceiveBufferSize;
    MediaSubsession &fSubsession;
    char *fStreamId;
};

// 初始化类的静态成员变量
std::atomic<Boolean> DummySink::fAudioReceive(False);

// 实现类的静态成员函数：允许客户端音频输出到扬声器
int DummySink::openClientSpeaker()
{
    fAudioReceive.store(True, std::memory_order_release);
    return 0;
}

// 实现类的静态成员函数：关闭客户端音频输出到扬声器
int DummySink::closeClientSpeaker()
{
    fAudioReceive.store(False, std::memory_order_release);
    return 0;
}

// 实现外部C接口函数：允许客户端音频输出到扬声器
extern "C" int open_client_speaker(void)
{
    return DummySink::openClientSpeaker();
}

// 实现外部C接口函数：关闭客户端音频输出到扬声器
extern "C" int close_client_speaker(void)
{
    return DummySink::closeClientSpeaker();
}

#define RTSP_CLIENT_VERBOSITY_LEVEL 1 // by default, print verbose output from each "RTSPClient"

static unsigned rtspClientCount = 0; // Counts how many streams (i.e., "RTSPClient"s) are currently in use.

void openURL(UsageEnvironment &env, char const *progName, rtsp_client_context *client_ctx)
{
    OutPacketBuffer::maxSize = 4 * 1024 * 1024; // 客户端拼帧缓冲区，解决马赛克核心

    // Begin by creating a "RTSPClient" object.  Note that there is a separate "RTSPClient" object for each stream that we wish
    // to receive (even if more than stream uses the same "rtsp://" URL).
    RTSPClient *rtspClient = ourRTSPClient::createNew(env, client_ctx->uri, RTSP_CLIENT_VERBOSITY_LEVEL, progName);

    if (rtspClient == NULL)
    {
        env << "Failed to create a RTSP client for URL \"" << client_ctx->uri << "\": " << env.getResultMsg() << "\n";
        return;
    }
    client_ctx->our_rtsp_client = rtspClient;
    ++rtspClientCount;

    // Next, send a RTSP "DESCRIBE" command, to get a SDP description for the stream.
    // Note that this command - like all RTSP commands - is sent asynchronously; we do not block, waiting for a response.
    // Instead, the following function call returns immediately, and we handle the RTSP response later, from within the event loop:
    rtspClient->sendDescribeCommand(continueAfterDESCRIBE);
}

// Implementation of the RTSP 'response handlers':
void continueAfterDESCRIBE(RTSPClient *rtspClient, int resultCode, char *resultString)
{
    do
    {
        UsageEnvironment &env = rtspClient->envir();                 // alias
        StreamClientState &scs = ((ourRTSPClient *)rtspClient)->scs; // alias

        if (resultCode != 0)
        {
            env << *rtspClient << "Failed to get a SDP description: " << resultString << "\n";
            delete[] resultString;
            break;
        }

        char *const sdpDescription = resultString;

        env << *rtspClient << "Got a SDP description:\n"
            << sdpDescription << "\n";

        // Create a media session object from this SDP description:
        scs.session = MediaSession::createNew(env, sdpDescription);
        delete[] sdpDescription; // because we don't need it anymore

        if (scs.session == NULL)
        {
            env << *rtspClient << "Failed to create a MediaSession object from the SDP description: " << env.getResultMsg() << "\n";
            break;
        }
        else if (!scs.session->hasSubsessions())
        {
            env << *rtspClient << "This session has no media subsessions (i.e., no \"m=\" lines)\n";
            break;
        }

        // Then, create and set up our data source objects for the session.  We do this by iterating over the session's 'subsessions',
        // calling "MediaSubsession::initiate()", and then sending a RTSP "SETUP" command, on each one.
        // (Each 'subsession' will have its own data source.)
        scs.iter = new MediaSubsessionIterator(*scs.session);
        setupNextSubsession(rtspClient);
        return;
    } while (0);
    // An unrecoverable error occurred with this stream.
    // shutdownStream(rtspClient);
}

// By default, we request that the server stream its data using RTP/UDP.
// If, instead, you want to request that the server stream via RTP-over-TCP, change the following to True:
#define REQUEST_STREAMING_OVER_TCP False
#define RTP_PACKET_REORDERING_THRESHOLD_US 1000000
unsigned getReceiveBufferSize(UsageEnvironment &env, int socket);
unsigned setReceiveBufferTo(UsageEnvironment &env, int socket, unsigned requestedSize);
void setupNextSubsession(RTSPClient *rtspClient)
{
    UsageEnvironment &env = rtspClient->envir();                 // alias
    StreamClientState &scs = ((ourRTSPClient *)rtspClient)->scs; // alias

    scs.subsession = scs.iter->next();

    if (scs.subsession != NULL)
    {
        if (!scs.subsession->initiate())
        {
            env << *rtspClient << "Failed to initiate the \"" << *scs.subsession << "\" subsession: " << env.getResultMsg() << "\n";
            setupNextSubsession(rtspClient); // give up on this subsession; go to the next one
        }
        else
        {
            // 重新配置RTP接收参数（UDP组播场景下尤其重要）
            if (scs.subsession->rtpSource() != NULL)
            {
                unsigned const thresh = RTP_PACKET_REORDERING_THRESHOLD_US;
                scs.subsession->rtpSource()->setPacketReorderingThresholdTime(thresh);
                int socketNum = scs.subsession->rtpSource()->RTPgs()->socketNum();
                unsigned curBufferSize = setReceiveBufferTo(env, socketNum, 2 * 1024 * 1024);
                env << *rtspClient << "RTP socket recv buffer set to " << curBufferSize
                    << ", reorder threshold " << thresh << " us\n";
            }

            env << *rtspClient << "Initiated the \"" << *scs.subsession << "\" subsession (";
            if (scs.subsession->rtcpIsMuxed())
            {
                env << "client port " << scs.subsession->clientPortNum();
            }
            else
            {
                env << "client ports " << scs.subsession->clientPortNum() << "-" << scs.subsession->clientPortNum() + 1;
            }
            env << ")\n";
            // Continue setting up this subsession, by sending a RTSP "SETUP" command:
            rtspClient->sendSetupCommand(*scs.subsession, continueAfterSETUP, False, REQUEST_STREAMING_OVER_TCP);
        }
        return;
    }

    // We've finished setting up all of the subsessions.  Now, send a RTSP "PLAY" command to start the streaming:
    if (scs.session->absStartTime() != NULL)
    {
        // Special case: The stream is indexed by 'absolute' time, so send an appropriate "PLAY" command:
        rtspClient->sendPlayCommand(*scs.session, continueAfterPLAY, scs.session->absStartTime(), scs.session->absEndTime());
    }
    else
    {
        scs.duration = scs.session->playEndTime() - scs.session->playStartTime();
        rtspClient->sendPlayCommand(*scs.session, continueAfterPLAY);
    }
}

void continueAfterSETUP(RTSPClient *rtspClient, int resultCode, char *resultString)
{
    do
    {
        UsageEnvironment &env = rtspClient->envir();                 // alias
        StreamClientState &scs = ((ourRTSPClient *)rtspClient)->scs; // alias

        if (resultCode != 0)
        {
            env << *rtspClient << "Failed to set up the \"" << *scs.subsession << "\" subsession: " << resultString << "\n";
            break;
        }

        env << *rtspClient << "Set up the \"" << *scs.subsession << "\" subsession (";

        if (scs.subsession->rtcpIsMuxed())
        {
            env << "client port " << scs.subsession->clientPortNum();
        }
        else
        {
            env << "client ports " << scs.subsession->clientPortNum() << "-" << scs.subsession->clientPortNum() + 1;
        }

        env << ")\n";
        // ---- 从 SDP 中提取音频采样率并写入上下文 ----
        if (strncmp(scs.subsession->mediumName(), "audio", 5) == 0)
        {
            BasicUsageEnvironment *basicEnv = dynamic_cast<BasicUsageEnvironment *>(&env);
            rtsp_client_context *ctx = basicEnv ? (rtsp_client_context *)basicEnv->getUserData() : NULL;
            if (ctx)
            {
                unsigned sdp_rate = scs.subsession->rtpTimestampFrequency();
                if (sdp_rate > 0 && sdp_rate != (unsigned)ctx->samplerate)
                {
                    AMY_LOG_INFO("SDP audio sample rate: %u Hz\n", sdp_rate);
                    ctx->samplerate = (int)sdp_rate;
                }
            }
        }
        // Having successfully setup the subsession, create a data sink for it, and call "startPlaying()" on it.
        // (This will prepare the data sink to receive data; the actual flow of data from the client won't start happening until later,
        // after we've sent a RTSP "PLAY" command.)
        scs.subsession->sink = DummySink::createNew(env, *scs.subsession, rtspClient->url());

        // perhaps use your own custom "MediaSink" subclass instead
        if (scs.subsession->sink == NULL)
        {
            env << *rtspClient << "Failed to create a data sink for the \"" << *scs.subsession << "\" subsession: " << env.getResultMsg() << "\n";
            break;
        }

        env << *rtspClient << "Created a data sink for the \"" << *scs.subsession << "\" subsession\n";
        scs.subsession->miscPtr = rtspClient; // a hack to let subsession handler functions get the "RTSPClient" from the subsession
        scs.subsession->sink->startPlaying(*(scs.subsession->readSource()), subsessionAfterPlaying, scs.subsession);

        // Also set a handler to be called if a RTCP "BYE" arrives for this subsession:
        if (scs.subsession->rtcpInstance() != NULL)
        {
            scs.subsession->rtcpInstance()->setByeWithReasonHandler(subsessionByeHandler, scs.subsession);
        }
    } while (0);

    delete[] resultString;

    // Set up the next subsession, if any:
    setupNextSubsession(rtspClient);
}

void continueAfterPLAY(RTSPClient *rtspClient, int resultCode, char *resultString)
{
    Boolean success = False;

    do
    {
        UsageEnvironment &env = rtspClient->envir();                 // alias
        StreamClientState &scs = ((ourRTSPClient *)rtspClient)->scs; // alias

        if (resultCode != 0)
        {
            env << *rtspClient << "Failed to start playing session: " << resultString << "\n";
            break;
        }

        // Set a timer to be handled at the end of the stream's expected duration (if the stream does not already signal its end
        // using a RTCP "BYE").  This is optional.  If, instead, you want to keep the stream active - e.g., so you can later
        // 'seek' back within it and do another RTSP "PLAY" - then you can omit this code.
        // (Alternatively, if you don't want to receive the entire stream, you could set this timer for some shorter value.)
        if (scs.duration > 0)
        {
            unsigned const delaySlop = 2; // number of seconds extra to delay, after the stream's expected duration.  (This is optional.)

            scs.duration += delaySlop;
            unsigned uSecsToDelay = (unsigned)(scs.duration * 1000000);

            scs.streamTimerTask = env.taskScheduler().scheduleDelayedTask(uSecsToDelay, (TaskFunc *)streamTimerHandler, rtspClient);
        }

        env << *rtspClient << "Started playing session";

        if (scs.duration > 0)
        {
            env << " (for up to " << scs.duration << " seconds)";
        }

        env << "...\n";

        success = True;
    } while (0);

    delete[] resultString;

    if (!success)
    {
        // An unrecoverable error occurred with this stream.
        shutdownStream(rtspClient);
    }
}

// Implementation of the other event handlers:
void subsessionAfterPlaying(void *clientData)
{
    MediaSubsession *subsession = (MediaSubsession *)clientData;
    RTSPClient *rtspClient = (RTSPClient *)(subsession->miscPtr);

    // Begin by closing this subsession's stream:
    Medium::close(subsession->sink);
    subsession->sink = NULL;

    // Next, check whether *all* subsessions' streams have now been closed:
    MediaSession &session = subsession->parentSession();
    MediaSubsessionIterator iter(session);

    while ((subsession = iter.next()) != NULL)
    {
        if (subsession->sink != NULL)
            return; // this subsession is still active
    }
    // All subsessions' streams have now been closed, so shutdown the client:
    shutdownStream(rtspClient);
}

void subsessionByeHandler(void *clientData, char const *reason)
{
    MediaSubsession *subsession = (MediaSubsession *)clientData;
    RTSPClient *rtspClient = (RTSPClient *)subsession->miscPtr;
    UsageEnvironment &env = rtspClient->envir(); // alias

    env << *rtspClient << "Received RTCP \"BYE\"";

    if (reason != NULL)
    {
        env << " (reason:\"" << reason << "\")";
        delete[] (char *)reason;
    }

    env << " on \"" << *subsession << "\" subsession\n";

    // Now act as if the subsession had closed:
    subsessionAfterPlaying(subsession);
}

void streamTimerHandler(void *clientData)
{
    ourRTSPClient *rtspClient = (ourRTSPClient *)clientData;
    StreamClientState &scs = rtspClient->scs; // alias

    scs.streamTimerTask = NULL;
    // Shut down the stream:
    shutdownStream(rtspClient);
}

void shutdownStream(RTSPClient *rtspClient, int exitCode)
{
    UsageEnvironment &env = rtspClient->envir();                 // alias
    StreamClientState &scs = ((ourRTSPClient *)rtspClient)->scs; // alias
    // First, check whether any subsessions have still to be closed:
    if (scs.session != NULL)
    {
        Boolean someSubsessionsWereActive = False;
        MediaSubsessionIterator iter(*scs.session);
        MediaSubsession *subsession;

        while ((subsession = iter.next()) != NULL)
        {
            if (subsession->sink != NULL)
            {
                Medium::close(subsession->sink);
                subsession->sink = NULL;

                if (subsession->rtcpInstance() != NULL)
                {
                    subsession->rtcpInstance()->setByeHandler(NULL, NULL); // in case the server sends a RTCP "BYE" while handling "TEARDOWN"
                }

                someSubsessionsWereActive = True;
            }
        }

        if (someSubsessionsWereActive)
        {
            // Send a RTSP "TEARDOWN" command, to tell the server to shutdown the stream.
            // Don't bother handling the response to the "TEARDOWN".
            rtspClient->sendTeardownCommand(*scs.session, NULL);
        }
    }
    env << *rtspClient << "-------------Closing the stream.\n";
    Medium::close(rtspClient);
    // Note that this will also cause this stream's "StreamClientState" structure to get reclaimed.
    if (--rtspClientCount == 0)
    {
        // The final stream has ended, so exit the application now.
        // (Of course, if you're embedding this code into your own application, you might want to comment this out,
        // and replace it with "eventLoopWatchVariable = 1;", so that we leave the LIVE555 event loop, and continue running "main()".)
        // exit(exitCode);
    }
}

// Implementation of "ourRTSPClient":
ourRTSPClient *ourRTSPClient::createNew(UsageEnvironment &env, char const *rtspURL,
                                        int verbosityLevel, char const *applicationName, portNumBits tunnelOverHTTPPortNum)
{
    return new ourRTSPClient(env, rtspURL, verbosityLevel, applicationName, tunnelOverHTTPPortNum);
}

ourRTSPClient::ourRTSPClient(UsageEnvironment &env, char const *rtspURL,
                             int verbosityLevel, char const *applicationName, portNumBits tunnelOverHTTPPortNum) : RTSPClient(env, rtspURL, verbosityLevel, applicationName, tunnelOverHTTPPortNum, -1)
{
}

ourRTSPClient::~ourRTSPClient()
{
}

// Implementation of "StreamClientState":
StreamClientState::StreamClientState() : iter(NULL), session(NULL), subsession(NULL), streamTimerTask(NULL),
                                         duration(0.0)
{
}

StreamClientState::~StreamClientState()
{
    delete iter;

    if (session != NULL)
    {
        // We also need to delete "session", and unschedule "streamTimerTask" (if set)
        UsageEnvironment &env = session->envir(); // alias

        env.taskScheduler().unscheduleDelayedTask(streamTimerTask);
        Medium::close(session);
    }
}

// Implementation of "DummySink":
// Even though we're not going to be doing anything with the incoming data, we still need to receive it.
// Define the size of the buffer that we'll use:
#define DUMMY_SINK_RECEIVE_BUFFER_SIZE (2 * 1024 * 1024)
#define DUMMY_SINK_AUDIO_RECEIVE_BUFFER_SIZE (64 * 1024)
// 100000
DummySink *DummySink::createNew(UsageEnvironment &env, MediaSubsession &subsession, char const *streamId)
{
    DummySink *sink = new DummySink(env, subsession, streamId);

    BasicUsageEnvironment *basicEnv = dynamic_cast<BasicUsageEnvironment *>(&env);
    if (basicEnv == nullptr)
    {
        AMY_LOG_ERROR("BasicUsageEnvironment* basicEnv = dynamic_cast<BasicUsageEnvironment*>(env);");
    }
    else
    {
        sink->userData = basicEnv->getUserData();
    }
    return sink;
}

DummySink::DummySink(UsageEnvironment &env, MediaSubsession &subsession, char const *streamId) : MediaSink(env),
                                                                                                 fSubsession(subsession)
{
    fStreamId = strDup(streamId);
    unsigned buf_size = DUMMY_SINK_RECEIVE_BUFFER_SIZE;
    if (strncmp(subsession.mediumName(), "audio", 5) == 0)
        buf_size = DUMMY_SINK_AUDIO_RECEIVE_BUFFER_SIZE; // 音频帧就几百字节，64KB 绰绰有余
    fReceiveBuffer = new u_int8_t[buf_size];
    fReceiveBufferSize = buf_size;
}

DummySink::~DummySink()
{
    delete[] fReceiveBuffer;
    delete[] fStreamId;
}

void DummySink::afterGettingFrame(void *clientData, unsigned frameSize, unsigned numTruncatedBytes,
                                  struct timeval presentationTime, unsigned durationInMicroseconds)
{
    DummySink *sink = (DummySink *)clientData;
    sink->afterGettingFrame(frameSize, numTruncatedBytes, presentationTime, durationInMicroseconds);
}

unsigned DummySink::read_ue(const uint8_t *data, unsigned *bit_offset, unsigned max_bits)
{
    int leadingZeroBits = -1;
    unsigned b = 0;

    // 计算前导0的个数
    for (b = 0; !b && *bit_offset < max_bits; leadingZeroBits++)
    {
        b = (data[*bit_offset / 8] >> (7 - (*bit_offset % 8))) & 1;
        (*bit_offset)++;
    }

    if (leadingZeroBits < 0 || *bit_offset + leadingZeroBits > max_bits)
    {
        return 0;
    }

    // 读取剩余的比特
    unsigned codeNum = (1 << leadingZeroBits) - 1;
    for (int i = 0; i < leadingZeroBits; i++)
    {
        if (*bit_offset >= max_bits)
            break;
        b = (data[*bit_offset / 8] >> (7 - (*bit_offset % 8))) & 1;
        codeNum += b << (leadingZeroBits - 1 - i);
        (*bit_offset)++;
    }

    return codeNum;
}
// 最快速的SPS分辨率解析
int DummySink::get_h264_resolution_fast(const uint8_t *sps, unsigned sps_size, int &width, int &height)
{
    if (sps_size < 8)
        return -1;

    unsigned max_bits = sps_size * 8;
    unsigned bit_offset = 8; // 跳过NAL头(0x67)

    // printf("SPS数据: ");
    // for (int i = 0; i < (sps_size < 16 ? sps_size : 16); i++) {
    //   printf("%02x ", sps[i]);
    // }
    // printf("\n");

    // 跳过profile_idc, constraint_flags, level_idc
    bit_offset += 24;

    // 跳过seq_parameter_set_id
    // unsigned seq_param_set_id =
    read_ue(sps, &bit_offset, max_bits);
    // printf("seq_parameter_set_id: %u\n", seq_param_set_id);

    // 处理high profile参数
    if (sps[1] >= 100)
    { // profile_idc
        unsigned chroma_format_idc = read_ue(sps, &bit_offset, max_bits);
        // printf("chroma_format_idc: %u\n", chroma_format_idc);

        if (chroma_format_idc == 3)
        {
            bit_offset++; // separate_colour_plane_flag
        }

        read_ue(sps, &bit_offset, max_bits); // bit_depth_luma_minus8
        read_ue(sps, &bit_offset, max_bits); // bit_depth_chroma_minus8
        bit_offset++;                        // qpprime_y_zero_transform_bypass_flag
    }

    // 跳过log2_max_frame_num_minus4
    read_ue(sps, &bit_offset, max_bits);

    // 处理pic_order_cnt_type
    unsigned pic_order_cnt_type = read_ue(sps, &bit_offset, max_bits);
    // printf("pic_order_cnt_type: %u\n", pic_order_cnt_type);

    if (pic_order_cnt_type == 0)
    {
        read_ue(sps, &bit_offset, max_bits); // log2_max_pic_order_cnt_lsb_minus4
    }
    else if (pic_order_cnt_type == 1)
    {
        bit_offset++;                        // delta_pic_order_always_zero_flag
        read_ue(sps, &bit_offset, max_bits); // offset_for_non_ref_pic
        read_ue(sps, &bit_offset, max_bits); // offset_for_top_to_bottom_field
        unsigned num_ref_frames = read_ue(sps, &bit_offset, max_bits);
        for (unsigned i = 0; i < num_ref_frames; i++)
        {
            read_ue(sps, &bit_offset, max_bits); // offset_for_ref_frame[i]
        }
    }

    // 跳过max_num_ref_frames
    read_ue(sps, &bit_offset, max_bits);

    // 跳过gaps_in_frame_num_value_allowed_flag
    bit_offset++;

    // 读取分辨率参数
    unsigned pic_width_in_mbs_minus1 = read_ue(sps, &bit_offset, max_bits);
    unsigned pic_height_in_map_units_minus1 = read_ue(sps, &bit_offset, max_bits);
    unsigned frame_mbs_only_flag = (sps[bit_offset / 8] >> (7 - (bit_offset % 8))) & 1;
    bit_offset++;

    // printf("pic_width_in_mbs_minus1: %u\n", pic_width_in_mbs_minus1);
    // printf("pic_height_in_map_units_minus1: %u\n", pic_height_in_map_units_minus1);
    // printf("frame_mbs_only_flag: %u\n", frame_mbs_only_flag);

    // 计算基础分辨率
    width = (pic_width_in_mbs_minus1 + 1) * 16;
    height = (2 - frame_mbs_only_flag) * (pic_height_in_map_units_minus1 + 1) * 16;
#if 0
    // 检查帧裁剪
    if (bit_offset < max_bits) {
        unsigned frame_cropping_flag = (sps[bit_offset / 8] >> (7 - (bit_offset % 8))) & 1;
        bit_offset++;

        if (frame_cropping_flag) {
            unsigned frame_crop_left_offset = read_ue(sps, &bit_offset, max_bits);
            unsigned frame_crop_right_offset = read_ue(sps, &bit_offset, max_bits);
            unsigned frame_crop_top_offset = read_ue(sps, &bit_offset, max_bits);
            unsigned frame_crop_bottom_offset = read_ue(sps, &bit_offset, max_bits);

            printf("Frame cropping: left=%u, right=%u, top=%u, bottom=%u\n",
                frame_crop_left_offset, frame_crop_right_offset,
                frame_crop_top_offset, frame_crop_bottom_offset);

            width -= (frame_crop_left_offset + frame_crop_right_offset) * 2;
            height -= (frame_crop_top_offset + frame_crop_bottom_offset) * 2;
        }
    }
#endif
    //    AMY_LOG_INFO("wdith:%d,height:%d\n", width, height);
    return 0;
}
extern "C" int h264_frame_get_cb(video_mode_attr *attr, void *user_data)
{
    rtsp_client_context *rtsp_context = (rtsp_client_context *)user_data;
    struct ak_vdec_frame frame = {0};

    Amy_mutex_lock(&rtsp_context->mutex); // 必须加锁！

    // 先检查video_dev是否合法（双重保险）
    if (rtsp_context->video_dev != NULL)
    {
        if (video_decodec_get_frame(rtsp_context->video_dev, &frame, NULL) == 0)
        {
            attr->sw = frame.frame_obj.data.pitch_width;
            attr->sh = frame.frame_obj.data.pitch_height;
            attr->spx = 0;
            attr->spy = 0;
            attr->spw = frame.width;
            attr->sph = frame.height;
            attr->sfmt = GP_FORMAT_YUV420SP;
            attr->dpx = rtsp_context->dx;
            attr->dpy = rtsp_context->dy;
            attr->dpw = rtsp_context->dw;
            attr->dph = rtsp_context->dh;
            ak_mem_dma_vaddr2paddr((void *)frame.frame_obj.data.data, (unsigned long *)&attr->sphy);
        }
        else
        {
            memset(attr, 0, sizeof(video_mode_attr));
            attr->dpx = rtsp_context->dx;
            attr->dpy = rtsp_context->dy;
            attr->dpw = rtsp_context->dw;
            attr->dph = rtsp_context->dh;
        }
    }
    else
    {
        // video_dev已释放/未初始化，返回空属性
        memset(attr, 0, sizeof(video_mode_attr));
        attr->dpx = rtsp_context->dx;
        attr->dpy = rtsp_context->dy;
        attr->dpw = rtsp_context->dw;
        attr->dph = rtsp_context->dh;
    }

    Amy_mutex_unlock(&rtsp_context->mutex); // 必须解锁！
    return 0;
}

void DummySink::updateSPS(rtsp_client_context *rtsp_context, int frameSize)
{
    if (rtsp_context->sps)
    {
        Amy_mempool_free(rtsp_context->mp, rtsp_context->sps);
    }

    rtsp_context->sps_size = frameSize;
    rtsp_context->sps = (char *)Amy_mempool_alloc(rtsp_context->mp, rtsp_context->sps_size);
    if (rtsp_context->sps)
    {
        memcpy(rtsp_context->sps, fReceiveBuffer, frameSize);
    }
}
void DummySink::updatePPS(rtsp_client_context *rtsp_context, int frameSize)
{
    if (rtsp_context->pps)
    {
        Amy_mempool_free(rtsp_context->mp, rtsp_context->pps);
    }

    rtsp_context->pps_size = frameSize;
    rtsp_context->pps = (char *)Amy_mempool_alloc(rtsp_context->mp, frameSize);
    if (rtsp_context->pps)
    {
        memcpy(rtsp_context->pps, fReceiveBuffer, frameSize);
    }
}

void DummySink::updateDispayDevice(rtsp_client_context *rtsp_context)
{
    int width, height;
    if ((get_h264_resolution_fast((uint8_t *)rtsp_context->sps, rtsp_context->sps_size, width, height) == 0) &&
        (width <= 1920) && (height <= 1088))
    {
        // 切换了分辨率
        if ((rtsp_context->video_dev) && (rtsp_context->width != width) && (rtsp_context->height != height))
        {
            video_decodec_close(rtsp_context->video_dev);
            rtsp_context->video_dev = NULL;
            rtsp_context->decode_ready = 0; // 新解码器需等下次关键帧送达后才能收P帧
        }
        if (!rtsp_context->video_dev)
        {
            rtsp_context->video_dev = video_decodec_open(width, height, H264_ENC_TYPE);

            if (rtsp_context->video_dev)
            {
                rtsp_context->width = width;
                rtsp_context->height = height;

                video_mode_cb_attr attr;
                attr.func = h264_frame_get_cb;
                attr.user_data = rtsp_context;
                Amy_video_mode_event_put(&attr);
            }
        }
    }
}
void DummySink::updateAudioDevice(rtsp_client_context *rtsp_context, int sample_rate)
{
    // 初始化音频输出设备
    if (!rtsp_context->ao_dev)
    {
        rtsp_context->ao_dev = audio_output_open(1, sample_rate, 60, 16 * 1024);
        audio_output_set_volume(rtsp_context->ao_dev, rtsp_context->volume * 10);
        rtsp_context->ao_cleared = 0;
        rtsp_context->samplerate = sample_rate;
    }
    else // 重置音频输出设备
    {
        int chn, rate;
        audio_output_param_get(&chn, &rate, NULL);
        if ((chn != 1) || (rate != rtsp_context->samplerate))
        {
            audio_output_reset_params(rtsp_context->ao_dev, 1, rtsp_context->samplerate);
            audio_output_set_volume(rtsp_context->ao_dev, rtsp_context->volume * 10);
            rtsp_context->ao_cleared = 0;
            rtsp_context->samplerate = sample_rate;
        }
    }
}

void DummySink::sendSPSToDev(rtsp_client_context *ctx)
{
    unsigned char start_code[] = {0, 0, 0, 1};
    stream_data *vdec_s = video_decodec_buf_get(ctx->video_dev, ctx->sps_size + sizeof(start_code));
    if (vdec_s)
    {
        vdec_s->size = ctx->sps_size + sizeof(start_code);
        memcpy(vdec_s->data, start_code, sizeof(start_code));
        memcpy(vdec_s->data + sizeof(start_code), ctx->sps, ctx->sps_size);
        Amy_timestamp(vdec_s->timestamp);
        vdec_s->keyframe = H264_FRAME_TYPE_SPS;
        tuya_video_main_ringbuffer_put(vdec_s->data, vdec_s->size, vdec_s->timestamp, vdec_s->keyframe);

        if (ctx->avi_dev)
        {
            stream_data *rec_s = avi_buffer_get(ctx->avi_dev, (int)vdec_s->size);
            if (!rec_s)
            {
                AMY_LOG_ERROR("avi buffer get failed\n");
            }
            else
            {
                memcpy(rec_s->data, vdec_s->data, vdec_s->size);
                rec_s->keyframe = vdec_s->keyframe;
                rec_s->timestamp = vdec_s->timestamp;
                if (avi_video_send(ctx->avi_dev, rec_s))
                {
                    avi_buffer_put(ctx->avi_dev, rec_s);
                    AMY_LOG_ERROR("send avi video failed\n");
                }
            }
        }
        if (video_decodec_send_stream(ctx->video_dev, vdec_s))
        {
            video_decodec_buf_put(ctx->video_dev, vdec_s);
        }
    }
}
void DummySink::sendPPSToDev(rtsp_client_context *ctx)
{
    unsigned char start_code[] = {0, 0, 0, 1};
    stream_data *vdec_s = video_decodec_buf_get(ctx->video_dev, ctx->pps_size + sizeof(start_code));
    if (vdec_s)
    {
        vdec_s->size = ctx->pps_size + sizeof(start_code);
        memcpy(vdec_s->data, start_code, sizeof(start_code));
        memcpy(vdec_s->data + sizeof(start_code), ctx->pps, ctx->pps_size);
        Amy_timestamp(vdec_s->timestamp);
        vdec_s->keyframe = H264_FRAME_TYPE_PPS;

        tuya_video_main_ringbuffer_put(vdec_s->data, vdec_s->size, vdec_s->timestamp, vdec_s->keyframe);

        if (ctx->avi_dev)
        {
            stream_data *rec_s = avi_buffer_get(ctx->avi_dev, (int)vdec_s->size);
            if (!rec_s)
            {
                AMY_LOG_ERROR("avi buffer get failed\n");
            }
            else
            {
                memcpy(rec_s->data, vdec_s->data, vdec_s->size);
                rec_s->keyframe = vdec_s->keyframe;
                rec_s->timestamp = vdec_s->timestamp;
                if (avi_video_send(ctx->avi_dev, rec_s))
                {
                    avi_buffer_put(ctx->avi_dev, rec_s);
                    AMY_LOG_ERROR("send avi video failed\n");
                }
            }
        }
        if (video_decodec_send_stream(ctx->video_dev, vdec_s))
        {
            video_decodec_buf_put(ctx->video_dev, vdec_s);
        }
    }
}
void DummySink::sendIDRToDev(rtsp_client_context *ctx, unsigned int frameSize)
{
    unsigned char start_code[] = {0, 0, 0, 1};
    stream_data *vdec_s = video_decodec_buf_get(ctx->video_dev, frameSize + sizeof(start_code));
    if (vdec_s)
    {
        vdec_s->size = frameSize + sizeof(start_code);
        memcpy(vdec_s->data, start_code, sizeof(start_code));
        memcpy(vdec_s->data + sizeof(start_code), fReceiveBuffer, frameSize);
        Amy_timestamp(vdec_s->timestamp);
        vdec_s->keyframe = H264_FRAME_TYPE_IDR;

        tuya_video_main_ringbuffer_put(vdec_s->data, vdec_s->size, vdec_s->timestamp, vdec_s->keyframe);

        if (ctx->avi_dev)
        {
            stream_data *rec_s = avi_buffer_get(ctx->avi_dev, (int)vdec_s->size);
            if (!rec_s)
            {
                AMY_LOG_ERROR("avi buffer get failed\n");
            }
            else
            {
                memcpy(rec_s->data, vdec_s->data, vdec_s->size);
                rec_s->keyframe = vdec_s->keyframe;
                rec_s->timestamp = vdec_s->timestamp;
                if (avi_video_send(ctx->avi_dev, rec_s))
                {
                    avi_buffer_put(ctx->avi_dev, rec_s);
                    AMY_LOG_ERROR("send avi video failed\n");
                }
            }
        }
        if (video_decodec_send_stream(ctx->video_dev, vdec_s))
        {
            video_decodec_buf_put(ctx->video_dev, vdec_s);
        }
    }
}

void DummySink::sendPPPToDev(rtsp_client_context *ctx, unsigned int frameSize)
{
    unsigned char start_code[] = {0, 0, 0, 1};
    stream_data *vdec_s = video_decodec_buf_get(ctx->video_dev, frameSize + sizeof(start_code));
    if (vdec_s)
    {
        vdec_s->size = frameSize + sizeof(start_code);
        memcpy(vdec_s->data, start_code, sizeof(start_code));
        memcpy(vdec_s->data + sizeof(start_code), fReceiveBuffer, frameSize);
        Amy_timestamp(vdec_s->timestamp);
        vdec_s->keyframe = H264_FRAME_TYPE_PPP;
        tuya_video_main_ringbuffer_put(vdec_s->data, vdec_s->size, vdec_s->timestamp, vdec_s->keyframe);
        if (ctx->avi_dev)
        {
            stream_data *rec_s = avi_buffer_get(ctx->avi_dev, (int)vdec_s->size);
            if (!rec_s)
            {
                AMY_LOG_ERROR("avi buffer get failed\n");
            }
            else
            {
                memcpy(rec_s->data, vdec_s->data, vdec_s->size);
                rec_s->keyframe = vdec_s->keyframe;
                rec_s->timestamp = vdec_s->timestamp;
                if (avi_video_send(ctx->avi_dev, rec_s))
                {
                    avi_buffer_put(ctx->avi_dev, rec_s);
                    AMY_LOG_ERROR("send avi video failed\n");
                }
            }
        }
        if (video_decodec_send_stream(ctx->video_dev, vdec_s))
        {
            video_decodec_buf_put(ctx->video_dev, vdec_s);
        }
    }
}

void DummySink::sendAudioToAvi(rtsp_client_context *rtsp_context)
{
    stream_data *avi_ast = NULL;
    avi_ast = avi_buffer_get(rtsp_context->avi_dev, rtsp_context->au_rawsize);
    if (!avi_ast)
    {
        AMY_LOG_ERROR("avi audio  buffer  get failed\n");
        return;
    }
    Amy_timestamp(avi_ast->timestamp);
    // 录制对端声音（RTSP 对端，如门口机访客），不混入本地麦克风
    memcpy(avi_ast->data, rtsp_context->au_rawdata, rtsp_context->au_rawsize);
    if (avi_audio_send(rtsp_context->avi_dev, avi_ast))
    {
        avi_buffer_put(rtsp_context->avi_dev, avi_ast);
    }
}
void DummySink::sendAudioToDev(rtsp_client_context *rtsp_context, unsigned int frameSize)
{
    (void)frameSize;
    unsigned int remain = 0, used = 0, total = 0;
    if ((rtsp_context->ao_dev != NULL) &&
        (audio_output_query(rtsp_context->ao_dev, &remain, &used, &total) == 0))
    {
        if (fAudioReceive.load(std::memory_order_acquire) == True)
        {
            if (rtsp_context->au_rawsize > 0 && remain > rtsp_context->au_rawsize)
            {
                if (!rtsp_context->ao_cleared)
                {
                    audio_output_clear(rtsp_context->ao_dev);
                    rtsp_context->ao_cleared = 1;
                }
                if (audio_output_write(rtsp_context->ao_dev, rtsp_context->au_rawdata, rtsp_context->au_rawsize))
                {
                    audio_output_flush(rtsp_context->ao_dev);
                }
            }
            else
            {
                audio_output_flush(rtsp_context->ao_dev);
                AMY_LOG_WARNING("audio output device flush\n");
            }
        }
    }

    if ((rtsp_context->avi_dev) && (avi_has_audio(rtsp_context->avi_dev)))
    {
        sendAudioToAvi(rtsp_context);
    }
}
// If you don't want to see debugging output for each received frame, then comment out the following line:
// #define DEBUG_PRINT_EACH_RECEIVED_FRAME 1
void DummySink::afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes, struct timeval presentationTime, unsigned /*durationInMicroseconds*/)
{

    // We've just received a frame of data.  (Optionally) print out information about it:
#ifdef DEBUG_PRINT_EACH_RECEIVED_FRAME

    if (fStreamId != NULL)
        envir() << "Stream \"" << fStreamId << "\"; ";

    envir() << fSubsession.mediumName() << "/" << fSubsession.codecName() << ":\tReceived " << frameSize << " bytes";

    if (numTruncatedBytes > 0)
        envir() << " (with " << numTruncatedBytes << " bytes truncated)";

    char uSecsStr[6 + 1]; // used to output the 'microseconds' part of the presentation time

    sprintf(uSecsStr, "%06u", (unsigned)presentationTime.tv_usec);
    envir() << ".\tPresentation time: " << (int)presentationTime.tv_sec << "." << uSecsStr;

    if (fSubsession.rtpSource() != NULL && !fSubsession.rtpSource()->hasBeenSynchronizedUsingRTCP())
    {
        envir() << "!"; // mark the debugging output to indicate that this presentation time is not RTCP-synchronized
    }

#ifdef DEBUG_PRINT_NPT
    envir() << "\tNPT: " << fSubsession.getNormalPlayTime(presentationTime);
#endif

    envir() << "\n";
#endif
    rtsp_client_context *ctx = (rtsp_client_context *)userData;
    if (numTruncatedBytes > 0)
    {
        if (ctx)
        {
            Amy_mutex_lock(&ctx->mutex);
            ctx->truncated_frames++;
            ctx->wait_next_idr = 1;
            unsigned int truncated = ctx->truncated_frames;
            Amy_mutex_unlock(&ctx->mutex);
            AMY_LOG_WARNING("RTP frame truncated(%u bytes), drop frame and wait IDR. cnt=%u\n",
                            numTruncatedBytes, truncated);
        }
        continuePlaying();
        return;
    }

    if (strncmp(fSubsession.codecName(), "H264", 4) == 0)
    {
        rtsp_client_context *rtsp_context = static_cast<rtsp_client_context *>(userData);
        Amy_mutex_lock(&rtsp_context->mutex);
        int frame_type = h264_frame_type(fReceiveBuffer[0]);
        if (rtsp_context->wait_next_idr && frame_type != H264_FRAME_TYPE_IDR)
        {
            Amy_mutex_unlock(&rtsp_context->mutex);
            continuePlaying();
            return;
        }
        if (frame_type == H264_FRAME_TYPE_SPS)
        {
            updateSPS(rtsp_context, frameSize);
        }
        else if (frame_type == H264_FRAME_TYPE_PPS)
        {
            updatePPS(rtsp_context, frameSize);
        }
        else if (frame_type == H264_FRAME_TYPE_IDR)
        {
            if (rtsp_context->wait_next_idr)
            {
                rtsp_context->wait_next_idr = 0;
                AMY_LOG_INFO("Got IDR after packet loss, resume video decode\n");
            }
            updateDispayDevice(rtsp_context);

            if ((rtsp_context->request_record) && (!rtsp_context->avi_dev))
            {
                rtsp_context->request_record = 0;
                char path[128] = {0};
                sprintf(path, TFCARD_MEDIA_PATH "%ld", time(NULL));
                rtsp_context->avi_dev = avi_record_open(rtsp_context->width, rtsp_context->height, rtsp_context->samplerate, path, "H264", rtsp_context->recvideo_cb);
            }
            if (rtsp_context->video_dev && rtsp_context->pps && rtsp_context->sps)
            {
                sendSPSToDev(rtsp_context);
                sendPPSToDev(rtsp_context);
                // IDR 直接从接收缓冲送解码（同PPP），不复制进内存池；仅当关键帧成功送入后才放行P帧
                sendIDRToDev(rtsp_context, frameSize);
                rtsp_context->decode_ready = 1;
            }
        }
        else if (rtsp_context->video_dev && rtsp_context->pps && rtsp_context->sps && rtsp_context->decode_ready)
        {
            sendPPPToDev(rtsp_context, frameSize);
        }

        /* 抓拍不依赖视频模式显示管线，直接在收流线程触发，非视频模式（静默抓拍/录像）也能工作 */
        if (rtsp_context->request_snapshort && rtsp_context->video_dev)
        {
            struct ak_vdec_frame snap_frame = {0};
            if (video_decodec_get_frame(rtsp_context->video_dev, &snap_frame, NULL) == 0)
            {
                if (Amy_snapshot_start((char *)snap_frame.frame_obj.data.data,
                                   snap_frame.frame_obj.data.pitch_width,
                                   snap_frame.frame_obj.data.pitch_height,
                                   snap_frame.width, snap_frame.height,
                                   rtsp_context->snapshort_cb) == 0)
                    rtsp_context->request_snapshort = 0;
            }
        }
        Amy_mutex_unlock(&rtsp_context->mutex);
    }
    else if ((strncmp(fSubsession.mediumName(), "audio", 5) == 0) &&
             ((strncmp(fSubsession.codecName(), "PCMU", 4) == 0) ||
              (strncmp(fSubsession.codecName(), "PCMA", 4) == 0) ||
              (strncmp(fSubsession.codecName(), "L16", 3) == 0) ||
              (strncmp(fSubsession.codecName(), "PCM", 3) == 0)))
    {
        rtsp_client_context *rtsp_context = static_cast<rtsp_client_context *>(userData);

        Amy_mutex_lock(&rtsp_context->mutex);
        // 从 subsession 获取实际采样率（SDP 协商结果）
        int sample_rate = (int)fSubsession.rtpTimestampFrequency();
        if (sample_rate <= 0)
            sample_rate = 16000;                                              // 降级保护
        if (rtsp_context->samplerate != sample_rate || !rtsp_context->ao_dev) // 可能异步线程关闭了AO。需要重复检查AO的情况
        {
            updateAudioDevice(rtsp_context, sample_rate);
        }

        bool is_g711 = false;
        unsigned char g711_type = TUYA_G711_MU_LAW;
        if (strncmp(fSubsession.codecName(), "PCMU", 4) == 0)
        {
            is_g711 = true;
            g711_type = TUYA_G711_MU_LAW;
        }
        else if (strncmp(fSubsession.codecName(), "PCMA", 4) == 0)
        {
            is_g711 = true;
            g711_type = TUYA_G711_A_LAW;
        }

        unsigned int pcm_size = is_g711 ? (frameSize * 2) : frameSize;
        if ((!rtsp_context->au_rawdata) || (rtsp_context->au_rawsize != pcm_size))
        {
            if (rtsp_context->au_rawdata)
            {
                Amy_mempool_free(rtsp_context->mp, rtsp_context->au_rawdata);
            }
            rtsp_context->au_rawsize = pcm_size;
            rtsp_context->au_rawdata = (char *)Amy_mempool_alloc(rtsp_context->mp, rtsp_context->au_rawsize);
        }

        unsigned long long timestamp;
        Amy_timestamp(timestamp);
        tuya_audio_main_ringbuffer_put(fReceiveBuffer, frameSize, timestamp);

        if (rtsp_context->au_rawdata != NULL && rtsp_context->au_rawsize > 0)
        {
            if (is_g711)
            {
                unsigned int out_size = rtsp_context->au_rawsize;
                tuya_g711_decode(g711_type, (unsigned short *)fReceiveBuffer, frameSize,
                                 (unsigned char *)rtsp_context->au_rawdata, &out_size);
                rtsp_context->au_rawsize = out_size;
            }
            else
            {
                memcpy(rtsp_context->au_rawdata, fReceiveBuffer, frameSize);
            }
            sendAudioToDev(rtsp_context, frameSize);
        }
        Amy_mutex_unlock(&rtsp_context->mutex);
    }

    // Then continue, to request the next frame of data:
    continuePlaying();
}

Boolean DummySink::continuePlaying()
{
    if (fSource == NULL)
        return False; // sanity check (should not happen)

    // Request the next frame of data from our input source.  "afterGettingFrame()" will get called later, when it arrives:
    fSource->getNextFrame(fReceiveBuffer, fReceiveBufferSize, afterGettingFrame, this, onSourceClosure, this);
    return True;
}
