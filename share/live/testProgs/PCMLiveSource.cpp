#include "PCMLiveSource.hh"
#define RTSP_AUDIO_FRAME_SIZE 320

extern "C"
{
#include "audio_output.h"
#include "Amy_sdk.h"
#include "audio_input.h"
#include "tuya/tuya_audio_recv.h"
#include "tuya/TuyaOS/vendor/ak3760e/tuyaos/components/base_hal/test/video/tuya_g711_utils.h"
}

// ====================== 全局安全锁：唯一必须加的东西 ======================
static Amy_mutex_t g_pcm_lock = PTHREAD_MUTEX_INITIALIZER;

// 静态成员变量
Boolean PCMLiveSource::fAudioSend = False;
Boolean PCMLiveSource::fTuyaAudio = False;

// ====================== 静态开关：加锁保护 ======================
int PCMLiveSource::openServerSpeaker()
{
    Amy_mutex_lock(&g_pcm_lock);
    fAudioSend = True;
    Amy_mutex_unlock(&g_pcm_lock);
    return 0;
}

int PCMLiveSource::closeServerSpeaker()
{
    Amy_mutex_lock(&g_pcm_lock);
    fAudioSend = False;
    Amy_mutex_unlock(&g_pcm_lock);
    return 0;
}

int PCMLiveSource::enableTuyaAudio()
{
    Amy_mutex_lock(&g_pcm_lock);
    fTuyaAudio = True;
    Amy_mutex_unlock(&g_pcm_lock);
    return 0;
}

int PCMLiveSource::disableTuyaAudio()
{
    Amy_mutex_lock(&g_pcm_lock);
    fTuyaAudio = False;
    Amy_mutex_unlock(&g_pcm_lock);
    return 0;
}

// C接口
extern "C" int open_server_speaker(void) { return PCMLiveSource::openServerSpeaker(); }
extern "C" int close_server_speaker(void) { return PCMLiveSource::closeServerSpeaker(); }
extern "C" int tuya_audio_enable(void) { return PCMLiveSource::enableTuyaAudio(); }
extern "C" int tuya_audio_disable(void) { return PCMLiveSource::disableTuyaAudio(); }

// ====================== 构造 ======================
PCMLiveSource::PCMLiveSource(UsageEnvironment &env)
    : FramedSource(env)
{
    ai_ctx = NULL;
}

PCMLiveSource *PCMLiveSource::createNew(UsageEnvironment &env)
{
    PCMLiveSource *newSource = new PCMLiveSource(env);

    // 加锁：防止与析构/doGetNextFrame并发访问
    Amy_mutex_lock(&g_pcm_lock);
    if (!fTuyaAudio && newSource->ai_ctx == NULL)
    {
        int chn, rate;
        newSource->ai_ctx = audio_input_param_get(&chn, &rate, NULL);
        if ((!newSource->ai_ctx) || (chn != 1) || (rate != 16000))
        {
            if (newSource->ai_ctx)
            {
                audio_input_close(newSource->ai_ctx);
            }
            newSource->ai_ctx = audio_input_open(1, 16000, 100);
        }
    }
    else
    {
        tuya_recv_audio_ringbuffer_clear();
    }
    Amy_mutex_unlock(&g_pcm_lock);

    return newSource;
}

// ====================== 析构（安全加锁） ======================
PCMLiveSource::~PCMLiveSource()
{
    // 加锁：保证和doGetNextFrame互斥，不会一边删一边用
    Amy_mutex_lock(&g_pcm_lock);
    if (ai_ctx != NULL)
    {
        audio_input_close(ai_ctx);
        ai_ctx = NULL;
    }
    Amy_mutex_unlock(&g_pcm_lock);
}

unsigned PCMLiveSource::maxFrameSize() const
{
    return 1024 * 1024;
}

// ====================== doGetNextFrame（安全加锁） ======================
void PCMLiveSource::doGetNextFrame()
{
    fFrameSize = 0;

    // --------------------- 安全读取全局开关 ---------------------
    Boolean audioSend, tuyaAudio;
    Amy_mutex_lock(&g_pcm_lock);
    audioSend = fAudioSend;
    tuyaAudio = fTuyaAudio;
    Amy_mutex_unlock(&g_pcm_lock);

    // --------------------- 涂鸦音频 ---------------------
    if (tuyaAudio)
    {
        stream_data *ast = tuya_recv_audio_buffer_get(RTSP_AUDIO_FRAME_SIZE);
        if (ast != NULL)
        {
            int ret = tuya_recv_audio_frame_get(ast);
            if (ret > 0 && audioSend)
            {
                // 涂鸦下行音频实际下发为 PCM(16kHz/16bit)，需编码为 G.711 u-law 再经 RTSP PCMU 送给门口机
                unsigned char dst_data[4096] = {0};
                unsigned int dst_len = 0;
                tuya_g711_encode(TUYA_G711_MU_LAW, (unsigned short *)ast->data, ret, dst_data, &dst_len);
                if (dst_len > 0 && dst_len <= fMaxSize)
                {
                    fFrameSize = dst_len;
                    memcpy(fTo, dst_data, fFrameSize);
                    gettimeofday(&fPresentationTime, NULL);
                }
            }
            tuya_recv_audio_buffer_release(ast);
        }

        nextTask() = envir().taskScheduler().scheduleDelayedTask(0, (TaskFunc *)FramedSource::afterGetting, this);
        return;
    }

    // --------------------- 安全访问 ai_ctx ---------------------
    Amy_mutex_lock(&g_pcm_lock);
    audio_input_context *ctx = this->ai_ctx; // 快照，防止被置空
    Amy_mutex_unlock(&g_pcm_lock);

    if (ctx == NULL)
    {
        nextTask() = envir().taskScheduler().scheduleDelayedTask(0, (TaskFunc *)FramedSource::afterGetting, this);
        return;
    }

    // --------------------- 正常业务逻辑 ---------------------
    stream_data *ai_ast = audio_input_buffer_get(ctx, RTSP_AUDIO_FRAME_SIZE);
    if (!ai_ast)
    {
        nextTask() = envir().taskScheduler().scheduleDelayedTask(0, (TaskFunc *)FramedSource::afterGetting, this);
        return;
    }

    int ret = audio_input_frame_get_live(ctx, ai_ast);
    if (ret != ai_ast->size || !audioSend)
    {
        audio_input_frame_release(ctx, ai_ast);
        nextTask() = envir().taskScheduler().scheduleDelayedTask(0, (TaskFunc *)FramedSource::afterGetting, this);
        return;
    }
    unsigned char dst_data[4096] = {0};
    unsigned int dst_len = 0;
    tuya_g711_encode(TUYA_G711_MU_LAW, (unsigned short *)ai_ast->data, ai_ast->size, dst_data, &dst_len);

    if (dst_len > 0 && dst_len <= fMaxSize)
    {
        fFrameSize = dst_len;
        memcpy(fTo, dst_data, fFrameSize);
        gettimeofday(&fPresentationTime, NULL);
    }

    audio_input_frame_release(ctx, ai_ast);

    nextTask() = envir().taskScheduler().scheduleDelayedTask(0, (TaskFunc *)FramedSource::afterGetting, this);
}