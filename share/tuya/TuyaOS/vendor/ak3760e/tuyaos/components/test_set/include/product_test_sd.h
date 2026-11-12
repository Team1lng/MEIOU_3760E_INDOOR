#ifndef _PRODUCT_TEST_SD_H_
#define _PRODUCT_TEST_SD_H_

extern int product_test_sd_access(uint32_t size,int cnts);
extern int product_test_sd_speed(uint32_t *read_spead,uint32_t *write_speed);
extern int product_test_sd_get_volume(uint32_t *volume); 
extern int product_test_sd_is_mounted(void); 
extern int product_test_sd_is_insert(void); 
extern int product_test_sd_format( char *mmcblock);
#endif
