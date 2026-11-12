#ifndef __PTAL_MAC_H__
#define __PTAL_MAC_H__

#ifdef __cplusplus__
extern "C" {
#endif

extern int ptal_set_smac(int cpu,char *mac);
extern int ptal_set_mmac(char *mac);
extern int ptal_get_mmac(char *mac);
extern int ptal_get_smac(int cpu,char *mac);

#ifdef __cplusplus__
}
#endif

#endif

