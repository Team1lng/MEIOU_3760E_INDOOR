#include "speex_aec_wapper.h"

#ifdef ENABLE_SPEEX_AEC
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <speex/speex_echo.h>
static pthread_mutex_t g_aec_mutex = PTHREAD_MUTEX_INITIALIZER;

struct sat_speex_aec_context
{
  int sample_rate;
  int frame_size;
  SpeexEchoState *state;
};

void *speex_aec_create(int sample_rate)
{
  const int frame_size = sample_rate / 100;           // 10ms
  const int filter_length = sample_rate * 250 / 1000; // 250ms tail

  struct sat_speex_aec_context *ctx = (struct sat_speex_aec_context *)calloc(1, sizeof(*ctx));
  if (!ctx)
    return NULL;

  ctx->sample_rate = sample_rate;
  ctx->frame_size = frame_size;

  ctx->state = sat_speex_echo_state_init(frame_size, filter_length);
  if (!ctx->state)
  {
    free(ctx);
    return NULL;
  }

  sat_speex_echo_ctl(ctx->state, SPEEX_ECHO_SET_SAMPLING_RATE, &sample_rate);

  return ctx;
}

void speex_aec_destroy(void *aec)
{
  if (!aec)
    return;
  pthread_mutex_lock(&g_aec_mutex);
  struct sat_speex_aec_context *ctx = (struct sat_speex_aec_context *)aec;
  if (ctx->state)
    sat_speex_echo_state_destroy(ctx->state);
  free(ctx);
  pthread_mutex_unlock(&g_aec_mutex);
}

void speex_aec_render(void *aec, const int16_t *pcm, int samples)
{
  if (!aec || !pcm || samples <= 0)
    return;
  pthread_mutex_lock(&g_aec_mutex);
  struct sat_speex_aec_context *ctx = (struct sat_speex_aec_context *)aec;
  const int frame_size = ctx->frame_size;

  for (int offset = 0; offset + frame_size <= samples; offset += frame_size)
  {
    sat_speex_echo_playback(ctx->state, pcm + offset);
  }
  pthread_mutex_unlock(&g_aec_mutex);
}

void speex_aec_capture(void *aec, int16_t *pcm, int samples)
{
  if (!aec || !pcm || samples <= 0)
    return;
  pthread_mutex_lock(&g_aec_mutex);
  struct sat_speex_aec_context *ctx = (struct sat_speex_aec_context *)aec;
  const int frame_size = ctx->frame_size;

  for (int offset = 0; offset + frame_size <= samples; offset += frame_size)
  {
    sat_speex_echo_capture(ctx->state, pcm + offset, pcm + offset);
  }
  pthread_mutex_unlock(&g_aec_mutex);
}

static void *g_global_aec = NULL;

void *speex_aec_global_handle(void)
{
  return g_global_aec;
}

void speex_aec_set_global_handle(void *aec)
{
  g_global_aec = aec;
}

#else /* !ENABLE_SPEEX_AEC — passthrough */
#include <stdio.h>
void *speex_aec_create(int sample_rate)
{
  (void)sample_rate;
  return (void *)0x1;
}

void speex_aec_destroy(void *aec)
{
  (void)aec;
}

void speex_aec_render(void *aec, const int16_t *pcm, int samples)
{
  (void)aec;
  (void)pcm;
  (void)samples;
}

void speex_aec_capture(void *aec, int16_t *pcm, int samples)
{
  (void)aec;
  (void)pcm;
  (void)samples;
}

void *speex_aec_global_handle(void)
{
  return NULL;
}

void speex_aec_set_global_handle(void *aec)
{
  (void)aec;
}

#endif // ENABLE_SPEEX_AEC
