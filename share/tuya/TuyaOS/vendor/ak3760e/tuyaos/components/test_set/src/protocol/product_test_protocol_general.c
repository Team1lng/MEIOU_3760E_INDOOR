/* 
 *  @copyright Copyright (c) tuya.inc 2021
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <inttypes.h>
#include "cJSON.h"
#include "tb_log.h"
#include "list.h"
#include "product_test_protocol_general.h"
#include "product_test_modem.h"
#include "product_test_gpio.h"

#define PT_GEN_HEAD_0 (0x55)  // general product protocol head
#define PT_GEN_HEAD_1 (0xaa)

#define PT_GEN_VER    (0)     // general product protocol version

#define PT_GEN_PROD_CMD (0)   // general product protocol product enter/exit cmd

#define PT_GEN_STR_TRUE "true"   // true string
#define PT_GEN_STR_FALSE "false" // false string
#define PT_GEN_FALSE_JSON "{\"ret\":false}"  // general protocol general return false string
#define PT_GEN_TRUE_JSON  "{\"ret\":true}"   // general protocol general return true string

#define PT_GEN_MIN_SIZE (7) // head(2) + version(1) + cmd(1) + data len(2) +  csum (1)
//#define PT_GEN_PT_MAX_SIZE  (512+PT_GEN_PT_MIN_SIZE) // for common protocal
#define PT_GEN_JSON_SIZE (4096)  //general protocol max encode json size
#define PT_GEN_MAX_SIZE (1024 + PT_GEN_MIN_SIZE) // for upgrade

typedef struct gen_frame_s
{
    unsigned char hd[2]; // head 
    unsigned char ver[1];// version
    unsigned char cmd[1];//cmd
    unsigned char dlen[2];//total len
    char *data;  //data
    unsigned char csum[1];// sum 
    unsigned int vdlen;  // valid data len 
    unsigned char *buf;  // data buffer
    unsigned char tlen;  // totol buffer
}PT_GEN_FRAME_T;

struct pt_cmd_table_s; //protocol command table,See also PT_CMD_TB_T

/*@brief  encode information from hardware information format to protocol information format 
 *@param(in) hbuf:information in hardware format,See PT_HBUF_T
 *@param(out)out:information in string  formation
 *@return 0:succeed,-1:fail
 */
typedef int (*CMD_ENCODE_T)(PT_HBUF_T *hbuf,char *out);

/*@brief  decode information from protocol information format to hardware information format
 *@param(in)cmd:comand  paramter,See PT_CMD_TB_T 
 *@param(in)frame:information in general protocol frame formation , See PT_GEN_FRAME_T
 *@param(out)dst:information in  hardware format,See PT_HBUF_T
 *@return 0:succeed,-1:fail
 */
typedef int (*CMD_DECODE_T)(struct pt_cmd_table_s *cmd,PT_GEN_FRAME_T *frame,PT_HBUF_T *hbuf);

typedef struct pt_cmd_table_s
{
    int index;   // command index
    int pt_type; // protocol type
    int hwl_mtype; // test main type
    int hwl_stype; //  test sub type
    CMD_ENCODE_T encode; // encode function
    CMD_DECODE_T decode; // decode function
}PT_CMD_TB_T;

typedef enum
{
    E_PT_GEN_MODE = 0x00,      // product enter mode
    E_PT_GEN_R_MASTER_FIRMWARE = 0x01,  // read master firmware information
    E_PT_GEN_R_SLAVE_FIRMWARE = 0x02,   // read slave firmware information
    E_PT_GEN_W_CFGINFO = 0x03,          // write config information
    E_PT_GEN_R_CFGINFO = 0x04,          // read config information
    E_PT_GEN_W_MASTER_MAC = 0x05,       // wirte mater mac address
    E_PT_GEN_R_MASTER_MAC = 0x06,       // read mater mac address
    E_PT_GEN_W_SLAVE_MAC = 0x07,        // write slave mac address
    E_PT_GEN_R_SLAVE_MAC = 0x08,        // read slave mac address
    E_PT_GEN_SELF_CHECK = 0x09,         // self check
    E_PT_GEN_BUTTON_TEST = 0x0a,        // button test 
    E_PT_GEN_LED_TEST = 0x0b,           // led test
    E_PT_GEN_W_BSN = 0x0c,              // write bsn
    E_PT_GEN_R_BSN = 0x0d,              // read bsn
    E_PT_GEN_W_SN = 0x0e,               // write sn
    E_PT_GEN_R_SN = 0x0f,               // read sn
    E_PT_GEN_MASTER_CAP = 0x10,         // matter capacity (rssi)
    E_PT_GEN_SLAVE_CAP = 0x11,          // slave capacity (rssi)
    E_PT_GEN_WIFI_SPEED = 0x12,         // wifi speed test (iperf)
    E_PT_GEN_CAMERA = 0x13,             // camera test
    E_PT_GEN_IRCUT  = 0x14,             // ircut test
    E_PT_GEN_SPEAKER = 0x15,            // speaker test 
    E_PT_GEN_MIC = 0x16,                // mic test
    E_PT_GEN_IRLED = 0x17,              // irled test
    E_PT_GEN_BLACK_CAMERA = 0x1A,       // black camera test
    E_PT_GEN_MOTOR  = 0x1B,             // motor test
    E_PT_GEN_W_COUNTRY = 0x1C,          // write country
    E_PT_GEN_R_COUNTRY = 0x1D,          // read country
    E_PT_GEN_UPGRADE_CFG = 0x1e,        // upgrade config
    E_PT_GEN_UPGRADE_START = 0x1f,      // upgrade start
    E_PT_GEN_UPGRADEING = 0x20,         // upgrade doing
    E_PT_GEN_UPGRADE_END = 0x21,        // upgrade end
    E_PT_GEN_REBOOT = 0x22,             // reboot
    E_PT_GEN_SLAVE_UPGRADE = 0x23,      // slave upgrade
    E_PT_GEN_PIR = 0x25,                // pir test
    E_PT_GEN_USB_TEST = 0x26,           // usb test
    E_PT_GEN_LTE_TEST = 0x29,           // modem test
    E_PT_GEN_FLAG_DECTECT = 0x2A,       // flag detect 
    E_PT_GEN_LTE_READ_CCID = 0x2B,      // read ccid
    E_PT_GEN_POWER = 0x2C,              // power test
    E_PT_GEN_ANALOG_SENSOR = 0x2D,      // analog sensor test
    E_PT_GEN_LTE_READ_IMSI = 0x2E,      // lte read imsi 
    E_PT_GEN_LTE_READ_IMEI = 0x2F,      // lte read imei
    E_PT_GEN_AGEING_TEST = 0x33,        // aging test
    E_PT_GEN_W_BATTERY_INFO= 0x38,      // write battery information
    E_PT_GEN_R_BATTERY_INFO = 0x39,     // read battery information
    E_PT_GEN_W_WIFI_CFG = 0x3A,         // write wifi config
    E_PT_GEN_LTE_READ_RSSI = 0x3B,      //lte read rssi
    E_PT_GEN_IR_SEND = 0x3C,            // ir send
    E_PT_GEN_IR_RECV = 0x3D,            // ir recv
    E_PT_GEN_IR_MODE = 0x3E,            // ir mode
    E_PT_GEN_GEN_DL_START = 0x40,       // general download start
    E_PT_GEN_GEN_DL = 0x41,             // general downloading 
    E_PT_GEN_GEN_DL_END= 0x42,          // general download end 
    E_PT_GEN_SWITCH_TEST = 0x43,        // switch test
    E_PT_GEN_GPIO_TEST = 0x46,          // gpio test 
    E_PT_GEN_BOOL_TEST=0x47,            // bool test 
    E_PT_GEN_MAX    // protocol type max
}PT_GEN_CMD_E;

enum{
    E_PROD_ENTER, //product enter
    E_PROD_EXIT,  // product exit
};

static int sg_start_prog_flag = 0;

int product_test_is_entered(void)
{
    return sg_start_prog_flag;
}

static int __product_test_decode_enter_prod(PT_CMD_TB_T *cmd,PT_GEN_FRAME_T *frame,PT_HBUF_T *hbuf)
{
    int iret = 0;
    cJSON *root;
    cJSON *action;
    int *parm;
    
    TB_LOGD();

    if(cmd == NULL || frame == NULL || hbuf == NULL) {
        TB_LOGE("invalid pram");
        return -1;  
    }

    hbuf->len = 0;

    hbuf->result = E_FALSE;
    hbuf->main_type = cmd->hwl_mtype;
    hbuf->sub_type = cmd->hwl_stype;

    parm = (int *)hbuf->data;

    if(frame->dlen[0] == 0 && frame->dlen[1] == 0) {  
        sg_start_prog_flag = 1;
        return 0;
    }

    if(frame->data == NULL) {
        TB_LOGE("frame data is NULL");
        return -1;
    }

    root = cJSON_Parse(frame->data);

    if(root == NULL) {
        TB_LOGE("cJSON_Parse err");
        return -1;
    }  

    action = cJSON_GetObjectItem(root,"action");

    if(action == NULL) {
        TB_LOGE("can't find action");
        cJSON_Delete(root);
        return -1;
    }

    if(strcmp("enter",action->valuestring) == 0) {
        sg_start_prog_flag = 1;
        *parm = E_PROD_ENTER;
        hbuf->len = sizeof(*parm);
    }else if(strcmp("exit",action->valuestring) == 0) {
        sg_start_prog_flag = 0;
        *parm = E_PROD_EXIT;
        hbuf->len = sizeof(*parm);
    }else {    
        iret = -1;
    }

    cJSON_Delete(root);

    return iret;
}

//cmd = E_PROD_ENTER, E_PROD_EXIT is same here
static int __product_test_encode_enter_prod(PT_HBUF_T *hbuf,char *out)
{
    PROD_INFO_T *inf;
    char *pt_ver;
    char *product_type;
    char *write_mac;
    

    TB_LOGD();
    if(hbuf == NULL || out == NULL) {
        TB_LOGE("invalid parm");
        return -1;
    }

    if(hbuf->result == E_FALSE) {
        TB_LOGE("read fail");
        return -1;
    }

    inf = (PROD_INFO_T*)(hbuf->data);
    
    if(strlen(inf->protocol_ver) == 0 || strlen(inf->product_type) == 0) {
        TB_LOGE("invalid info");
        return -1;
    }

    pt_ver = inf->protocol_ver; 
    product_type = inf->product_type;
 
    if(inf->mac_write== 1) {
        write_mac = PT_GEN_STR_TRUE;
    } else {
        write_mac = PT_GEN_STR_FALSE;
    }

    sprintf(out,"{\"comProtocol\":\"%s\",\"deviceType\":\"%s\",\"writeMac\":\"%s\"}",pt_ver,product_type,write_mac);
        
    return 0;   
}

static int __product_test_decode_common(PT_CMD_TB_T *cmd,PT_GEN_FRAME_T *frame,PT_HBUF_T *hbuf)
{

    if(cmd == NULL || frame == NULL || hbuf == NULL) {
        TB_LOGE("invalid pram");
        return -1;  
    }

    hbuf->len = 0;

    hbuf->result = E_FALSE;
    hbuf->main_type = cmd->hwl_mtype;
    hbuf->sub_type = cmd->hwl_stype;
    return 0;
}

static int __product_test_encode_common(PT_HBUF_T *hbuf,char *out)
{
    if(hbuf == NULL || out == NULL) {
        TB_LOGE("invalid parm");
        return -1;
    }

    TB_LOGD("hbuf:%p,result:%d",hbuf,hbuf->result);

    return hbuf->result == E_TRUE?0:-1;
}

static int __product_test_decode_r_mfirm(PT_CMD_TB_T *cmd,PT_GEN_FRAME_T *frame,PT_HBUF_T *hbuf)
{
    return __product_test_decode_common(cmd,frame,hbuf);

}

static int __product_test_encode_r_mfirm(PT_HBUF_T *hbuf,char *out)
{
    PROD_INFO_T *inf;

    TB_LOGD();

    if(hbuf == NULL || out == NULL) {
        TB_LOGE("invalid parm");
        return -1;
    }

    if(hbuf->result == E_FALSE) {
        TB_LOGE("read fail");
        return -1;
    }
    
    inf = (PROD_INFO_T*)(hbuf->data);

    if(strlen(inf->mfirmname) == 0 || strlen(inf->mfirmver)==0) {
        TB_LOGE("invalid info");
        return -1;
    }
    
    sprintf(out,"{\"ret\":true,\"firmName\":\"%s\",\"firmVer\":\"%s\"}",inf->mfirmname,inf->mfirmver);

    return 0;
}
    
static int __product_test_decode_r_sfirm(PT_CMD_TB_T *cmd,PT_GEN_FRAME_T *frame,PT_HBUF_T *hbuf)
{
    cJSON * root;
    cJSON * type;
    PROD_INFO_T *inf;
    
    if(cmd == NULL || frame == NULL || hbuf == NULL) {
        TB_LOGE("invalid pram");
        return -1;  
    }

    hbuf->len = 0;

    hbuf->result = E_FALSE;
    hbuf->main_type = cmd->hwl_mtype;
    hbuf->sub_type = cmd->hwl_stype;

    inf = (PROD_INFO_T*)(hbuf->data);

    if(frame->data == NULL) {
            TB_LOGE("frame data is NULL");
            return -1;
    }
    
    root = cJSON_Parse(frame->data);

    if(root == NULL) {
            TB_LOGE("cJSON_Parse err");
            return -1;
    }

    type = cJSON_GetObjectItem(root,"cpu");

    if(type == NULL) {
            TB_LOGE("can't find upgrade_type");
            cJSON_Delete(root);
            return -1;
        }

    inf->cpu = type->valueint;    
    cJSON_Delete(root);

    hbuf->len = sizeof(PROD_INFO_T);
    
    return 0;
}

static int __product_test_encode_r_sfirm(PT_HBUF_T *hbuf,char *out)
{
    PROD_INFO_T *inf;

    if(hbuf == NULL || out == NULL) {
        TB_LOGE("invalid parm");
        return -1;
    }

    if(hbuf->result == E_FALSE) {
        TB_LOGE("read fail");
        return -1;
    }
    
    inf = (PROD_INFO_T*)(hbuf->data);

    if(strlen(inf->sfirmname) == 0 || strlen(inf->sfirmver) == 0) {
        TB_LOGE("invalid info");
        return -1;
    }
    
    sprintf(out,"{\"ret\":true,\"firmName\":\"%s\",\"firmVer\":\"%s\"}",inf->sfirmname,inf->sfirmver);

    return 0;

}

