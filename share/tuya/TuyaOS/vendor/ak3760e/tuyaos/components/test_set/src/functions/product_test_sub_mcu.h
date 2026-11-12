#ifndef __PRODUCT_TEST_SUB_MCU_H__
#define __PRODUCT_TEST_SUB_MCU_H__


extern int product_test_sub_mcu_get_version(unsigned int *pver);
extern int product_test_sub_mcu_get_wifi_rf_flag(int *flag);
extern int product_test_sub_mcu_connect_ap(char *ssid,char *psk);
extern int product_test_sub_mcu_write_mac(char *mac);
extern int product_test_sub_mcu_get_pir_events(int* cnts);
extern int product_test_sub_mcu_pir_test(int counts);
extern int product_test_sub_mcu_get_key(int n,int *flag);
extern int product_test_sub_mcu_get_sleep_flag(int *flag);
extern int product_test_sub_mcu_set_deep_sleep(void);
extern int product_test_sub_mcu_get_wifi_rssi(int *rssi);
extern int product_test_sub_mcu_set_mac(char *mac);

#endif

