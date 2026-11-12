#include "sat_aec_wrapper.h"
#include <stdio.h>
#include <pthread.h>

#ifdef ENABLE_WEBRTC_AEC

#include <cstring>
#include <memory>

#include "api/echo_canceller3_factory.h"
#include "api/echo_canceller3_config.h"
#include "audio_processing/include/audio_processing.h"
#include "audio_processing/audio_buffer.h"
#include "audio_processing/audio_frame.h"
#include "audio_processing/high_pass_filter.h"

using namespace webrtc;

static pthread_mutex_t g_aec_mutex = PTHREAD_MUTEX_INITIALIZER;

struct sat_aec_context
{
    int sample_rate;
    int chunk_samples;
    std::unique_ptr<EchoControl> echo_control;
    std::unique_ptr<HighPassFilter> hp_filter;
    StreamConfig config;
    std::unique_ptr<AudioBuffer> render_buffer;
    std::unique_ptr<AudioBuffer> capture_buffer;
    AudioFrame render_frame;
    AudioFrame capture_frame;
};

extern "C"
{

    void *sat_aec_create(int sample_rate)
    {
        EchoCanceller3Config aec_config;
        aec_config.filter.export_linear_aec_output = false;
        aec_config.delay.use_external_delay_estimator = true;

        auto *ctx = new (std::nothrow) sat_aec_context();
        if (!ctx)
            return nullptr;

        ctx->sample_rate = sample_rate;
        ctx->chunk_samples = sample_rate / 100;

        ctx->echo_control = EchoCanceller3Factory(aec_config).Create(sample_rate, 1, 1);
        if (!ctx->echo_control)
        {
            delete ctx;
            return nullptr;
        }

        ctx->hp_filter.reset(new HighPassFilter(sample_rate, 1));
        ctx->config = StreamConfig(sample_rate, 1, false);

        ctx->render_buffer.reset(new AudioBuffer(
            ctx->config.sample_rate_hz(), ctx->config.num_channels(),
            ctx->config.sample_rate_hz(), ctx->config.num_channels(),
            ctx->config.sample_rate_hz(), ctx->config.num_channels()));

        ctx->capture_buffer.reset(new AudioBuffer(
            ctx->config.sample_rate_hz(), ctx->config.num_channels(),
            ctx->config.sample_rate_hz(), ctx->config.num_channels(),
            ctx->config.sample_rate_hz(), ctx->config.num_channels()));

        return ctx;
    }

    void sat_aec_destroy(void *aec)
    {
        if (!aec)
            return;
        pthread_mutex_lock(&g_aec_mutex);
        auto *ctx = static_cast<sat_aec_context *>(aec);
        delete ctx;
        pthread_mutex_unlock(&g_aec_mutex);
    }

    void sat_aec_render(void *aec, const int16_t *pcm, int samples)
    {
        if (!aec || !pcm || samples <= 0)
            return;
        pthread_mutex_lock(&g_aec_mutex);
        auto *ctx = static_cast<sat_aec_context *>(aec);
        const int chunk = ctx->chunk_samples;

        for (int offset = 0; offset + chunk <= samples; offset += chunk)
        {
            ctx->render_frame.UpdateFrame(0, pcm + offset, chunk,
                                          ctx->sample_rate,
                                          AudioFrame::kNormalSpeech,
                                          AudioFrame::kVadActive, 1);
            ctx->render_buffer->CopyFrom(&ctx->render_frame);
            ctx->render_buffer->SplitIntoFrequencyBands();
            ctx->echo_control->AnalyzeRender(ctx->render_buffer.get());
            ctx->render_buffer->MergeFrequencyBands();
        }
        pthread_mutex_unlock(&g_aec_mutex);
    }

    void sat_aec_capture(void *aec, int16_t *pcm, int samples)
    {
        if (!aec || !pcm || samples <= 0)
            return;
        pthread_mutex_lock(&g_aec_mutex);
        auto *ctx = static_cast<sat_aec_context *>(aec);
        const int chunk = ctx->chunk_samples;

        for (int offset = 0; offset + chunk <= samples; offset += chunk)
        {
            ctx->capture_frame.UpdateFrame(0, pcm + offset, chunk,
                                           ctx->sample_rate,
                                           AudioFrame::kNormalSpeech,
                                           AudioFrame::kVadActive, 1);
            ctx->capture_buffer->CopyFrom(&ctx->capture_frame);
            ctx->echo_control->SetAudioBufferDelay(0);
            ctx->echo_control->AnalyzeCapture(ctx->capture_buffer.get());
            ctx->capture_buffer->SplitIntoFrequencyBands();
            ctx->hp_filter->Process(ctx->capture_buffer.get(), true);
            ctx->echo_control->ProcessCapture(ctx->capture_buffer.get(), false);
            ctx->capture_buffer->MergeFrequencyBands();

            ctx->capture_buffer->CopyTo(&ctx->capture_frame);
            memcpy(pcm + offset, ctx->capture_frame.data(),
                   chunk * sizeof(int16_t));
        }

        pthread_mutex_unlock(&g_aec_mutex);
    }

    static void *g_global_aec = NULL;

    void *sat_aec_global_handle(void)
    {
        pthread_mutex_lock(&g_aec_mutex);
        void *handle = g_global_aec;
        pthread_mutex_unlock(&g_aec_mutex);
        return handle;
    }

    void sat_aec_set_global_handle(void *aec)
    {
        pthread_mutex_lock(&g_aec_mutex);
        g_global_aec = aec;
        pthread_mutex_unlock(&g_aec_mutex);
    }

} // extern "C"

#else /* !ENABLE_AEC — 透传实现 */

extern "C"
{

    void *sat_aec_create(int sample_rate)
    {
        (void)sample_rate;
        return (void *)0x1;
    }

    void sat_aec_destroy(void *aec)
    {
        (void)aec;
    }

    void sat_aec_render(void *aec, const int16_t *pcm, int samples)
    {
        (void)aec;
        (void)pcm;
        (void)samples;
    }

    void sat_aec_capture(void *aec, int16_t *pcm, int samples)
    {
        (void)aec;
        (void)pcm;
        (void)samples;
    }

    void *sat_aec_global_handle(void)
    {
        return NULL;
    }

    void sat_aec_set_global_handle(void *aec)
    {
        (void)aec;
    }

} // extern "C"

#endif // ENABLE_AEC
