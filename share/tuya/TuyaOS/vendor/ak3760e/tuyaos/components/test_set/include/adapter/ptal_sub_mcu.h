#ifndef _PTAL_SUB_MCU_H_
#define _PTAL_SUB_MCU_H_

#ifdef __cplusplus__
extern "C" {
#endif

extern int ptal_sub_mcu_init(void);
extern int ptal_sub_mcu_get_version(unsigned int *pver); 
extern int ptal_sub_mcu_get_wifi_rf_flag(int *flag);
extern int ptal_sub_mcu_connect_ap(char * ssid, char * passwd);
extern int ptal_sub_mcu_write_mac(char *mac);
extern int ptal_sub_mcu_get_pir_event(int *cnts);
extern int ptal_sub_mcu_get_key(int n,int *flag); 
extern int ptal_sub_mcu_get_sleep_flag(int *flag);
extern int ptal_sub_mcu_set_deep_sleep(void);
extern int ptal_sub_mcu_get_wifi_rssi(int *rssi);

extern int ptal_sub_mcu_get_wake_in(int *value);
extern int ptal_sub_mcu_set_wake_out(int value); 
extern int ptal_sub_mcu_get_wake_panel(int *value);
extern int ptal_sub_mcu_set_mac(char *mac);

#ifdef __cplusplus__
}
#endif
#endif