static int __product_test_decode_w_cfg(PT_CMD_TB_T *cmd,PT_GEN_FRAME_T *frame,PT_HBUF_T *hbuf)
{
    cJSON *root,*auzkey;
    cJSON *uuid,*pid,*prod_test;
    cJSON *ap_pwd,*psk,*ap_ssid;
    CFG_PARM_T *parm;

    TB_LOGD();
    
    if(cmd == NULL || frame == NULL || hbuf == NULL) {
        TB_LOGE("invalid pram");
        return -1;  
    }

    hbuf->len = 0;

    hbuf->result = E_FALSE;
    hbuf->main_type = cmd->hwl_mtype;
    hbuf->sub_type = cmd->hwl_stype;


    parm = (CFG_PARM_T*)(hbuf->data);

    memset(parm,0,sizeof(CFG_PARM_T));

    if(frame->data == NULL) {
            TB_LOGE("frame data is NULL");
            return -1;
        }
    root = cJSON_Parse(frame->data);
    if(root == NULL) {
            TB_LOGE("cJSON_Parse err");
            return -1;
    }

    auzkey = cJSON_GetObjectItem(root,"auzkey");

    if(auzkey == NULL) {
        TB_LOGE("can't find auzkey");
        cJSON_Delete(root);
        return -1;
    }else{
        strncpy(parm->auzkey,auzkey->valuestring,PARM_MAX_SIZE);
    }
    
    uuid = cJSON_GetObjectItem(root,"uuid");
    if(uuid == NULL) {
        TB_LOGE("can't find uuid");
        cJSON_Delete(root);
        return -1;
    }else if (uuid->valuestring != NULL) {
        strncpy(parm->uuid,uuid->valuestring,PARM_MAX_SIZE);
    }
    
    pid = cJSON_GetObjectItem(root,"pid");

    if(pid != NULL && pid->valuestring != NULL) {
        strncpy(parm->pid,pid->valuestring,PARM_MAX_SIZE);
    }
    
    prod_test = cJSON_GetObjectItem(root,"prod_test");

    if(prod_test != NULL) {
        parm->prod_test = prod_test->valueint;  
    }
    
    ap_ssid = cJSON_GetObjectItem(root,"ap_ssid");

    if(ap_ssid != NULL && ap_ssid->valuestring != NULL) {
        strncpy(parm->ap_ssid,ap_ssid->valuestring,PARM_MIN_SIZE);
    }
    
    ap_pwd = cJSON_GetObjectItem(root,"ap_pwd");

    if(ap_pwd != NULL && ap_pwd->valuestring != NULL) {
        strncpy(parm->ap_pwd,ap_pwd->valuestring,PARM_MIN_SIZE);
    }

    psk = cJSON_GetObjectItem(root,"pskKey");

    if(psk != NULL && psk->valuestring != NULL){
        strncpy(parm->pskKey,psk->valuestring,PARM_MIN_SIZE);
    }
    return 0;
}

static int __product_test_encode_w_cfg(PT_HBUF_T *hbuf,char *out)
{
    return __product_test_encode_common(hbuf,out);
}

static int __product_test_decode_r_cfg(PT_CMD_TB_T *cmd,PT_GEN_FRAME_T *frame,PT_HBUF_T *hbuf)
{
    CFG_PARM_T *parm;
    
    if(cmd == NULL || frame == NULL || hbuf == NULL) {
        TB_LOGE("invalid pram");
        return -1;  
    }

    hbuf->len = 0;

    hbuf->result = E_FALSE;
    hbuf->main_type = cmd->hwl_mtype;
    hbuf->sub_type = cmd->hwl_stype;

    parm = (CFG_PARM_T*)(hbuf->data);

    memset(parm,0,sizeof(CFG_PARM_T));

    return 0;
}

static int __product_test_encode_r_cfg(PT_HBUF_T *hbuf,char *out)
{
    CFG_PARM_T *parm;

    if(hbuf == NULL || out == NULL) {
        TB_LOGE("invalid parm");
        return -1;
    }

    if(hbuf->result == E_FALSE) {
        TB_LOGE("read fail");
        return -1;
    }
    
    parm = (CFG_PARM_T*)(hbuf->data);
    
    if(parm->prod_test) {
        if(strlen(parm->pskKey) == 0) {
            sprintf(out,"{\"auzkey\":\"%s\",\"uuid\":\"%s\",\"pid\":\"%s\",\"prod_test\":true,\"ap_ssid\":\"%s\",\"ap_pwd\":\"%s\"}", \
                 parm->auzkey,parm->uuid,parm->pid,parm->ap_ssid,parm->ap_pwd);
        }else {
            sprintf(out,"{\"auzkey\":\"%s\",\"uuid\":\"%s\",\"pid\":\"%s\",\"prod_test\":true,\"ap_ssid\":\"%s\",\"ap_pwd\":\"%s\",\"pskKey\":\"%s\"}", \
                        parm->auzkey,parm->uuid,parm->pid,parm->ap_ssid,parm->ap_pwd,parm->pskKey);
            }
    }else {
            if(strlen(parm->pskKey) == 0) {
                sprintf(out,"{\"auzkey\":\"%s\",\"uuid\":\"%s\",\"pid\":\"%s\",\"prod_test\":false,\"ap_ssid\":\"%s\",\"ap_pwd\":\"%s\"}", \
                       parm->auzkey,parm->uuid,parm->pid,parm->ap_ssid,parm->ap_pwd);
            }else {
                sprintf(out,"{\"auzkey\":\"%s\",\"uuid\":\"%s\",\"pid\":\"%s\",\"prod_test\":false,\"ap_ssid\":\"%s\",\"ap_pwd\":\"%s\",\"pskKey\":\"%s\"}", \
                      parm->auzkey,parm->uuid,parm->pid,parm->ap_ssid,parm->ap_pwd,parm->pskKey);
            }
        }
    return 0;
}

static int __product_test_decode_w_mmac(PT_CMD_TB_T *cmd,PT_GEN_FRAME_T *frame,PT_HBUF_T *hbuf)
{
    CFG_PARM_T *parm;
    cJSON *root;
    cJSON *mac;
    
    if(cmd == NULL || frame == NULL || hbuf == NULL) {
        TB_LOGE("invalid pram");
        return -1;  
    }
    
    hbuf->len = 0;
    hbuf->result = E_FALSE;
    hbuf->main_type = cmd->hwl_mtype;
    hbuf->sub_type = cmd->hwl_stype;

    parm = (CFG_PARM_T*)(hbuf->data);


    if(frame->data == NULL) {
        TB_LOGE("frame data is NULL");
        return -1;
    }

    root = cJSON_Parse(frame->data);

    if(root == NULL) {
        TB_LOGE("cJSON_Parse err");
        return -1;
    }  

    mac = cJSON_GetObjectItem(root,"mac");
    if(mac == NULL) {
        TB_LOGE("can't find mac");
        cJSON_Delete(root);
        return -1;
    }

    strncpy(parm->mmac,mac->valuestring,PARM_MIN_SIZE);

    return 0;
    
}

static int __product_test_encode_w_mmac(PT_HBUF_T *hbuf,char *out)
{
    return __product_test_encode_common(hbuf,out);
}

static int __product_test_decode_r_mmac(PT_CMD_TB_T *cmd,PT_GEN_FRAME_T *frame,PT_HBUF_T *hbuf)
{
//  CFG_PARM_T *parm;

    if(cmd == NULL || frame == NULL || hbuf == NULL) {
        TB_LOGE("invalid pram");
        return -1;  
    }

    hbuf->len = 0;

    hbuf->result = E_FALSE;
    hbuf->main_type = cmd->hwl_mtype;
    hbuf->sub_type = cmd->hwl_stype;

//  parm = (CFG_PARM_T*)(hbuf->data);
    
    return 0;
}

static int __product_test_encode_r_mmac(PT_HBUF_T *hbuf,char *out)
{
    int iret = 0;
    CFG_PARM_T *parm;

    if(hbuf == NULL || out == NULL) {
        TB_LOGE("invalid parm");
        return -1;
    }
    
    if(hbuf->result == E_FALSE) {
        TB_LOGE("read fail");
        return -1;
    }

    parm = (CFG_PARM_T*)(hbuf->data);

    if(strlen(parm->mmac))
        sprintf(out,"{\"mac\":\"%s\"}",parm->mmac);
    else 
        iret = -1;
    
    return iret;
}

static int __product_test_decode_w_smac(PT_CMD_TB_T *cmd,PT_GEN_FRAME_T *frame,PT_HBUF_T *hbuf)
{
    CFG_PARM_T *parm;
    cJSON *root;
    cJSON *cpunum;
    cJSON *mac;
    
    if(cmd == NULL || frame == NULL || hbuf == NULL) {
        TB_LOGE("invalid pram");
        return -1;  
    }
    
    hbuf->len = 0;
    hbuf->result = E_FALSE;
    hbuf->main_type = cmd->hwl_mtype;
    hbuf->sub_type = cmd->hwl_stype;

    parm = (CFG_PARM_T*)(hbuf->data);

    if(frame->data == NULL) {
        TB_LOGE("frame data is NULL");
        return -1;
    }

    root = cJSON_Parse(frame->data);

    if(root == NULL) {
        TB_LOGE("cJSON_Parse err");
        return -1;
    }  

    cpunum = cJSON_GetObjectItem(root,"cpu");
    
    if(cpunum ==  NULL) {
        TB_LOGE("can't find cpunum");
        cJSON_Delete(root);
        return -1;
    }

    parm->snum = cpunum->valueint;
    
    mac = cJSON_GetObjectItem(root,"mac");
    
    if(mac == NULL) {
            TB_LOGE("can't find mac");
            cJSON_Delete(root);
            return -1;
    }

    strncpy(parm->smac,mac->valuestring,PARM_MIN_SIZE);

    return 0;
    
}

static int __product_test_encode_w_smac(PT_HBUF_T *hbuf,char *out)
{
    return __product_test_encode_common(hbuf,out);
}

static int __product_test_decode_r_smac(PT_CMD_TB_T *cmd,PT_GEN_FRAME_T *frame,PT_HBUF_T *hbuf)
{
    CFG_PARM_T *parm;
    cJSON *root;
    cJSON *cpunum;

    if(cmd == NULL || frame == NULL || hbuf == NULL) {
        TB_LOGE("invalid pram");
        return -1;  
    }

    hbuf->len = 0;

    hbuf->result = E_FALSE;
    hbuf->main_type = cmd->hwl_mtype;
    hbuf->sub_type = cmd->hwl_stype;

    parm = (CFG_PARM_T*)(hbuf->data);

    if(frame->data == NULL) {
        TB_LOGE("frame data is NULL");
        return -1;
    }
    
    root = cJSON_Parse(frame->data);
    if(root == NULL) {
        TB_LOGE("cJSON_Parse err");
            return -1;
    }

    cpunum = cJSON_GetObjectItem(root,"cpu");
    if(NULL == cpunum) {
        TB_LOGE("can't find cpunum");
        cJSON_Delete(root);
        return -1;
    }

    parm->snum = cpunum->valueint;

    cJSON_Delete(root);
    
    return 0;
}

static int __product_test_encode_r_smac(PT_HBUF_T *hbuf,char *out)
{
    int iret = -1;
    CFG_PARM_T *parm;

    if(hbuf == NULL || out == NULL) {
        TB_LOGE("invalid parm");
        return -1;
    }
    
    if(hbuf->result == E_FALSE) {
        TB_LOGE("read fail");
        return -1;
    }

    parm = (CFG_PARM_T*)(hbuf->data);

    if(hbuf->result == E_TRUE) {
        if(strlen(parm->smac)){
            sprintf(out,"{\"cpu\":%d,\"mac\":\"%s\"}",parm->snum,parm->smac);
            iret  = 0;
        }
    }
    return iret;
}

static int __product_test_decode_self_check(PT_CMD_TB_T *cmd,PT_GEN_FRAME_T *frame,PT_HBUF_T *hbuf)
{
    return __product_test_decode_common(cmd,frame,hbuf); 
}

static int __product_test_encode_self_check(PT_HBUF_T *hbuf,char *out)
{
    return __product_test_encode_common(hbuf,out);    
}

static int __product_test_decode_r_key(PT_CMD_TB_T *cmd,PT_GEN_FRAME_T *frame,PT_HBUF_T *hbuf)
{
    return __product_test_decode_common(cmd,frame,hbuf);
}


static int __product_test_encode_r_key(PT_HBUF_T *hbuf,char *out)
{
    int iret = -1;
    int key;

    if(hbuf == NULL || out == NULL) {
        TB_LOGE("invalid parm");
        return -1;
    }
    
    if(hbuf->result == E_FALSE) {
        TB_LOGE("read fail");
        return -1;
    }

    key = *(int*)(hbuf->data);

    if(hbuf->result == E_TRUE) {
        sprintf(out,"{\"keyEvent\":\"key%d\"}",key);
        iret  = 0;
    }
    return iret;
}

static int __product_test_decode_w_led(PT_CMD_TB_T *cmd,PT_GEN_FRAME_T *frame,PT_HBUF_T *hbuf)
{
    cJSON *root;
    cJSON *led; 
    int *state;
        
    if(cmd == NULL || frame == NULL || hbuf == NULL) {
        TB_LOGE("invalid pram");
        return -1;  
    }
        
    hbuf->len = 0;
    hbuf->result = E_FALSE;
    hbuf->main_type = cmd->hwl_mtype;
    hbuf->sub_type = cmd->hwl_stype;
    state  = (int *)hbuf->data;
    if(frame->data == NULL) {
        TB_LOGE("frame data is NULL");
        return -1;
    }
    
    root = cJSON_Parse(frame->data);
    
    if(root == NULL) {
        TB_LOGE("cJSON_Parse err");
        return -1;
    }  
    
    led = cJSON_GetObjectItem(root,"lightAction");
    
    if(led == NULL) {
        TB_LOGE("can't find led");
        cJSON_Delete(root);
            return -1;
        }
    
    *state = led->valueint;
    hbuf->len = sizeof(*state);
    cJSON_Delete(root);

    return 0;

}

static int __product_test_encode_w_led(PT_HBUF_T *hbuf,char *out)
{
    return __product_test_encode_common(hbuf,out);
}

