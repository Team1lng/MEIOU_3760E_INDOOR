#ifndef __PTAL_ZIGBEE_H__
#define __PTAL_ZIGBEE_H__

#ifdef __cplusplus__
extern "C" {
#endif
typedef struct zigbee_param 
{
    char name[64+1];
    int baud;
    int channel;
    int pkts;
}ZIGBEE_PARAM_T;

extern int ptal_zigbee_get_firmware_info(ZIGBEE_PARAM_T *param,char *buf, int size); 
extern int ptal_zigbee_get_version_info(ZIGBEE_PARAM_T *param,char *buf, int size);
extern int ptal_zigbee_test(ZIGBEE_PARAM_T *param,int *recv_ptks); 

#ifdef __cplusplus__
}
#endif

#endif
