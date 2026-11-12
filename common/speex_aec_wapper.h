#ifndef SPEEX_AEC_WRAPPER_H
#define SPEEX_AEC_WRAPPER_H
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

  void *speex_aec_create(int sample_rate);
  void speex_aec_destroy(void *aec);
  void *speex_aec_global_handle(void);
  void speex_aec_set_global_handle(void *aec);
  void speex_aec_render(void *aec, const int16_t *pcm, int samples);
  void speex_aec_capture(void *aec, int16_t *pcm, int samples);

#ifdef __cplusplus
}
#endif

#endif // SPEEX_AEC_WRAPPER_H