static int __product_test_decode_w_bsn(PT_CMD_TB_T *cmd,PT_GEN_FRAME_T *frame,PT_HBUF_T *hbuf)
{
    cJSON *root;
    cJSON *bsn;
    CFG_PARM_T *inf;
    
    if(cmd == NULL || frame == NULL || hbuf == NULL) {
        TB_LOGE("invalid pram");
        return -1;  
    }
        
    hbuf->len = 0;
    hbuf->result = E_FALSE;
    hbuf->main_type = cmd->hwl_mtype;
    hbuf->sub_type = cmd->hwl_stype;

    inf = (CFG_PARM_T*)(hbuf->data);

    if(frame->data == NULL) {
        TB_LOGE("frame data is NULL");
        return -1;
    }
    
    root = cJSON_Parse(frame->data);
    
    if(root == NULL) {
        TB_LOGE("cJSON_Parse err");
        return -1;
    } 

    bsn = cJSON_GetObjectItem(root,"bsn");
    
    if(bsn == NULL || bsn->valuestring == NULL) {
        TB_LOGE("can't find bsn");
        cJSON_Delete(root);
        return -1;
    }

    
    strcpy(inf->bsn,bsn->valuestring);
    hbuf->len = strlen(inf->bsn) +1;
    cJSON_Delete(root);
    return 0;
}
static int __product_test_encode_w_bsn(PT_HBUF_T *hbuf,char *out)
{
    return __product_test_encode_common(hbuf,out);
}

static int __product_test_decode_r_bsn(PT_CMD_TB_T *cmd,PT_GEN_FRAME_T *frame,PT_HBUF_T *hbuf)
{
    return __product_test_decode_common(cmd,frame,hbuf);
}

static int __product_test_encode_r_bsn(PT_HBUF_T *hbuf,char *out)
{
    int iret = -1;
    CFG_PARM_T *inf;
    
    if(hbuf == NULL || out == NULL) {
        TB_LOGE("invalid parm");
        return -1;
    }
        
    if(hbuf->result == E_FALSE) {
        TB_LOGE("read fail");
        return -1;
    }
    
    inf = (CFG_PARM_T*)(hbuf->data);

    if(strlen(inf->bsn)) {
        sprintf(out,"{\"bsn\":\"%s\"}",inf->bsn);
        iret = 0;
    }
    
    return iret;
}

static int __product_test_decode_w_sn(PT_CMD_TB_T *cmd,PT_GEN_FRAME_T *frame,PT_HBUF_T *hbuf)
{
    cJSON *root;
    cJSON *sn;
    CFG_PARM_T *inf;

    TB_LOGD();
        
    if(cmd == NULL || frame == NULL || hbuf == NULL) {
        TB_LOGE("invalid pram");
        return -1;  
    }
        
    hbuf->len = 0;
    hbuf->result = E_FALSE;
    hbuf->main_type = cmd->hwl_mtype;
    hbuf->sub_type = cmd->hwl_stype;

    inf = (CFG_PARM_T*)(hbuf->data);
    
    if(frame->data == NULL) {
        TB_LOGE("frame data is NULL");
        return -1;
    }
    
    root = cJSON_Parse(frame->data);
    
    if(root == NULL) {
        TB_LOGE("cJSON_Parse err");
        return -1;
    } 

    sn = cJSON_GetObjectItem(root,"sn");
    
    if(sn == NULL || sn->valuestring == NULL) {
        TB_LOGE("can't find sn");
        cJSON_Delete(root);
        return -1;
    }

    strcpy(inf->sn,sn->valuestring);
    hbuf->len = strlen(inf->sn) +1;
    cJSON_Delete(root);
    return 0;
}
static int __product_test_encode_w_sn(PT_HBUF_T *hbuf,char *out)
{
    TB_LOGD();
    return __product_test_encode_common(hbuf,out);
}

static int __product_test_decode_r_sn(PT_CMD_TB_T *cmd,PT_GEN_FRAME_T *frame,PT_HBUF_T *hbuf)
{
    TB_LOGD();
    return __product_test_decode_common(cmd,frame,hbuf);
}

static int __product_test_encode_r_sn(PT_HBUF_T *hbuf,char *out)
{
    int iret = -1;
    CFG_PARM_T *inf;

    TB_LOGD();
    
    if(hbuf == NULL || out == NULL) {
        TB_LOGE("invalid parm");
        return -1;
    }
        
    if(hbuf->result == E_FALSE) {
        TB_LOGE("read fail");
        return -1;
    }
    
    inf = (CFG_PARM_T*) (hbuf->data);

    if(strlen(inf->sn)) {
        sprintf(out,"{\"sn\":\"%s\"}",inf->sn);
        iret = 0;
    }
    
    return iret;
}

static int __product_test_decode_mcap_test(PT_CMD_TB_T *cmd,PT_GEN_FRAME_T *frame,PT_HBUF_T *hbuf)
{
    cJSON *root;
    cJSON *cssid;
    cJSON *cchannel;
    cJSON *cnum;
    cJSON *cscan_all;
    WIRELESS_PARM_T *parm;

    if(cmd == NULL || frame == NULL || hbuf == NULL) {
        TB_LOGE("invalid pram");
        return -1;  
    }

    hbuf->len = 0;
    hbuf->result = E_FALSE;
    hbuf->main_type = cmd->hwl_mtype;
    hbuf->sub_type = cmd->hwl_stype;
    parm = (WIRELESS_PARM_T*)(hbuf->data);
    
    if(frame->data == NULL) {
        TB_LOGE("frame data is NULL");
        return -1;
    }
    
    root = cJSON_Parse(frame->data);
    
    if(root == NULL) {
        TB_LOGE("cJSON_Parse err");
        return -1;
    }

    cssid = cJSON_GetObjectItem(root,"ssid");
        
    if(cssid && cssid->valuestring) {
            strcpy((char *)parm->ssid,cssid->valuestring);
    }

    cchannel = cJSON_GetObjectItem(root,"channel");
    
    if(cchannel) {
        parm->channel = cchannel->valueint;
    }

    cnum = cJSON_GetObjectItem(root,"num");

    if(cnum) {
        parm->num = cnum->valueint;
    }

    cscan_all = cJSON_GetObjectItem(root,"scanAll");

    if(cscan_all) {
        parm->scan_all = cscan_all->valueint;   
    }

    hbuf->len = sizeof(WIRELESS_PARM_T);

    cJSON_Delete(root);

    return 0;
}


static int __product_test_encode_mcap_test(PT_HBUF_T *hbuf,char *out)
{
    int i,n=0;
    WIRELESS_PARM_T *parm;
    
    if(hbuf == NULL || out == NULL) {
        TB_LOGE("invalid parm");
        return -1;
    }
        
    if(hbuf->result == E_FALSE) {
        TB_LOGE("read fail");
        return -1;
    }
    
    parm = (WIRELESS_PARM_T*)(hbuf->data);

    if(!parm->scan_all) {
        sprintf(out,"{\"ret\":true,\"rssi\":%d}",parm->info[0].rssi); 
        return 0;
    }

    n += sprintf(out+n,"{\"ret\":true,\"num\":0,\"rssi\":0,\"rssiList\":[");    

    for(i=0;i< parm->totals-1;i++) {
        n += sprintf(out+n,"{\"ap\":\"%s\",\"rssi\":0,\"rate\":%d},",parm->info[i].ssid,parm->info[i].rssi);    
    }

    n += sprintf(out+n,"{\"ap\":\"%s\",\"rssi\":0,\"rate\":%d}]}",parm->info[i].ssid,parm->info[i].rssi);   
    
    return 0;
}


static int __product_test_decode_scap_test(PT_CMD_TB_T *cmd,PT_GEN_FRAME_T *frame,PT_HBUF_T *hbuf)
{
    cJSON *root;
    cJSON *cssid;
    cJSON *ccpu;
    cJSON *cnum;
    cJSON *cchannel;
    SCAP_PARM_T *parm;

    if(cmd == NULL || frame == NULL || hbuf == NULL) {
        TB_LOGE("invalid pram");
        return -1;  
    }
        
    hbuf->len = 0;
    hbuf->result = E_FALSE;
    hbuf->main_type = cmd->hwl_mtype;
    hbuf->sub_type = cmd->hwl_stype;
    parm = (SCAP_PARM_T*)(hbuf->data);

    memset(parm,0,sizeof(SCAP_PARM_T));
    
    if(frame->data == NULL) {
        TB_LOGE("frame data is NULL");
        return -1;
    }
    
    root = cJSON_Parse(frame->data);
    
    if(root == NULL) {
        TB_LOGE("cJSON_Parse err");
        return -1;
    }

    ccpu = cJSON_GetObjectItem(root,"cpu");

    if(ccpu == NULL) {
        TB_LOGE("can't find cpu");
        cJSON_Delete(root);
        return -1;
    }
    parm->cpu = ccpu->valueint;

    TB_LOGD("cpu:%d",parm->cpu);
    
    cssid = cJSON_GetObjectItem(root,"ssid");
    
    if(cssid != NULL && strlen(cssid->valuestring)) {
        strncpy(parm->ssid,cssid->valuestring,64);
	TB_LOGD("ssid:%s",parm->ssid);
    }

    cchannel = cJSON_GetObjectItem(root,"channel");

    if(cchannel) {
        parm->channel = cchannel->valueint; 
	TB_LOGD("channel:%d",parm->channel);
    }

    cnum = cJSON_GetObjectItem(root,"num");

    if(cnum) {
        parm->num = cnum->valueint;
	TB_LOGD("num:%d",parm->num);
    }

    cJSON_Delete(root);

    return 0;
}

static int __product_test_encode_scap_test(PT_HBUF_T *hbuf,char *out)
{
    SCAP_PARM_T *parm;
    
    if(hbuf == NULL || out == NULL) {
        TB_LOGE("invalid parm");
        return -1;
    }
        
    if(hbuf->result == E_FALSE) {
        TB_LOGE("read fail");
        return -1;
    }
    
    parm = (SCAP_PARM_T*)(hbuf->data);

    if(parm->rssi)
        sprintf(out,"{\"ret\":true,\"rssi\":%d}",parm->rssi); 
    else{
        sprintf(out,"{\"ret\":true,\"num\":%d}",parm->num); 
    }

    return 0;
}

// wifi test
static int __product_test_decode_wifi_test(PT_CMD_TB_T *cmd,PT_GEN_FRAME_T *frame,PT_HBUF_T *hbuf)
{
    cJSON *root;
    cJSON *server; 
    cJSON *time;
    WIFI_IPERF_T *iperf;

    TB_LOGD();
        
    if(cmd == NULL || frame == NULL || hbuf == NULL) {
        TB_LOGE("invalid pram");
        return -1;  
    }
        
    hbuf->len = 0;
    hbuf->result = E_FALSE;
    hbuf->main_type = cmd->hwl_mtype;
    hbuf->sub_type = cmd->hwl_stype;
    iperf = (WIFI_IPERF_T*)(hbuf->data);
    memset(iperf,0,sizeof(WIFI_IPERF_T));
    
    if(frame->data == NULL) {
        TB_LOGE("frame data is NULL");
        return -1;
    }
    
    root = cJSON_Parse(frame->data);
    
    if(root == NULL) {
        TB_LOGE("cJSON_Parse err");
        return -1;
    } 

    server = cJSON_GetObjectItem(root,"server");

    
    if(server != NULL && server->valuestring != NULL) {
        strcpy(iperf->server,server->valuestring);
    }


    time = cJSON_GetObjectItem(root,"time"); 

    if(time == NULL) {
        TB_LOGE("can't find time");
        cJSON_Delete(root);
        return -1;
    }

    iperf->time = time->valueint;
    TB_LOGD("time:%d",iperf->time);
    hbuf->len = sizeof(WIFI_IPERF_T);
    cJSON_Delete(root);

    return 0;
}

static int __product_test_encode_wifi_test(PT_HBUF_T *hbuf,char *out)
{
    WIFI_IPERF_T *iperf;
    
    if(hbuf == NULL || out == NULL) {
        TB_LOGE("invalid parm");
        return -1;
    }
        
    if(hbuf->result == E_FALSE) {
        TB_LOGE("read fail");
        return -1;
    }
    
    iperf = (WIFI_IPERF_T *) (hbuf->data);

    sprintf(out,"{\"ret\":true,\"bandwidth\":\"%.2lfMB/sec\",\"max\":\"%.2lfMB/sec\",\"min\":\"%.2lfMB/sec\"}",
                                            iperf->avr_speed/1000,iperf->max_speed/1000,iperf->min_speed/1000);
    return 0;
}

static int __product_test_decode_video_test(PT_CMD_TB_T *cmd,PT_GEN_FRAME_T *frame,PT_HBUF_T *hbuf)
{
    return __product_test_decode_common(cmd,frame,hbuf);
}

static int __product_test_encode_video_test(PT_HBUF_T *hbuf,char *out)
{
    char *ip;
    if(hbuf == NULL || out == NULL) {
        TB_LOGE("invalid parm");
        return -1;
    }
        
    if(hbuf->result == E_FALSE) {
        TB_LOGE("read fail");
        return -1;
    }

    ip = (char*) (hbuf->data);

    sprintf(out,"{\"ret\":true,\"rtspUrl\":\"rtsp://%s:8554/stream0\"}",ip);

    return 0;
}


static int __product_test_decode_ircut_test(PT_CMD_TB_T *cmd,PT_GEN_FRAME_T *frame,PT_HBUF_T *hbuf)
{
    cJSON *root;
    cJSON *ircut; 
    int *num;

    TB_LOGD();
        
    if(cmd == NULL || frame == NULL || hbuf == NULL) {
        TB_LOGE("invalid pram");
        return -1;  
    }
        
    hbuf->len = 0;
    hbuf->result = E_FALSE;
    hbuf->main_type = cmd->hwl_mtype;
    hbuf->sub_type = cmd->hwl_stype;
    num  = (int *)(hbuf->data);
    if(frame->data == NULL) {
        TB_LOGE("frame data is NULL");
        return -1;
    }
    
    root = cJSON_Parse(frame->data);
    
    if(root == NULL) {
        TB_LOGE("cJSON_Parse err");
        return -1;
    }  
    
    ircut = cJSON_GetObjectItem(root,"ircutSwitch");
    
        if(ircut == NULL) {
        TB_LOGE("can't find led");
        cJSON_Delete(root);
            return -1;
        }

    TB_LOGD("ircut:%d",ircut->valueint);
    
    *num = ircut->valueint;
    hbuf->len = sizeof(*num);
    return 0;
}

static int __product_test_encode_ircut_test(PT_HBUF_T *hbuf,char *out)
{
    return __product_test_encode_common(hbuf,out);
}

