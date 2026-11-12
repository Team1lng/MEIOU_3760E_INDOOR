#ifndef __PTAL_AUDIO_H__
#define __PTAL_AUDIO_H__

#ifdef __cplusplus__
extern "C" {
#endif

extern int ptal_test_audio_test(void);
extern int ptal_swept_sine_start(int num);
extern int ptal_swept_over(void);
extern int ptal_read_swept_sine_result(void);

#ifdef __cplusplus__
}
#endif

#endif
