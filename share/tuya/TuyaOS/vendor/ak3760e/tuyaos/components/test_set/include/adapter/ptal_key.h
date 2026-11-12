#ifndef _PTAL_KEY_H_
#define _PTAL_KEY_H_

#ifdef __cplusplus__
extern "C" {
#endif

#define MAX_KEY_NUM 8 //max key numbers in main cpu
#define MAX_SUB_MCU_KEY_NUM (8)  // max key numbers in sub cpu

extern int ptal_key_get(int n,int *flag);

#ifdef __cplusplus__
}
#endif

#endif