static int __product_test_decode_speaker_test(PT_CMD_TB_T *cmd,PT_GEN_FRAME_T *frame,PT_HBUF_T *hbuf)
{
    cJSON *root; 
    cJSON *sound; 
    int *mode;
    int tmp_val;

    if(cmd == NULL || frame == NULL || hbuf == NULL) {
        TB_LOGE("invalid pram");
        return -1;  
    }
        
    hbuf->len = 0;
    hbuf->result = E_FALSE;
    hbuf->main_type = cmd->hwl_mtype;
    hbuf->sub_type = cmd->hwl_stype;
    mode = (int*) (hbuf->data);
    
    if(frame->data == NULL) {
        TB_LOGE("frame data is NULL");
        return -1;
    }
    
    root = cJSON_Parse(frame->data);
    
    if(root == NULL) {
        TB_LOGE("cJSON_Parse err");
        return -1;
    } 

    sound = cJSON_GetObjectItem(root,"sound");

    if( sound == NULL) {
          TB_LOGE("can't find sound");
          cJSON_Delete(root);
          return -1;
    }

    tmp_val= sound->valueint;
    if (tmp_val == 1){
        *mode = E_SPEAKER_START;
    }else{
        *mode = E_SPEAKER_STOP;  
    }

    TB_LOGD("mode:%d,tmp_val:%d",*mode,tmp_val);

    hbuf->len =  sizeof(int);
    cJSON_Delete(root);

    return 0;
}

static int __product_test_encode_speaker_test(PT_HBUF_T *hbuf,char *out)
{
    return __product_test_encode_common(hbuf,out);
}


static int __product_test_decode_ir_led_test(PT_CMD_TB_T *cmd,PT_GEN_FRAME_T *frame,PT_HBUF_T *hbuf)
{
    cJSON *root;
    cJSON *led; 
    int *state;

    TB_LOGD();
        
    if(cmd == NULL || frame == NULL || hbuf == NULL) {
        TB_LOGE("invalid pram");
        return -1;  
    }
        
    hbuf->len = 0;
    hbuf->result = E_FALSE;
    hbuf->main_type = cmd->hwl_mtype;
    hbuf->sub_type = cmd->hwl_stype;
    state  = (int *)hbuf->data;
    if(frame->data == NULL) {
        TB_LOGE("frame data is NULL");
        return -1;
    }
    
    root = cJSON_Parse(frame->data);
    
    if(root == NULL) {
        TB_LOGE("cJSON_Parse err");
        return -1;
    }  
    
    led = cJSON_GetObjectItem(root,"irLED");
    
        if(led == NULL) {
        TB_LOGE("can't find led");
        cJSON_Delete(root);
            return -1;
        }

    TB_LOGD("led:%d",led->valueint);
    
    *state = led->valueint;
    hbuf->len = sizeof(*state);
    return 0;
}

static int __product_test_encode_ir_led_test(PT_HBUF_T *hbuf,char *out)
{
    return __product_test_encode_common(hbuf,out);
}

static int __product_test_decode_black_video_test(PT_CMD_TB_T *cmd,PT_GEN_FRAME_T *frame,PT_HBUF_T *hbuf)
{
    return __product_test_decode_common(cmd,frame,hbuf);
}

static int __product_test_encode_black_video_test(PT_HBUF_T *hbuf,char *out)
{
    char *ip;
    if(hbuf == NULL || out == NULL) {
        TB_LOGE("invalid parm");
        return -1;
    }
        
    if(hbuf->result == E_FALSE) {
        TB_LOGE("read fail");
        return -1;
    }

    ip = (char*) (hbuf->data);

    sprintf(out,"{\"ret\":true,\"rtspUrl\":\"rtsp://%s:8554/stream0\"}",ip);

    return 0;
}


static int __product_test_encode_motor_test(PT_HBUF_T *hbuf,char *out)
{
    return __product_test_encode_common(hbuf,out);
}

static int __product_test_decode_motor_test(PT_CMD_TB_T *cmd,PT_GEN_FRAME_T *frame,PT_HBUF_T *hbuf)
{
    return __product_test_decode_common(cmd,frame,hbuf);
}

static int __product_test_decode_w_country(PT_CMD_TB_T *cmd,PT_GEN_FRAME_T *frame,PT_HBUF_T *hbuf)
{
    cJSON *root;
    cJSON *country;
    CFG_PARM_T *inf;

    if(cmd == NULL || frame == NULL || hbuf == NULL) {
        TB_LOGE("invalid pram");
        return -1;  
    }
        
    hbuf->len = 0;
    hbuf->result = E_FALSE;
    hbuf->main_type = cmd->hwl_mtype;
    hbuf->sub_type = cmd->hwl_stype;
    inf = (CFG_PARM_T*) (hbuf->data);
    
    if(frame->data == NULL) {
        TB_LOGE("frame data is NULL");
        return -1;
    }
    
    root = cJSON_Parse(frame->data);
    
    if(root == NULL) {
        TB_LOGE("cJSON_Parse err");
        return -1;
    } 

    country = cJSON_GetObjectItem(root,"country");
    
    if(country == NULL) {
        TB_LOGE("can't find country");
        cJSON_Delete(root);
        return -1;
    }
    
    hbuf->len = strlen(inf->country) +1;
    strcpy(inf->country,country->valuestring);
    cJSON_Delete(root);
	return 0;
}

static int __product_test_encode_w_country(PT_HBUF_T *hbuf,char *out)
{
    return __product_test_encode_common(hbuf,out);
}

static int __product_test_decode_r_country(PT_CMD_TB_T *cmd,PT_GEN_FRAME_T *frame,PT_HBUF_T *hbuf)
{
    return __product_test_decode_common(cmd,frame,hbuf);
}

static int __product_test_encode_r_country(PT_HBUF_T *hbuf,char *out)
{
    int iret = -1;
    CFG_PARM_T *inf;
    
    if(hbuf == NULL || out == NULL) {
        TB_LOGE("invalid parm");
        return -1;
    }
        
    if(hbuf->result == E_FALSE) {
        TB_LOGE("read fail");
        return -1;
    }
    
    inf = (CFG_PARM_T*) (hbuf->data);

    if(strlen(inf->country)) {
        sprintf(out,"{\"ret\":true,\"country\":\"%s\"}",inf->country);
        iret = 0;
    }
    return iret;
}

//UPGRADE_CFG_T

static int __product_test_decode_upgrade_cfg(PT_CMD_TB_T *cmd,PT_GEN_FRAME_T *frame,PT_HBUF_T *hbuf)
{
    cJSON *root;
    cJSON *baud;
    cJSON *crc32;
    cJSON *size;
    unsigned long baud_i;
    unsigned long crc32_i;
    unsigned long size_i;
    UPGRADE_CFG_T *ucfg;

    if(cmd == NULL || frame == NULL || hbuf == NULL) {
        TB_LOGE("invalid pram");
        return -1;  
    }
        
    hbuf->len = 0;
    hbuf->result = E_FALSE;
    hbuf->main_type = cmd->hwl_mtype;
    hbuf->sub_type = cmd->hwl_stype;
    ucfg = (UPGRADE_CFG_T*) (hbuf->data);
    
    if(frame->data == NULL) {
        TB_LOGE("frame data is NULL");
        return -1;
    }
    
    root = cJSON_Parse(frame->data);
    
    if(root == NULL) {
        TB_LOGE("cJSON_Parse err");
        return -1;
    } 

    baud = cJSON_GetObjectItem(root,"baud");
    
    if(baud == NULL) {
          TB_LOGE("can't find baud,set baud 0");
          baud_i = 0;
    }else {
          baud_i =  (unsigned long)baud->valueint;
    }

    crc32 = cJSON_GetObjectItem(root,"crc32");

    if(crc32 == NULL) {
          TB_LOGE("can't find crc32");
          cJSON_Delete(root);
    }

    crc32_i = (unsigned long)crc32->valueint;
    
    size = cJSON_GetObjectItem(root,"size");

    if(size == NULL) {
          TB_LOGE("can't find size");
          cJSON_Delete(root);
    }

    size_i =(unsigned long) size->valueint;
    cJSON_Delete(root);

    ucfg->baud = baud_i;
    ucfg->crc32 = crc32_i;
    ucfg->size = size_i;
    TB_LOGD("baud:%ld,crc32:%ld,size:%ld\n",ucfg->baud,ucfg->crc32,ucfg->size);
    return 0;
}

static int __product_test_encode_upgrade_cfg(PT_HBUF_T *hbuf,char *out)
{
    return __product_test_encode_common(hbuf,out);
}

static int __product_test_decode_upgrade_start(PT_CMD_TB_T *cmd,PT_GEN_FRAME_T *frame,PT_HBUF_T *hbuf)
{
    return __product_test_decode_common(cmd,frame,hbuf);
}

static int __product_test_encode_upgrade_start(PT_HBUF_T *hbuf,char *out)
{
    UPGRADE_CFG_T *ucfg;
    unsigned long packet_size;
    
    TB_LOGD();

    if(hbuf == NULL || out == NULL) {
            TB_LOGE("invalid parm");
            return -1;
    }

    ucfg = (UPGRADE_CFG_T*) (hbuf->data);

    packet_size = ucfg->packet_size;

    if(hbuf->result == E_FALSE){
        TB_LOGE("modem enter test mode fail");
        sprintf(out, PT_GEN_FALSE_JSON );
        return -1;
    }

    TB_LOGD("packet_size:%ld",packet_size);

    sprintf(out, "{\"ret\":true,\"size\":%ld}",packet_size);

    return 0;
}

static int __product_test_decode_upgrade_recv(PT_CMD_TB_T *cmd,PT_GEN_FRAME_T *frame,PT_HBUF_T *hbuf)
{
        
    if(cmd == NULL || frame == NULL || hbuf == NULL) {
        TB_LOGE("invalid pram");
        return -1;  
    }

    hbuf->len = frame->vdlen;
    hbuf->result = E_FALSE;
    hbuf->main_type = cmd->hwl_mtype;
    hbuf->sub_type = cmd->hwl_stype;
    memcpy(hbuf->data,frame->data,frame->vdlen);

    return 0;
}

static int __product_test_encode_upgrade_recv(PT_HBUF_T *hbuf,char *out)
{
    return __product_test_encode_common(hbuf,out);
}

static int __product_test_decode_upgrade_end(PT_CMD_TB_T *cmd,PT_GEN_FRAME_T *frame,PT_HBUF_T *hbuf)
{
    return __product_test_decode_common(cmd,frame,hbuf);
}

static int __product_test_encode_upgrade_end(PT_HBUF_T *hbuf,char *out)
{
    return __product_test_encode_common(hbuf,out);
}

static int __product_test_decode_reboot(PT_CMD_TB_T *cmd,PT_GEN_FRAME_T *frame,PT_HBUF_T *hbuf)
{
    return __product_test_decode_common(cmd,frame,hbuf);
}

static int __product_test_encode_reboot(PT_HBUF_T *hbuf,char *out)
{
    return __product_test_encode_common(hbuf,out);
}

static int __product_test_decode_upgrade_slave(PT_CMD_TB_T *cmd,PT_GEN_FRAME_T *frame,PT_HBUF_T *hbuf)
{
    int *up_type;
    cJSON *root;
    cJSON *upgrade_type;

    if(cmd == NULL || frame == NULL || hbuf == NULL) {
        TB_LOGE("invalid pram");
        return -1;  
    }

    hbuf->len = 0;

    hbuf->result = E_FALSE;
    hbuf->main_type = cmd->hwl_mtype;
    hbuf->sub_type = cmd->hwl_stype;

    up_type = (int*)(hbuf->data);

    if(frame->data == NULL) {
        TB_LOGE("frame data is NULL");
        return -1;
    }

    root = cJSON_Parse(frame->data);
    if(root == NULL) {
        TB_LOGE("cJSON_Parse err");
        return -1;
    }

    upgrade_type = cJSON_GetObjectItem(root,"cpu");

    if(upgrade_type == NULL) {
        TB_LOGE("can't find upgrade_type");
        cJSON_Delete(root);
        return -1;
    }
        
    *up_type = upgrade_type->valueint;
    hbuf->len = sizeof(*up_type);
    
    cJSON_Delete(root);

    return 0;
}

static int __product_test_encode_upgrade_slave(PT_HBUF_T *hbuf,char *out)
{
    return __product_test_encode_common(hbuf,out);
}

static int __product_test_decode_pir_test(PT_CMD_TB_T *cmd,PT_GEN_FRAME_T *frame,PT_HBUF_T *hbuf)
{
    cJSON *root;
    cJSON *ctype;
    int *type;
    
    TB_LOGD();

    if(cmd == NULL || frame == NULL || hbuf == NULL) {
        TB_LOGE("invalid pram");
        return -1;  
    }

    hbuf->len = 0;
    hbuf->result = E_FALSE;
    hbuf->main_type = cmd->hwl_mtype;
    hbuf->sub_type = cmd->hwl_stype;

    type = (int*)(hbuf->data);

    *type = 0;

    if(frame->data == NULL) {
        TB_LOGW("frame data is NULL");
        return 0;
    }
    
    root = cJSON_Parse(frame->data);

    if(root == NULL) {
        TB_LOGE("cJSON_Parse err");
        return -1;
    }

    ctype = cJSON_GetObjectItem(root,"type");

    if(ctype != NULL && ctype->valuestring != NULL && strcmp(ctype->valuestring,"PCBA") == 0) {
        *type = 1;	  
    }

    cJSON_Delete(root);

    hbuf->len = sizeof(int);

    TB_LOGD("type:%d",*type);

    return 0;
}

static int __product_test_encode_pir_test(PT_HBUF_T *hbuf,char *out)
{
    int iret = -1;
    TB_LOGD();

    if(hbuf == NULL || out == NULL) {
            TB_LOGE("invalid parm");
    }

    if(hbuf->result == E_FALSE){
        sprintf(out,"{\"ret\":false,\"pirEvent\":[]}"); 
    }else {
        sprintf(out,"{\"ret\":true,\"pirEvent\":[1]}");
        iret = 0;
    }

    return iret;
}


static int __product_test_decode_usb_test(PT_CMD_TB_T *cmd,PT_GEN_FRAME_T *frame,PT_HBUF_T *hbuf)
{
    return __product_test_decode_common(cmd,frame,hbuf);
}

static int __product_test_encode_usb_test(PT_HBUF_T *hbuf,char *out)
{
    return __product_test_encode_common(hbuf,out);
}

