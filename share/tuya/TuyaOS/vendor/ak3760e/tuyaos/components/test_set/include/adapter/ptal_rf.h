#ifndef __PTAL_RF_H__
#define __PTAL_RF_H__
#ifdef __cplusplus__
extern "C" {
#endif

extern int ptal_set_wifi_rf_flag(int flag);
extern int ptal_get_wifi_rf_flag(int *flag);
extern int ptal_set_bt_rf_flag(int flag);
extern int ptal_get_bt_rf_flag(int *flag);

#ifdef __cplusplus__
}
#endif
#endif
