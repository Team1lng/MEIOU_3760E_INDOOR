#ifndef SIGNAL_LED_ANMATION_H
#define SIGNAL_LED_ANMATION_H
#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned long int ledHandle_t;

typedef enum HW_LED_TYPE {
	HW_USE_POINT_LED,
	HW_USE_LINE_LED,
	HW_USE_RING_LED,
} LED_TYPE;

ledHandle_t LedAnimationInit(const char *conf);
int LedAnimationUpdate(ledHandle_t* handle, const char* animation);
int LedAnimationRelease(ledHandle_t* handle);

#ifdef __cplusplus
}
#endif
#endif