static int __product_test_decode_modem_test(PT_CMD_TB_T *cmd,PT_GEN_FRAME_T *frame,PT_HBUF_T *hbuf)
{
    cJSON *root;
    cJSON *stage;
    int *mode;
    
    TB_LOGD();
    if(cmd == NULL || frame == NULL || hbuf == NULL) {
        TB_LOGE("invalid pram");
        return -1;  
    }

    hbuf->len = 0;

    hbuf->result = E_FALSE;
    hbuf->main_type = cmd->hwl_mtype;
    hbuf->sub_type = cmd->hwl_stype;

    mode = (int*)(hbuf->data);

    if(frame->data == NULL) {
        TB_LOGE("frame data is NULL");
        return -1;
    }
    
    root = cJSON_Parse(frame->data);
    if(root == NULL) {
        TB_LOGE("cJSON_Parse err");
        return -1;
    }

    stage = cJSON_GetObjectItem(root,"stage");

    if(stage == NULL) {
        TB_LOGE("can't find stage");
        cJSON_Delete(root);
        return -1;
    }
    
    if(strcmp(stage->valuestring,"start") == 0) {
             *mode = E_MODEM_ENTER_TEST;
         }else if(strcmp(stage->valuestring,"end") == 0) {
              *mode = E_MODEM_EXIT_TEST;
         }else if(strcmp(stage->valuestring,"first") == 0) {
             *mode = E_MODEM_TEST_MODE1;
         }else if(strcmp(stage->valuestring,"second") == 0) {
             *mode = E_MODEM_TEST_MODE2;
         }else if(strcmp(stage->valuestring,"third") == 0) {
             *mode = E_MODEM_TEST_MODE3;
         }
        
    cJSON_Delete(root);

    return 0;
}

static int __product_test_encode_modem_test(PT_HBUF_T *hbuf,char *out)
{
    int mode;
    char *stage;
    
    TB_LOGD();
    if(hbuf == NULL || out == NULL) {
            TB_LOGE("invalid parm");
            return -1;
    }

    mode = *((int *)(hbuf->data));
    
    if(mode == E_MODEM_ENTER_TEST) {
        stage = "start";
    }else if(mode == E_MODEM_EXIT_TEST) {
        stage = "end";
    }else if(mode == E_MODEM_TEST_MODE1) {
        stage = "first";
    }else if(mode == E_MODEM_TEST_MODE2) {
        stage = "second";
    }else if(mode == E_MODEM_TEST_MODE3) {
        stage = "third";
    }else {
        TB_LOGE("invalid stage");
        return -1;
    }

    if(hbuf->result == E_FALSE){
        TB_LOGE("modem enter test mode fail");
        sprintf(out, "{\"stage\":\"%s\",\"ret\":false}",stage);
        return -1;
    }

    sprintf(out, "{\"stage\":\"%s\",\"ret\":true}",stage);

    return 0;
}


static int __product_test_decode_flag_dect(PT_CMD_TB_T *cmd,PT_GEN_FRAME_T *frame,PT_HBUF_T *hbuf)
{
    int iret = 0;
    cJSON *root;
    cJSON *flag; 
    int *flag_dect;

    if(cmd == NULL || frame == NULL || hbuf == NULL) {
        TB_LOGE("invalid pram");
        return -1;  
    }
    
    hbuf->len = 0;
    
    hbuf->result = E_FALSE;
    hbuf->main_type = cmd->hwl_mtype;
    hbuf->sub_type = cmd->hwl_stype;
    
    flag_dect = (int*)(hbuf->data);
    
    if(frame->data == NULL) {
        TB_LOGE("frame data is NULL");
        return -1;
    }
        
    root = cJSON_Parse(frame->data);
    if(root == NULL) {
        TB_LOGE("cJSON_Parse err");
        return -1;
    }
    
    flag = cJSON_GetObjectItem(root,"flagType");
    if(flag == NULL) {
            TB_LOGE("can't find flag");
            cJSON_Delete(root);
            return -1;
        }
    
    if(strcmp(flag->valuestring,"wifi_rf") == 0) {
            *flag_dect  = E_FLAG_DECT_WIFIRF;                       
        }else if(strcmp(flag->valuestring,"board") == 0) {
            *flag_dect  = E_FLAG_DECT_BOARDTEST;         
    }else {
            iret = -1;    
        }
    cJSON_Delete(root);
    
    return iret;

}

static int __product_test_encode_flag_dect(PT_HBUF_T *hbuf,char *out)
{
    int flag_dect;
    char *flag_type;
    
    if(hbuf == NULL || out == NULL) {
            TB_LOGE("invalid parm");
            return -1;
    }

    flag_dect = *((int *)(hbuf->data));
        
    if(hbuf->result == E_FALSE) {
        if(flag_dect == E_FLAG_DECT_WIFIRF) {
            sprintf(out,"{\"flagType\":\"wifi_rf\",\"ret\":false}"); 
            return 0;
        }else if(flag_dect == E_FLAG_DECT_BOARDTEST) {
             sprintf(out,"{\"flagType\":\"board\",\"ret\":false}");
             return 0;
        }
    }

    if(flag_dect == E_FLAG_DECT_WIFIRF) {
        flag_type = "wifi_rf";
    }else if(flag_dect == E_FLAG_DECT_BOARDTEST) {
        flag_type = "board";
    }else {
        TB_LOGE("invalid parm");
        return -1;
    }

    sprintf(out,"{\"flagType\":\"%s\",\"ret\":true}",flag_type);

    return 0;
}

static int __product_test_decode_r_ccid(PT_CMD_TB_T *cmd,PT_GEN_FRAME_T *frame,PT_HBUF_T *hbuf)
{
    return __product_test_decode_common(cmd,frame,hbuf);
}

static int __product_test_encode_r_ccid(PT_HBUF_T *hbuf,char *out)
{
    int iret = -1;
    MODEM_DATA_T *data;
        
    if(hbuf == NULL || out == NULL) {
            TB_LOGE("invalid parm");
            return -1;
    }
            
    if(hbuf->result == E_FALSE) {
            TB_LOGE("read fail");
            return -1;
    }

    data = (MODEM_DATA_T*)(hbuf->data);

    if(strlen(data->ccid)) {
         sprintf(out,"{\"ccid\":\"%s\",\"ret\":true}",data->ccid);
         iret = 0;
    }

    return iret;

}

static int __product_test_decode_r_imsi(PT_CMD_TB_T *cmd,PT_GEN_FRAME_T *frame,PT_HBUF_T *hbuf)
{
    return __product_test_decode_common(cmd,frame,hbuf);
}

static int __product_test_encode_r_imsi(PT_HBUF_T *hbuf,char *out)
{
    int iret = -1;
    MODEM_DATA_T *data;

    if(hbuf == NULL || out == NULL) {
        TB_LOGE("invalid parm");
        return -1;
    }

    if(hbuf->result == E_FALSE) {
        TB_LOGE("read fail");
        return -1;
    }
    
    data = (MODEM_DATA_T*)(hbuf->data);

    if(strlen(data->imsi)) {
        sprintf(out,"{\"imsi\":\"%s\",\"ret\":true}",data->imsi);
        iret = 0;
    }

    return iret;
}

static int __product_test_decode_r_imei(PT_CMD_TB_T *cmd,PT_GEN_FRAME_T *frame,PT_HBUF_T *hbuf)
{
    return __product_test_decode_common(cmd,frame,hbuf);
}

static int __product_test_encode_r_imei(PT_HBUF_T *hbuf,char *out)
{
        int iret = -1;
        MODEM_DATA_T *data;

        if(hbuf == NULL || out == NULL) {
                TB_LOGE("invalid parm");
                return -1;
        }

        if(hbuf->result == E_FALSE) {
                TB_LOGE("read fail");
                return -1;
        }

        data = (MODEM_DATA_T*)(hbuf->data);

        if(strlen(data->imei)) {
                sprintf(out,"{\"imei\":\"%s\",\"ret\":true}",data->imei);
                iret = 0;
        }

    return iret;
}

static int __product_test_decode_w_battery_info(PT_CMD_TB_T *cmd,PT_GEN_FRAME_T *frame,PT_HBUF_T *hbuf) 
{
    cJSON *root;
    cJSON *cfactory;
    cJSON *ctype;
    cJSON *ccapacity;
    cJSON *cinfo;

    BATTERY_PARM_T *parm;

    TB_LOGD();
    
    if(cmd == NULL || frame == NULL || hbuf == NULL) {
        TB_LOGE("invalid pram");
        return -1;  
    }

    hbuf->len = 0;
    hbuf->result = E_FALSE;
    hbuf->main_type = cmd->hwl_mtype;
    hbuf->sub_type = cmd->hwl_stype;

    parm = (BATTERY_PARM_T*)(hbuf->data);

    if(frame->data == NULL) {
            TB_LOGE("frame data is NULL");
            return -1;
        }

    root = cJSON_Parse(frame->data);
    if(root == NULL) {
            TB_LOGE("cJSON_Parse err");
            return -1;
    }

    cfactory = cJSON_GetObjectItem(root,"factory");

    if(cfactory == NULL) {
        TB_LOGE("can't find ssid");
        cJSON_Delete(root);
        return -1;
    }

    strncpy(parm->b_factory,cfactory->valuestring,PARM_MAX_SIZE);

    ctype = cJSON_GetObjectItem(root,"type");

    if(ctype == NULL) {
        TB_LOGE("can't find type");
        cJSON_Delete(root);
        return -1;
    }

    strncpy(parm->b_type,ctype->valuestring,PARM_MIN_SIZE);

    ccapacity = cJSON_GetObjectItem(root,"capacity");

    if(ccapacity == NULL) {
        TB_LOGE("can't find type");
        cJSON_Delete(root);
        return -1;
    }
    parm->b_capacity = ccapacity->valueint;

        
    cinfo = cJSON_GetObjectItem(root,"info");

    if(cinfo == NULL) {
        TB_LOGE("can't find type");
        cJSON_Delete(root);
        return -1;
    }

    strncpy(parm->b_info,cinfo->valuestring,PARM_MAX_SIZE*2);

    hbuf->len = sizeof(BATTERY_PARM_T);
    cJSON_Delete(root);
    
    return 0;
}

static int __product_test_encode_w_battery_info(PT_HBUF_T *hbuf,char *out)
{
    return __product_test_encode_common(hbuf,out);
}

static int __product_test_decode_r_battery_info(PT_CMD_TB_T *cmd,PT_GEN_FRAME_T *frame,PT_HBUF_T *hbuf) 
{
    return __product_test_decode_common(cmd,frame,hbuf);
}

static int __product_test_encode_r_battery_info(PT_HBUF_T *hbuf,char *out)
{
    BATTERY_PARM_T *parm;

    if(hbuf == NULL || out == NULL) {
        TB_LOGE("invalid parm");
        return -1;
    }

    if(hbuf->result == E_FALSE) {
        TB_LOGE("read fail");
        return -1;
    }

    parm = (BATTERY_PARM_T*)(hbuf->data);

    sprintf(out,"{\"ret\":true,\"factory\":\"%s\",\"type\":\"%s\",\"capacity\":%d,\"info\":\"%s\"}",
            parm->b_factory,parm->b_type,parm->b_capacity,parm->b_info);
    
    return 0;
}

static int __product_test_decode_w_wifi_cfg(PT_CMD_TB_T *cmd,PT_GEN_FRAME_T *frame,PT_HBUF_T *hbuf) 
{
    cJSON *root;
    cJSON *cssid;
    cJSON *cpwd;
    CFG_PARM_T *parm;

    TB_LOGD();
    
    if(cmd == NULL || frame == NULL || hbuf == NULL) {
        TB_LOGE("invalid pram");
        return -1;  
    }

    hbuf->len = 0;
    hbuf->result = E_FALSE;
    hbuf->main_type = cmd->hwl_mtype;
    hbuf->sub_type = cmd->hwl_stype;

    parm = (CFG_PARM_T*)(hbuf->data);

    memset(parm,0,sizeof(CFG_PARM_T));

    if(frame->data == NULL) {
            TB_LOGE("frame data is NULL");
            return -1;
        }

    root = cJSON_Parse(frame->data);
    if(root == NULL) {
            TB_LOGE("cJSON_Parse err");
            return -1;
    }

    cssid = cJSON_GetObjectItem(root,"ssid");

    if(cssid == NULL) {
        TB_LOGE("can't find ssid");
        cJSON_Delete(root);
        return -1;
    }else{
        strncpy(parm->ap_ssid,cssid->valuestring,PARM_MIN_SIZE);
    }

    cpwd = cJSON_GetObjectItem(root,"pwd");

    if(cpwd == NULL) {
        TB_LOGE("can't find pwd");
        cJSON_Delete(root);
        return -1;
    }else{
        strncpy(parm->ap_pwd,cpwd->valuestring,PARM_MIN_SIZE);
    }

    hbuf->len = sizeof(CFG_PARM_T);
    cJSON_Delete(root);
    
    return 0;
}

static int __product_test_encode_w_wifi_cfg(PT_HBUF_T *hbuf,char *out)
{
    return __product_test_encode_common(hbuf,out);
}

static int __product_test_decode_r_rssi(PT_CMD_TB_T *cmd,PT_GEN_FRAME_T *frame,PT_HBUF_T *hbuf)
{
    return __product_test_decode_common(cmd,frame,hbuf);
}

static int __product_test_encode_r_rssi(PT_HBUF_T *hbuf,char *out)
{
    int iret = -1;
    MODEM_DATA_T *data;

    if(hbuf == NULL || out == NULL) {
        TB_LOGE("invalid parm");
        return -1;
    }

    if(hbuf->result == E_FALSE) {
        TB_LOGE("read fail");
        return -1;
    }

    data = (MODEM_DATA_T*)(hbuf->data);

    if((data->rssi >= -113) && (data->rssi <= -51 )) {
        sprintf(out,"{\"ret\":true,\"rssi\":%d}",data->rssi);
        iret = 0;
    }

    return iret;
}


static char *power_type[] = {
    "capacity",
    "voltage",
    "charger",
};

