/* 
 *  @file product_test_etherent.h
 *  @brief product_test_adc module is supply etherent test interface
 *  @version 0.1
 *  @date 2021-05-07
 *
 *  @copyright Copyright (c) tuya.inc 2021
 *  
 */
#ifndef __PRODUCT_TEST_ETHERENT_H__
#define __PRODUCT_TEST_ETHERENT_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct etherent_iperf_s {
    char server[32]; /* iperf server ip */
    uint16_t port;   /* iperf port */
    uint32_t time;   /* iperf test time */
    double avr_speed;/* iperf average speed */
    double min_speed;/* iperf min speed */
    double max_speed;/* iperf max speed */
}ETHERENT_IPERF_T;


/*@brief get ethernet ip addr 
 *@param(in) interface:etherent interface name
 *@param(out)ip:ip address 
 *@return 0:succeed,-1:fail
 */
extern int product_test_etherent_get_ip(char * interface,uint32_t *ip);

/*@brief dynamic set ethernet ip addr 
 *@param(in) interface:etherent interface name
 *@return 0:succeed,-1:fail
 */
extern int product_test_etherent_set_auto_ip(char * interface);

/*@brief static  set ethernet ip addr 
 *@param(in) interface:etherent interface name
 *@param(in) ip:etherent static ip 
 *@return 0:succeed,-1:fail
 */
extern int product_test_etherent_set_static_ip(char *interface,uint32_t ip); 

/*@brief get ethernet link status  
 *@param(in) interface:etherent interface name
 *@param(out) status:etherent  status 1,link,0,unlink 
 *@return 0:succeed,-1:fail
 */
extern int product_test_etherent_get_link_status(char *interface,int * status);

/*@brief ethernet iperf test 
 *@param(in) path:test result save path
 *@param(in) iperf:iperf test parameter,see ETHERENT_IPERF_T
 *@return 0:succeed,-1:fail
 */
extern  int product_test_etherent_iperf_test(char *path,ETHERENT_IPERF_T *iperf);

#ifdef __cplusplus
}
#endif

#endif