static int __product_test_decode_r_power(PT_CMD_TB_T *cmd,PT_GEN_FRAME_T *frame,PT_HBUF_T *hbuf) 
{
    int iret = 0;
    cJSON *root;
    cJSON *type;
    BATTERY_PARM_T *parm;

    TB_LOGD();
        
    if(cmd == NULL || frame == NULL || hbuf == NULL) {
        TB_LOGE("invalid pram");
        return -1;  
    }
        
    hbuf->len = 0;
    hbuf->result = E_FALSE;
    hbuf->main_type = cmd->hwl_mtype;
    hbuf->sub_type = cmd->hwl_stype;
    parm  = (BATTERY_PARM_T *)hbuf->data;

    if(frame->data == NULL) {
        TB_LOGE("frame data is NULL");
        return -1;
    }
    
    root = cJSON_Parse(frame->data);
    
    if(root == NULL) {
        TB_LOGE("cJSON_Parse err");
        return -1;
    }

    type = cJSON_GetObjectItem(root,"type");

    if(type == NULL) {
        TB_LOGE("can't find type");
        cJSON_Delete(root);
        return -1;
    }
    if(strcmp(type->valuestring,power_type[0]) ==0) {
        parm->type =  E_POWER_CAPACITY;
    }else if (strcmp(type->valuestring,power_type[1]) ==0) {
        parm->type =  E_POWER_VOLTAGE;
    }else if (strcmp(type->valuestring,power_type[2]) ==0) {
        parm->type =  E_POWER_CHARGER;
    }else {
        TB_LOGE("invalid type:%s",type->valuestring);
        iret = -1;
    }
    
    
    return iret;
}

static int __product_test_encode_r_power(PT_HBUF_T *hbuf,char *out)
{
    int iret = -1;
    BATTERY_PARM_T *parm;
    char *type;

    if(hbuf == NULL || out == NULL) {
        TB_LOGE("invalid parm");
        return -1;
    }
    
    parm = (BATTERY_PARM_T*)(hbuf->data);

    switch(parm->type) {
        case E_POWER_CAPACITY:
            type = power_type[0];
            break;
        case E_POWER_VOLTAGE:
            type = power_type[1];
            break;
        case E_POWER_CHARGER:
            type = power_type[2];
            break;
        default:
            TB_LOGE("invalid type:%d",parm->type);
            return -1;
    }

    if(hbuf->result == E_TRUE) {
        sprintf(out,"{\"type\":\"%s\",\"val\":%d,\"ret\":true}",type,parm->value);
        iret  = 0;
    }else {
        sprintf(out,"{\"ret\":false}");
    }
    return iret;
    return 0;
}

static int __product_test_decode_r_adc(PT_CMD_TB_T *cmd,PT_GEN_FRAME_T *frame,PT_HBUF_T *hbuf) 
{
    cJSON *root;
    cJSON *index;
    cJSON *tmptype;
    ADC_PARM_T *parm;

    TB_LOGD();
        
    if(cmd == NULL || frame == NULL || hbuf == NULL) {
        TB_LOGE("invalid pram");
        return -1;  
    }
        
    hbuf->len = 0;
    hbuf->result = E_FALSE;
    hbuf->main_type = cmd->hwl_mtype;
    hbuf->sub_type = cmd->hwl_stype;
    parm  = (ADC_PARM_T *)hbuf->data;

    if(frame->data == NULL) {
        TB_LOGE("frame data is NULL");
        return -1;
    }
    
    root = cJSON_Parse(frame->data);
    
    if(root == NULL) {
        TB_LOGE("cJSON_Parse err");
        return -1;
    }  
    
    index = cJSON_GetObjectItem(root,"index");
    
    if(index == NULL) {
        TB_LOGE("can't find light");
        cJSON_Delete(root);
            return -1;
        }
    
    parm->channel = index->valueint;

    TB_LOGD("adc channel:%d",parm->channel);
    tmptype = cJSON_GetObjectItem(root,"type");
        if(tmptype == NULL) {
            TB_LOGE("can't find flag");
            cJSON_Delete(root);
            return -1;
        }

        if(strcmp(tmptype->valuestring,"light") == 0) {
        parm->type = E_SENSOR_LIGHT;
    }else if(strcmp(tmptype->valuestring,"adc") == 0){
        parm->type = E_SENSOR_ADC;
    }else{
         TB_LOGE("can't find type\n");
         return -1;
    }
    hbuf->len = sizeof(ADC_PARM_T);
    cJSON_Delete(root);

    return 0;
}

static int __product_test_encode_r_adc(PT_HBUF_T *hbuf,char *out)
{
    ADC_PARM_T *parm = (ADC_PARM_T*) (hbuf->data);

    TB_LOGD();
    if(parm->type == E_SENSOR_LIGHT){
        sprintf(out,"{\"type\":\"light\",\"index\":\"%d\",\"ret\":false}",parm->channel);
    }
    else if(parm->type == E_SENSOR_ADC) 
    {
        sprintf(out,"{\"type\":\"adc\",\"index\":\"%d\",\"ret\":false}",parm->channel);
    }
    if(hbuf->result == E_FALSE) {
        TB_LOGE("read fail");
        return -1;
    }
    if(parm->type == E_SENSOR_LIGHT){
        sprintf(out,"{\"type\":\"light\",\"index\":%d,\"val\":%d,\"ret\":true}",parm->channel,parm->value);
    }
    else if(parm->type == E_SENSOR_ADC) {
        sprintf(out,"{\"type\":\"adc\",\"index\":%d,\"val\":%d,\"ret\":true}",parm->channel,parm->value);
    }

    return 0;

}

static int __product_test_decode_aging_test(PT_CMD_TB_T *cmd,PT_GEN_FRAME_T *frame,PT_HBUF_T *hbuf)
{
    return __product_test_decode_common(cmd,frame,hbuf);
}

static int __product_test_encode_aging_test(PT_HBUF_T *hbuf,char *out)
{
    return __product_test_encode_common(hbuf,out);
}

static int __product_test_decode_ir_send_test(PT_CMD_TB_T *cmd,PT_GEN_FRAME_T *frame,PT_HBUF_T *hbuf)
{
    if(cmd == NULL || frame == NULL || hbuf == NULL) {
        TB_LOGE("invalid pram");
        return -1;  
    }

    hbuf->len = frame->vdlen;
    hbuf->result = E_FALSE;
    hbuf->main_type = cmd->hwl_mtype;
    hbuf->sub_type = cmd->hwl_stype;
    memcpy(hbuf->data,frame->data,frame->vdlen);
    return 0;
}

static int __product_test_encode_ir_send_test(PT_HBUF_T *hbuf,char *out)
{
    return __product_test_encode_common(hbuf,out);
}


static int __product_test_decode_ir_recv_test(PT_CMD_TB_T *cmd,PT_GEN_FRAME_T *frame,PT_HBUF_T *hbuf)
{

    if(cmd == NULL || frame == NULL || hbuf == NULL) {
        TB_LOGE("invalid pram");
        return -1;  
    }

    hbuf->len = frame->vdlen;
    hbuf->result = E_FALSE;
    hbuf->main_type = cmd->hwl_mtype;
    hbuf->sub_type = cmd->hwl_stype;
    memcpy(hbuf->data,frame->data,frame->vdlen);
    return 0;
}

static int __product_test_encode_ir_recv_test(PT_HBUF_T *hbuf,char *out)
{
    return __product_test_encode_common(hbuf,out);
}

static int __product_test_decode_ir_mode_set(PT_CMD_TB_T *cmd,PT_GEN_FRAME_T *frame,PT_HBUF_T *hbuf)
{
    int iret =  0;
    cJSON *root;
    cJSON *mode; 
    int *ir_mode;

    if(cmd == NULL || frame == NULL || hbuf == NULL) {
        TB_LOGE("invalid pram");
        return -1;  
    }
    
    hbuf->len = 0;
    
    hbuf->result = E_FALSE;
    hbuf->main_type = cmd->hwl_mtype;
    hbuf->sub_type = cmd->hwl_stype;
    
    ir_mode = (int*)(hbuf->data);
    
    if(frame->data == NULL) {
        TB_LOGE("frame data is NULL");
        return -1;
    }
        
    root = cJSON_Parse(frame->data);
    if(root == NULL) {
        TB_LOGE("cJSON_Parse err");
        return -1;
    }
    
    mode = cJSON_GetObjectItem(root,"mode");
    if(mode == NULL) {
        TB_LOGE("can't find mode ");
        cJSON_Delete(root);
        return -1;
    }
    
    if(strcmp(mode->valuestring,"emission") == 0) {
        *ir_mode  = E_IR_MODE_EMISSION;                       
    }else if(strcmp(mode->valuestring,"receive") == 0) {
        *ir_mode  = E_IR_MODE_RECEIVE;           
    }else {
        iret = -1;    
    }
    cJSON_Delete(root);
    
    return iret;
}

static int __product_test_encode_ir_mode_set(PT_HBUF_T *hbuf,char *out)
{
    return __product_test_encode_common(hbuf,out);
}

static int __product_test_decode_gdl_start(PT_CMD_TB_T *cmd,PT_GEN_FRAME_T *frame,PT_HBUF_T *hbuf)
{
    unsigned int size;
    char * ftype = NULL;
    char *md5=NULL;
    cJSON *root;
    cJSON *cjson_type;
    cJSON *cjson_size;
    cJSON *cjson_md5;
    GDL_PRAM_T *pgdl;
        
    if(cmd == NULL || frame == NULL || hbuf == NULL) {
        TB_LOGE("invalid pram");
        return -1;  
    }
    
    hbuf->len = 0;
    
    hbuf->result = E_FALSE;
    hbuf->main_type = cmd->hwl_mtype;
    hbuf->sub_type = cmd->hwl_stype;
    
    pgdl = (GDL_PRAM_T*)(hbuf->data);
    
    if(frame->data == NULL) {
        TB_LOGE("frame data is NULL");
        return -1;
    }
        
    root = cJSON_Parse(frame->data);
    if(root == NULL) {
        TB_LOGE("cJSON_Parse err");
        return -1;
    }

    cjson_type = cJSON_GetObjectItem(root,"fileType");
    
    if(cjson_type == NULL) {
        TB_LOGE("can't find file type");
        cJSON_Delete(root);
        return -1;
    }else {
        ftype = cjson_type->valuestring; 
    }
    
    cjson_size = cJSON_GetObjectItem(root,"size");
    
    if(cjson_size == NULL) {
        TB_LOGE("can't find size");
        cJSON_Delete(root);
        return -1;
    }

    size = cjson_size->valueint;
    
    cjson_md5 = cJSON_GetObjectItem(root,"md5");
    
    if(cjson_md5 == NULL) {
        TB_LOGE("can't find md5");
        cJSON_Delete(root);
        return -1;
    }
    
    md5 = cjson_md5->valuestring;

    strcpy(pgdl->type,ftype);
    strcpy(pgdl->md5,md5);
    pgdl->size = size;

    cJSON_Delete(root);

    return 0;
}

static int __product_test_encode_gdl_start(PT_HBUF_T *hbuf,char *out)
{
    return __product_test_encode_common(hbuf,out);
}

static int __product_test_decode_gdl_doing(PT_CMD_TB_T *cmd,PT_GEN_FRAME_T *frame,PT_HBUF_T *hbuf)
{
        
    if(cmd == NULL || frame == NULL || hbuf == NULL) {
        TB_LOGE("invalid pram");
        return -1;  
    }

    hbuf->len = frame->vdlen;
    hbuf->result = E_FALSE;
    hbuf->main_type = cmd->hwl_mtype;
    hbuf->sub_type = cmd->hwl_stype;
    memcpy(hbuf->data,frame->data,frame->vdlen);

    return 0;
}

static int __product_test_encode_gdl_doing(PT_HBUF_T *hbuf,char *out)
{
    return __product_test_encode_common(hbuf,out);
}

static int __product_test_decode_gdl_end(PT_CMD_TB_T *cmd,PT_GEN_FRAME_T *frame,PT_HBUF_T *hbuf)
{
    return __product_test_decode_common(cmd,frame,hbuf);
}

static int __product_test_encode_gdl_end(PT_HBUF_T *hbuf,char *out)
{
    return __product_test_encode_common(hbuf,out);
}

static int __product_test_decode_general_switch_test(PT_CMD_TB_T *cmd,PT_GEN_FRAME_T *frame,PT_HBUF_T *hbuf)
{
    cJSON *root;
    cJSON *ctype;
    cJSON *cstat;
    int *state;
    GEN_SWITCH_PARM_T *parm;

    if(cmd == NULL || frame == NULL || hbuf == NULL) {
        TB_LOGE("invalid pram");
        return -1;  
    }
        
    hbuf->len = 0;
    hbuf->result = E_FALSE;
    hbuf->main_type = cmd->hwl_mtype;
    hbuf->sub_type = cmd->hwl_stype;
    parm  = (GEN_SWITCH_PARM_T *)hbuf->data;
    if(frame->data == NULL) {
        TB_LOGE("frame data is NULL");
        return -1;
    }
    
    root = cJSON_Parse(frame->data);
    
    if(root == NULL) {
        TB_LOGE("cJSON_Parse err");
        return -1;
    }  
    
    ctype= cJSON_GetObjectItem(root,"type");
    
    if(ctype == NULL) {
        TB_LOGE("can't find type");
        cJSON_Delete(root);
            return -1;
        }

    if(strcmp(ctype->valuestring,"relay")==0) {
        parm->type = E_GEN_SWITCH_RELAY;
    }else {
        TB_LOGE("type:%s is invalid",ctype->valuestring);
        cJSON_Delete(root);
        return -1;  
    }

    cstat = cJSON_GetObjectItem(root,"stat");

    if(cstat == NULL) {
        TB_LOGE("can't find stat");
        cJSON_Delete(root);
            return -1;
        }

    if(strcmp(cstat->valuestring,"on") == 0) {
        parm->state = E_LED_ALL_ON;
    }else if(strcmp(cstat->valuestring,"off") == 0) {
        parm->state = E_LED_ALL_OFF;
    }else if(strcmp(cstat->valuestring,"flash") == 0) {
        parm->state = E_LED_ALL_FLASH;
    }else {
        TB_LOGE("state: %s invalid",cstat->valuestring);
        cJSON_Delete(root);
            return -1;
    }

    hbuf->len = sizeof(*state);
    cJSON_Delete(root);

    return 0;
}

static int __product_test_encode_general_switch_test(PT_HBUF_T *hbuf,char *out)
{
    return __product_test_encode_common(hbuf,out);
}


static int __product_test_decode_gpio_test(PT_CMD_TB_T *cmd,PT_GEN_FRAME_T *frame,PT_HBUF_T *hbuf)
{
    TB_LOGD();
    return __product_test_decode_common(cmd,frame,hbuf);
}

static int __product_test_encode_gpio_test(PT_HBUF_T *hbuf,char *out)
{
    int iret = 0;
    GPIO_GROUP_T *pg;

    TB_LOGD();
    
    if(hbuf == NULL || out == NULL) {
        TB_LOGE("invalid parm");
        return -1;
    }

    if(hbuf->result == E_TRUE) {
        return 0;
    }

    pg = (GPIO_GROUP_T*)(hbuf->data);

    if(pg->num == 2) {
        sprintf(out, "{\"ret\":false,\"G\":[\"%d,%d\"]}",pg->gpio[0].i,pg->gpio[1].i);  
    }else if(pg->num == 3) {
        sprintf(out,"{\"ret\":false,\"G\":[\"%d,%d\",\"%d,%d\"]}",pg->gpio[0].i,pg->gpio[1].i,pg->gpio[0].i,pg->gpio[2].i);
    } else if((pg->num == 5)) {
        sprintf(out,"{\"ret\":false,\"G\":[\"%d\"]}",pg->gpio[0].i);
    }else if((pg->num == 6)) {
        sprintf(out,"{\"ret\":false,\"G\":[\"%d\"]}",pg->gpio[0].i);
	} else {
      iret = -1; 
    }

    return iret;
}

static int __product_test_decode_mic_test(PT_CMD_TB_T *cmd,PT_GEN_FRAME_T *frame,PT_HBUF_T *hbuf)
{
    cJSON *root;
    cJSON *speaker;
    int *mode,tmp_val;
    
    if(cmd == NULL || frame == NULL || hbuf == NULL) {
        TB_LOGE("invalid pram");
        return -1;  
    }
    hbuf->len = 0;
    hbuf->result = E_FALSE;
    hbuf->main_type = cmd->hwl_mtype;
    hbuf->sub_type = cmd->hwl_stype;
    mode = (int*)(hbuf->data);
    if(frame->data == NULL) {
        TB_LOGE("frame data is NULL");
        return -1;
    }
    root = cJSON_Parse(frame->data);
    if(root == NULL) {
        TB_LOGE("cJSON_Parse err");
        return -1;
    }
    speaker = cJSON_GetObjectItem(root,"speaker");
    if(speaker == NULL) {
        TB_LOGE("can't find speaker");
        cJSON_Delete(root);
        return -1;
    }
    tmp_val= speaker->valueint;
    if (tmp_val == 1){
        *mode = E_MIC_START;
    }
    else{
        *mode = E_MIC_STOP;  
    }

    TB_LOGD("mode:%d,tmp_val:%d",*mode,tmp_val);

    hbuf->len =  sizeof(int);

    cJSON_Delete(root);
    return 0;
}

static int __product_test_encode_mic_test(PT_HBUF_T *hbuf,char *out)
{
    TB_LOGD("hbuf:%p,result:%d",hbuf,hbuf->result);
    return __product_test_encode_common(hbuf,out);
}

typedef struct boot_info_s {
    char *name;
    int type;
} BOOT_INFO_T;

static BOOT_INFO_T  sg_bool_testitems[] = {
	{"limit_switch",			E_BOOL_LIMIT_SWITCH},
	{"alarm_in",				E_BOOL_ALARM_IN},
	{"alarm_flash",				E_BOOL_ALARM_FLASH},
	{"alarm_off",				E_BOOL_ALARM_OFF},
	{"ddr",						E_BOOL_DDR},
	{"rfFlag",					E_BOOL_RFFLAG},
	{"light_white_flash",		E_BOOL_LIGHT_WHITE_FLASH},
	{"light_white_off",			E_BOOL_LIGHT_WHITE_OFF},
	{"auto_mic_on",				E_BOOL_AUTO_MIC_ON},
	{"auto_mic_off",			E_BOOL_AUTO_MIC_OFF},
	{"audio_start",				E_BOOL_AUDIO_START},
	{"swept_sine_start",		E_BOOL_SWEPT_SINE_START},
	{"swept_sine_over",			E_BOOL_SWEPT_SINE_OVER},
	{"read_swept_sine_result",	E_BOOL_READ_SWEPT_SINE_RESULT},
	{"light_color_auto",        E_BOOL_AUTO_LIGHT_COLOR},
};

static int __product_test_bool_find_type_by_name(const char *name,int* type) 
{
    int i,n;

    if(name == NULL || type == NULL) {
        TB_LOGE("invalid parm");
        return -1;	
    }

    n = sizeof(sg_bool_testitems)/sizeof(sg_bool_testitems[0]);

    for(i = 0; i < n; i++) {
        BOOT_INFO_T *p_bool = &sg_bool_testitems[i];
   	if(strcmp(name,p_bool->name) == 0) {
            *type = p_bool->type;
            return 0;	
	} 
    }
    return -1;
}

static int __product_test_bool_find_name_by_type(int type,char **name) 
{
    int i,n;

    if(type < 0 || type >= E_BOOL_MAX || name == NULL) {
        TB_LOGE("invalid parm");
        return -1;	
    }

    n = sizeof(sg_bool_testitems)/sizeof(sg_bool_testitems[0]);

    for(i = 0; i < n; i++) {
        BOOT_INFO_T *p_bool = &sg_bool_testitems[i];
   	if(type == p_bool->type) {
            *name = p_bool->name;
            return 0;	
	} 
    }

    return -1;
}

static int __product_test_decode_general_bool_test(PT_CMD_TB_T *cmd,PT_GEN_FRAME_T *frame,PT_HBUF_T *hbuf)
{
    cJSON *root;
    cJSON *testItem,*arrayItem,*item,*object;
    BOOL_PARM_T *parm;
    int size,i;
    
    if(cmd == NULL || frame == NULL || hbuf == NULL) {
        TB_LOGE("invalid pram");
        return -1;  
    }

    hbuf->len = 0;
    hbuf->result = E_FALSE;
    hbuf->main_type = cmd->hwl_mtype;
    hbuf->sub_type = cmd->hwl_stype;
    parm = (BOOL_PARM_T*)(hbuf->data);
    if(frame->data == NULL) {
        TB_LOGE("frame data is NULL");
        return -1;
    }
    root = cJSON_Parse(frame->data);
    if(root == NULL) {
        TB_LOGE("cJSON_Parse err");
        return -1;
    }

    testItem = cJSON_GetObjectItem(root,"testItem");

    if(testItem == NULL) {
        TB_LOGE("can't find testItem");
        cJSON_Delete(root);
        return -1;
    }

    if(__product_test_bool_find_type_by_name(testItem->valuestring,&parm->type)) {
        TB_LOGE("invalid parm");
        cJSON_Delete(root);
        return -1;  
    } 

    arrayItem = cJSON_GetObjectItem(root,"paras");
    if(arrayItem != NULL) {
        size = cJSON_GetArraySize(arrayItem);
        if (size > MAX_PARM)
        {
            TB_LOGE("arrayItem too large");
            cJSON_Delete(root);
            return -1;
        }
        parm->num = size;
        TB_LOGD("cJSON_GetArraySize: size=%d\n",size);
        for(i = 0; i < size; i++){
            printf("i=%d\n",i);
            object = cJSON_GetArrayItem(arrayItem,i);
            item = cJSON_GetObjectItem(object,"key");
            if(item == NULL){
                TB_LOGE("can't find Item");
                cJSON_Delete(root);
                return -1;
            }
            memcpy(parm->key[i],item->valuestring,strlen(item->valuestring));
            item = cJSON_GetObjectItem(object,"value");
            if(item == NULL){
                TB_LOGE("can't find Item");
                cJSON_Delete(root);
                return -1;
            }
            memcpy(parm->value[i],item->valuestring,strlen(item->valuestring));
        }
        TB_LOGD("parm  type:%d  %p",parm->type,parm);
    }

    hbuf->len =  sizeof(BOOL_PARM_T);

    TB_LOGD("bool hbuf->len:%d",hbuf->len);
    cJSON_Delete(root);
    return 0;
}

static int __product_test_encode_general_bool_test(PT_HBUF_T *hbuf,char *out)
{
    int iret = -1;
    BOOL_PARM_T *parm;
    char *test_item;
    char *err_code = "530002";
    char *err_msg = "error";

    if(hbuf == NULL || out == NULL) {
        TB_LOGE("invalid parm");
        return -1;
    }
    
    parm = (BOOL_PARM_T*)(hbuf->data);

    if(__product_test_bool_find_name_by_type(parm->type,&test_item)) {
        TB_LOGE("invalid type:%d",parm->type);
        return -1;  
    }

    if(parm->type == E_BOOL_LIMIT_SWITCH) {
        if(parm->value[0][0] != 1) {
            err_code = "530000";
            err_msg = "left error";
            hbuf->result = E_FALSE; 
        }else if(parm->value[0][1] != 1) {
            err_code = "530001";
            err_msg = "right error";
            hbuf->result = E_FALSE; 
        }
    } 

    if(hbuf->result == E_TRUE) {
        sprintf(out,"{\"testItem\":\"%s\",\"ret\":true}",test_item);
        iret  = 0;
    }else {
        sprintf(out,"{\"testItem\":\"%s\",\"ret\":false,\"errCode\":\"%s\",\"errMsg\":\"%s\"}",test_item,err_code,err_msg);
    }
    return iret;
}

static PT_CMD_TB_T sg_cmd_tbs[] = {
    {0,E_PT_GEN_MODE,E_TYPE_PRODUCT_INFO,E_INFO_PROD_INFO,__product_test_encode_enter_prod,__product_test_decode_enter_prod},
    {1,E_PT_GEN_R_MASTER_FIRMWARE,E_TYPE_PRODUCT_INFO,E_INFO_MFIRM_INFO,__product_test_encode_r_mfirm,__product_test_decode_r_mfirm},
    {2,E_PT_GEN_R_SLAVE_FIRMWARE,E_TYPE_PRODUCT_INFO,E_INFO_SFIRM_INFO,__product_test_encode_r_sfirm,__product_test_decode_r_sfirm},
    {3,E_PT_GEN_W_CFGINFO,E_TYPE_CFG_INFO,E_CFG_WRITE_CFG,__product_test_encode_w_cfg,__product_test_decode_w_cfg},
    {4,E_PT_GEN_R_CFGINFO,E_TYPE_CFG_INFO,E_CFG_READ_CFG,__product_test_encode_r_cfg,__product_test_decode_r_cfg},
    {5,E_PT_GEN_W_MASTER_MAC,E_TYPE_CFG_INFO,E_CFG_WRITE_MMAC,__product_test_encode_w_mmac,__product_test_decode_w_mmac},
    {6,E_PT_GEN_R_MASTER_MAC,E_TYPE_CFG_INFO,E_CFG_READ_MMAC,__product_test_encode_r_mmac,__product_test_decode_r_mmac},
    {7,E_PT_GEN_W_SLAVE_MAC,E_TYPE_CFG_INFO,E_CFG_WRITE_SMAC,__product_test_encode_w_smac,__product_test_decode_w_smac},
    {8,E_PT_GEN_R_SLAVE_MAC,E_TYPE_CFG_INFO,E_CFG_READ_SMAC,__product_test_encode_r_smac,__product_test_decode_r_smac},
    {9,E_PT_GEN_SELF_CHECK,E_TYPE_SELF_CHECK,E_GEN_TEST,__product_test_encode_self_check,__product_test_decode_self_check},
    {10,E_PT_GEN_BUTTON_TEST,E_TYPE_KEY,E_GEN_TEST,__product_test_encode_r_key,__product_test_decode_r_key},
    {11,E_PT_GEN_LED_TEST,E_TYPE_LED,E_GEN_TEST,__product_test_encode_w_led,__product_test_decode_w_led},
    {12,E_PT_GEN_W_BSN,E_TYPE_CFG_INFO,E_CFG_WRITE_BSN,__product_test_encode_w_bsn,__product_test_decode_w_bsn},
    {13,E_PT_GEN_R_BSN,E_TYPE_CFG_INFO,E_CFG_READ_BSN,__product_test_encode_r_bsn,__product_test_decode_r_bsn},
    {14,E_PT_GEN_W_SN,E_TYPE_CFG_INFO,E_CFG_WRITE_SN,__product_test_encode_w_sn,__product_test_decode_w_sn},
    {15,E_PT_GEN_R_SN,E_TYPE_CFG_INFO,E_CFG_READ_SN,__product_test_encode_r_sn,__product_test_decode_r_sn},
    {16,E_PT_GEN_MASTER_CAP,E_TYPE_WIFI,E_WIFI_R_RSSI,__product_test_encode_mcap_test,__product_test_decode_mcap_test},
    {17,E_PT_GEN_SLAVE_CAP,E_TYPE_SCAP,E_GEN_TEST,__product_test_encode_scap_test,__product_test_decode_scap_test},
    {18,E_PT_GEN_WIFI_SPEED,E_TYPE_WIFI,E_WIFI_IPERF_TEST,__product_test_encode_wifi_test,__product_test_decode_wifi_test},
    {19,E_PT_GEN_CAMERA,E_TYPE_VIDEO,E_VIDEO,__product_test_encode_video_test,__product_test_decode_video_test},
    {20,E_PT_GEN_IRCUT,E_TYPE_IRCUT,E_GEN_TEST,__product_test_encode_ircut_test,__product_test_decode_ircut_test}, 
    {21,E_PT_GEN_SPEAKER,E_TYPE_SPEAKER,E_GEN_TEST,__product_test_encode_speaker_test,__product_test_decode_speaker_test},
    {22,E_PT_GEN_MIC,E_TYPE_MIC,E_GEN_TEST,__product_test_encode_mic_test,__product_test_decode_mic_test}, 
    {23,E_PT_GEN_IRLED,E_TYPE_IRLED,E_GEN_TEST,__product_test_encode_ir_led_test,__product_test_decode_ir_led_test},
    {24,E_PT_GEN_MODE,0,0,NULL,NULL},        
    {25,E_PT_GEN_MODE,0,0,NULL,NULL},
    {26,E_PT_GEN_BLACK_CAMERA,E_TYPE_VIDEO,E_VIDEO_BLACK,__product_test_encode_black_video_test,__product_test_decode_black_video_test},
    {27,E_PT_GEN_MOTOR,E_TYPE_MOTOR,E_MOTOR_SELF_CHECK,__product_test_encode_motor_test,__product_test_decode_motor_test},
    {28,E_PT_GEN_W_COUNTRY,E_TYPE_CFG_INFO,E_CFG_WRITE_COUNTRY,__product_test_encode_w_country,__product_test_decode_w_country},       
    {29,E_PT_GEN_R_COUNTRY,E_TYPE_CFG_INFO,E_CFG_READ_COUNTRY,__product_test_encode_r_country,__product_test_decode_r_country},
    {30,E_PT_GEN_UPGRADE_CFG,E_TYPE_OTA,E_OTA_CFG,__product_test_encode_upgrade_cfg,__product_test_decode_upgrade_cfg},        
    {31,E_PT_GEN_UPGRADE_START,E_TYPE_OTA,E_OTA_START,__product_test_encode_upgrade_start,__product_test_decode_upgrade_start},
    {32,E_PT_GEN_UPGRADEING,E_TYPE_OTA,E_OTA_DLING,__product_test_encode_upgrade_recv,__product_test_decode_upgrade_recv},     
    {33,E_PT_GEN_UPGRADE_END,E_TYPE_OTA,E_OTA_END,__product_test_encode_upgrade_end,__product_test_decode_upgrade_end},
    {34,E_PT_GEN_REBOOT,E_TYPE_REBOOT,E_GEN_TEST,__product_test_encode_reboot,__product_test_decode_reboot},       
    {35,E_PT_GEN_SLAVE_UPGRADE,E_TYPE_SLAVE_OTA,E_GEN_TEST,__product_test_encode_upgrade_slave,__product_test_decode_upgrade_slave},
    {36,E_PT_GEN_MODE,0,0,NULL,NULL},
    {37,E_PT_GEN_PIR,E_TYPE_PIR,E_GEN_TEST,__product_test_encode_pir_test,__product_test_decode_pir_test},
    {38,E_PT_GEN_USB_TEST,E_TYPE_USB,E_GEN_TEST,__product_test_encode_usb_test,__product_test_decode_usb_test},
    {39,E_PT_GEN_MODE,0,0,NULL,NULL},
    {40,E_PT_GEN_MODE,0,0,NULL,NULL},
    {41,E_PT_GEN_LTE_TEST,E_TYPE_MODEM,E_MODEM_ENTER_TEST,__product_test_encode_modem_test,__product_test_decode_modem_test},
    {42,E_PT_GEN_FLAG_DECTECT,E_TYPE_FLAG_DECT,E_GEN_TEST,__product_test_encode_flag_dect,__product_test_decode_flag_dect},
    {43,E_PT_GEN_LTE_READ_CCID,E_TYPE_MODEM,E_MODEM_READ_CCID,__product_test_encode_r_ccid,__product_test_decode_r_ccid},
    {44,E_PT_GEN_POWER,E_TYPE_BATTERY,E_BATTERY_GET_VALUE,__product_test_encode_r_power,__product_test_decode_r_power},
    {45,E_PT_GEN_ANALOG_SENSOR,E_TYPE_ADC,E_GEN_TEST,__product_test_encode_r_adc,__product_test_decode_r_adc},
    {46,E_PT_GEN_LTE_READ_IMSI,E_TYPE_MODEM,E_MODEM_READ_IMSI,__product_test_encode_r_imsi,__product_test_decode_r_imsi},
    {47,E_PT_GEN_LTE_READ_IMEI,E_TYPE_MODEM,E_MODEM_READ_IMEI,__product_test_encode_r_imei,__product_test_decode_r_imei},
    {48,E_PT_GEN_MODE,0,0,NULL,NULL},
    {49,E_PT_GEN_MODE,0,0,NULL,NULL},
    {50,E_PT_GEN_MODE,0,0,NULL,NULL},
    {51,E_PT_GEN_AGEING_TEST,E_TYPE_AGING,E_GEN_TEST,__product_test_encode_aging_test,__product_test_decode_aging_test},
    {52,E_PT_GEN_MODE,0,0,NULL,NULL},
    {53,E_PT_GEN_MODE,0,0,NULL,NULL},
    {54,E_PT_GEN_MODE,0,0,NULL,NULL},
    {55,E_PT_GEN_MODE,0,0,NULL,NULL},
    {56,E_PT_GEN_W_BATTERY_INFO,E_TYPE_BATTERY,E_BATTERY_SET_INFO,__product_test_encode_w_battery_info,__product_test_decode_w_battery_info},
    {57,E_PT_GEN_R_BATTERY_INFO,E_TYPE_BATTERY,E_BATTERY_GET_INFO,__product_test_encode_r_battery_info,__product_test_decode_r_battery_info},
    {58,E_PT_GEN_W_WIFI_CFG,E_TYPE_WIFI,E_WIFI_CONNECT_AP,__product_test_encode_w_wifi_cfg,__product_test_decode_w_wifi_cfg},
    {59,E_PT_GEN_LTE_READ_RSSI,E_TYPE_MODEM,E_MODEM_READ_RSSI,__product_test_encode_r_rssi,__product_test_decode_r_rssi},
    {60,E_PT_GEN_IR_SEND,E_TYPE_IR,E_IR_SEND,__product_test_encode_ir_send_test,__product_test_decode_ir_send_test},
    {61,E_PT_GEN_IR_RECV,E_TYPE_IR,E_IR_RECV,__product_test_encode_ir_recv_test,__product_test_decode_ir_recv_test},
    {62,E_PT_GEN_IR_MODE,E_TYPE_IR,E_IR_MODE,__product_test_encode_ir_mode_set,__product_test_decode_ir_mode_set},
    {63,E_PT_GEN_MODE,0,0,NULL,NULL},
    {64,E_PT_GEN_GEN_DL_START,E_TYPE_GDL,E_GDL_START,__product_test_encode_gdl_start,__product_test_decode_gdl_start},
    {65,E_PT_GEN_GEN_DL,E_TYPE_GDL,E_GDL_DOING,__product_test_encode_gdl_doing,__product_test_decode_gdl_doing},
    {66,E_PT_GEN_GEN_DL_END,E_TYPE_GDL,E_GDL_END,__product_test_encode_gdl_end,__product_test_decode_gdl_end},
    {67,E_PT_GEN_SWITCH_TEST,E_TYPE_GEN_SWITCH,E_GEN_TEST,__product_test_encode_general_switch_test,__product_test_decode_general_switch_test},
    {68,E_PT_GEN_MODE,0,0,NULL,NULL},
    {69,E_PT_GEN_MODE,0,0,NULL,NULL},
    {70,E_PT_GEN_GPIO_TEST,E_TYPE_GPIO,E_GEN_TEST,__product_test_encode_gpio_test,__product_test_decode_gpio_test},
    {71,E_PT_GEN_BOOL_TEST,E_TYPE_BOOL_TEST,E_GEN_TEST,__product_test_encode_general_bool_test,__product_test_decode_general_bool_test},
    {72,E_PT_GEN_MAX,0,0,NULL,NULL},
};

static int __gen_get_valid_frame(unsigned char *buf,int len,PT_GEN_FRAME_T *frame)
{
    unsigned int tlen;
    unsigned int dlen;

    if(buf == NULL || frame == NULL) {
        TB_LOGE("invalid parm");
        return -1;
    }

    if(buf[0] !=  PT_GEN_HEAD_0 || 
        buf[1] != PT_GEN_HEAD_1) {
        TB_LOGE("invalid head");
        return -1;
    }

    dlen = (buf[4]<<8) + buf[5];
    tlen = dlen + PT_GEN_MIN_SIZE;

    if(len <  tlen) {
        TB_LOGE("data (%d) < ptlen(%d)",len,tlen);
        return -1;
    }

#if 0
    for(i=0; i<tlen-1; i++)
        csum += buf[i];

    csum %= 256; 

    if(csum != buf[tlen-1]) {
        TB_LOGE("csum err%x != %x",csum,buf[tlen-1]);
        return -1;
    }
#endif
    
    if(dlen == 0)
        frame->data = NULL;
    else
        frame->data = (char *)&buf[6];

    frame->hd[0] = buf[0];
    frame->hd[1] = buf[1];
    frame->ver[0] = buf[2];
    frame->cmd[0] = buf[3];
    frame->dlen[0] = buf[4];
    frame->dlen[1] = buf[5];
    frame->csum[0] = buf[6+dlen];
    buf[6+dlen] = '\0';
    frame->vdlen = dlen;
    frame->buf = buf;
    frame->tlen = tlen; 

    return 0;
}

static int __gen_create_frame(unsigned char cmd, unsigned char *data,unsigned char *out, int *olen)
{
    int i=0;
    unsigned int csum=0;
    unsigned int dlen,flen;
    unsigned char *buf= out;
    int hlen= PT_GEN_MIN_SIZE-1;
    
    if(data != NULL) {
        dlen = strlen((char *)data);
        flen = dlen + PT_GEN_MIN_SIZE;
    }else {
        dlen = 0;
        flen = PT_GEN_MIN_SIZE;
    } 
    
    memset(buf,0,flen);

    buf[0] = PT_GEN_HEAD_0;
    buf[1] = PT_GEN_HEAD_1;
    buf[2] = PT_GEN_VER;
    buf[3] = cmd;
    
    if(data != NULL) {
        buf[4] = (unsigned char)(dlen/256);
        buf[5] = (unsigned char)(dlen%256);
    }else{
        dlen = 0;
        buf[4] = 0x00;
        buf[5] = 0x00;
    }
    
    for(i=0;i<dlen;i++) {
        buf[hlen+i] = data[i];  
    }

    for(i=0;i<hlen+dlen; i++)
        csum+=buf[i];

    csum %= 256;
    buf[i] = csum;

    *olen = flen;
    
    return 0; 
}

static PT_CMD_TB_T * __get_tb_by_type(int main_type ,int sub_type) 
{
    int i,n;
    PT_CMD_TB_T *ptbl = &sg_cmd_tbs[0];
    
    n = sizeof(sg_cmd_tbs)/sizeof(sg_cmd_tbs[0]);


    for(i=0;i<n;i++) {
        if(ptbl->hwl_mtype == main_type && 
        ptbl->hwl_stype == sub_type) {
            break;
        }   
        ptbl ++;    
    }

    TB_LOGD("main_type:%d,sub_type:%d,index:%d\n",main_type,sub_type,i);

    if(i >= n) {
        TB_LOGE(" tbl is not found");
        return NULL;    
    
    }


    return ptbl;

}

static PT_CMD_TB_T * __get_tb_by_cmd(int cmd) 
{
    PT_CMD_TB_T *ptbl = &sg_cmd_tbs[0];
    
    if(cmd <0 || cmd > E_PT_GEN_MAX) {
        TB_LOGE("invalid parm");
        return NULL;
    }

    if(cmd != ptbl[cmd].index || cmd != ptbl[cmd].pt_type) {
        TB_LOGE("cmd(%d),index(%d),type(%d)",
                cmd,ptbl[cmd].index,
                ptbl[cmd].pt_type);
        return NULL;
    }
    
    return &ptbl[cmd];
}

int product_test_pt_gen_complete(unsigned char *buf,int len) 
{
    int i;
    unsigned int tlen;
    unsigned int dlen;
    unsigned int csum=0;

    if(buf == NULL || len  == 0) {
        TB_LOGE("invalid parm");
        return 0;
    }

    if(buf[0] !=  PT_GEN_HEAD_0 || 
        buf[1] != PT_GEN_HEAD_1) {
        TB_LOGD("invalid head");
        return 0;  
    }

    dlen = (buf[4]<<8) + buf[5];
    tlen = dlen + PT_GEN_MIN_SIZE;

    if(len <  tlen) {
        TB_LOGD("data (%d) < ptlen(%d)",len,tlen);
        return 0;
    }

    for(i=0; i<tlen-1; i++)
        csum += buf[i];

    csum %= 256; 

    if(csum != buf[tlen-1]) {
        TB_LOGE("csum err%x != %x",csum,buf[tlen-1]);
        return 0;
    }

    return 1;    
}

int product_test_pt_gen_decode(PT_FBUF_T *src,PT_HBUF_T *dst)
{
    int len;
    uint8_t *buf;
    int iret = -1;
    PT_CMD_TB_T *tbl;
    PT_GEN_FRAME_T frame;

    TB_LOGD();

    if(src == NULL || dst == NULL) {
        TB_LOGE("invalid parm");
        return -1;
    }

    buf = src->buf;
    len = src->len;

    memset(&frame,0,sizeof(PT_GEN_FRAME_T));

    if(__gen_get_valid_frame(buf,len,&frame)) {
        TB_LOGE("get invalid frame\n");
        return -1;
    }

    if(frame.vdlen)
        TB_LOGV("decode:%s",frame.data);

    if(sg_start_prog_flag == 0 && frame.cmd[0] != PT_GEN_PROD_CMD) {
        TB_LOGE("current is not in product mode");
        return -1;
    }
        
    if((tbl= __get_tb_by_cmd(frame.cmd[0])) == NULL) {
        TB_LOGE("cmd(%d) haven't cb\n",frame.cmd[0]);
        return -1;
    }

    if(tbl->decode) {
        iret = tbl->decode(tbl,&frame,dst);
    }else {
        TB_LOGE("cmd %d decode func is not valid",frame.cmd[0]);
    }

    return iret;
}

int product_test_pt_gen_encode(PT_HBUF_T *src,PT_FBUF_T *dst)
{
    int len;
    int iret = -1;
    PT_CMD_TB_T *tbl;
    uint8_t *obuf; 
    uint8_t buf[PT_GEN_JSON_SIZE];

    TB_LOGD();
    if(src  == NULL || dst == NULL) {
        TB_LOGE("invalid parm");
        return -1;
    }
    
    obuf = dst->buf;
    memset(buf,0,PT_GEN_JSON_SIZE);

    if((tbl = __get_tb_by_type(src->main_type,src->sub_type)) == NULL) {
        TB_LOGE("get cmd fail");
        return -1;  
    }

    if(tbl->encode) {
        iret = tbl->encode(src,(char *)buf);
    }else {
        TB_LOGE("mtype:%d stype:%d encode func is not valid",src->main_type,src->sub_type);
        return -1;
    }

    if(iret == 0) {
        if(strlen((char *)buf) == 0) {
            snprintf((char *)buf,PT_GEN_JSON_SIZE,PT_GEN_TRUE_JSON);
        }
    }else {
        if(strlen((char *)buf) == 0) {
            snprintf((char *)buf,PT_GEN_JSON_SIZE,PT_GEN_FALSE_JSON);
        }
    }

    TB_LOGD("encode:%s",buf);

    if(__gen_create_frame(tbl->pt_type,buf,obuf,&len)) {
        TB_LOGE("ty gen create frame fail");
        return -1;
    }
    
    dst->len = len;

    return 0;
}
