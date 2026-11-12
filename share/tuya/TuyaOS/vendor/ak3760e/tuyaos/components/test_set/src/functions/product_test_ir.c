/*
 *  @copyright Copyright (c) tuya.inc 2021
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "tkl_ir.h"
#include "cJSON.h"


#define TY_IR_TEST_FEQ              38000
#define TY_IR_TEST_MAX              (1024*2)

#define IR_DATA_S   560
#define IR_DATA_0   560
#define IR_DATA_1   1680

#define IR_CMD_START_1    9000
#define IR_CMD_START_2    4500

#define IR_CMD_STOP_1     9000
#define IR_CMD_STOP_2     2200

#define IR_CMD_STOP_3     560
#define IR_CMD_STOP_4     50000

#define IR_TIME_ALW       250

static const unsigned short  sg_ir_cmd_start[] = {IR_CMD_START_1, IR_CMD_START_2};
static const unsigned short  sg_ir_cmd_stop[]  = {IR_CMD_STOP_1, IR_CMD_STOP_2, IR_CMD_STOP_3, IR_CMD_STOP_4};



static int __product_test_ir_send_proc(unsigned short *ir_time_data, unsigned short data_len)
{

    int  ret;
    int i;

    TY_IR_BUF_T ir_data_s;

    printf("%s data_len: %d \n", __func__, data_len);
    
    memset(&ir_data_s,0,sizeof(TY_IR_BUF_T));

    ir_data_s.buf = malloc(sizeof(int)*TY_IR_TEST_MAX);
    ir_data_s.cnts = data_len;
    ir_data_s.size = sizeof(int)*TY_IR_TEST_MAX;

    if(NULL == ir_data_s.buf) {
        printf("Malloc ir_data_s.codelist failed!\n");
        return -1;
    }
    memset(ir_data_s.buf, 0, ir_data_s.size);

    for (i=0; i<data_len; i++) {
        ir_data_s.buf[i] = ir_time_data[i];
    }
    if (0 == ir_data_s.cnts%2) {
        ir_data_s.buf[ir_data_s.cnts++] = 10;
    }

#if 1
    for (i=0; i<ir_data_s.cnts; i++) {
        printf("%d ", ir_data_s.buf[i]);
    }
#endif

    printf("\r\n");

    ret = tkl_ir_set_tx_carrier(IR_ID_1, TY_IR_TEST_FEQ);
    
    printf("hal_ir_set_tx_carrier ret = %d\n", ret);
    
    if(tkl_ir_send(IR_ID_1,&ir_data_s) < 0) {
        ret = -1; 
    }

    free(ir_data_s.buf);

    printf("%s ret = %d\n", __func__, ret);

    return ret;
}

static unsigned short __product_test_ir_send_pkg(char  *str,  unsigned short  *trsdata,  unsigned short str_len)
{
    unsigned short  trsdata_len;
    int  i, j;

    if (str == NULL) {
        return -1;
    }

    //ir_start_cmd
    memcpy(trsdata, sg_ir_cmd_start, 4);
    trsdata += 2;

    //ir_data_cmd
    for (i = 0; i < str_len; i++) {
        for (j = 0; j < 8; j++) {
            *trsdata = IR_DATA_S;
            if (((*str >> j) & 0x01) == 1) {
                *(trsdata + 1) = IR_DATA_1;
            } else {
                *(trsdata + 1) = IR_DATA_0;
            }
            trsdata += 2;
        }
        str++;
    }

    //ir_stop_cmd
    memcpy(trsdata, sg_ir_cmd_stop, 8);
    trsdata_len = 2 + str_len * 8 * 2 + 4;

    return trsdata_len;
}

static int __product_test_ir_send_str(char  *ir_buf, int  ir_len)
{
    int  ret = -1;
    unsigned short  ir_time_len;
    unsigned short  *ir_time_buf;

    ir_time_buf = (unsigned short  *)malloc(TY_IR_TEST_MAX);
    if (NULL == ir_time_buf) {
        printf("malloc error\n");
        return -1;
    }
    memset(ir_time_buf, 0, TY_IR_TEST_MAX);

    printf("%s send_str:%s, len:%d\n", __func__, ir_buf, ir_len);
    
    ir_time_len = __product_test_ir_send_pkg(ir_buf, ir_time_buf, ir_len);
    ret  = __product_test_ir_send_proc(ir_time_buf, ir_time_len);

    free(ir_time_buf);

    return ret;
}

static int __compare_value(unsigned short value1, unsigned short  value2)
{
    if (value1 > value2) {
        if ((value1 - value2) < IR_TIME_ALW) {
            return 1;
        } else {
            return 0;
        }
    } else {
        if ((value2 - value1) < IR_TIME_ALW) {
            return 1;
        } else {
            return  0;
        }
    }
}

static  int __product_test_ir_rec_to_string(TY_IR_BUF_T *raw_code,char  *recv_buf, int  *buf_size)
{
    unsigned short ir_data_len;
    unsigned char  ir_data_buf = 0;
    unsigned char  i = 0, j;
    unsigned char  ir_rev_buf[32] = {0};
    unsigned char  ir_rev_len = 0;

    if (!raw_code) {
        printf("ir into error\n");
        return -1;
    }

    int  *trsdata = raw_code->buf;
    int  str_len = raw_code->cnts;
    
    while (!(__compare_value(*trsdata, IR_CMD_START_1) && __compare_value(*(trsdata + 1), IR_CMD_START_2))) {
        trsdata++;
        str_len--;

        if (str_len == 1) {
            printf("not found ir start cmd\n");
            return -1;
        }
    }

    trsdata += 2;
    ir_data_len = str_len - 2;

    while (ir_data_len / 16) {
        for (j = 0; j < 8; j++) {
            if ((__compare_value(*trsdata, IR_DATA_S) && __compare_value(*(trsdata + 1), IR_DATA_1))) {
                ir_data_buf |= (0x01 << j);
            } else if ((__compare_value(*trsdata, IR_DATA_S) && __compare_value(*(trsdata + 1), IR_DATA_0))) {
                ir_data_buf &= ~(0x01 << j);
            } else {
                goto end_data;
            }

            trsdata += 2;
            ir_data_len -= 2;
        }

        ir_rev_buf[i] = ir_data_buf;
        ir_data_buf = 0;
        i++;
    }

end_data:
    ir_rev_len = i;
    printf("rev_buf_len:%d\n", ir_rev_len);
    while (!((__compare_value(*trsdata, IR_CMD_STOP_1) && __compare_value(*(trsdata + 1), IR_CMD_STOP_2)))) {
        trsdata++;
        ir_data_len--;

        if (ir_data_len == 1) {
            printf("not found ir stop cmd1\n");
            return -1;
        }
    }

    if (__compare_value(*(trsdata + 2), IR_CMD_STOP_3)) {
        ir_data_buf |= (0x01 << j);
    } else {
        printf("not found ir stop cmd2\n");
        return -1;
    }
    memcpy(recv_buf, ir_rev_buf, ir_rev_len);
    *buf_size = ir_rev_len;
    return 0;
}

int product_test_ir_send(char *buf,int len) 
{
    int ret = -1;

    if (!buf  ||  (len <=  0)) {
        printf("%s err invalid parm\n", __func__);
        return -1;
    }

    if(tkl_ir_init(IR_ID_1)) {
        printf("%s err tkl_ir_init\n", __func__);
        return -1; 
    }

    ret = __product_test_ir_send_str(buf,len);

    tkl_ir_deinit(IR_ID_1);

    return ret;
}

int product_test_ir_receive(char *buf, int *len)
{
    int  ret = -1;
    TY_IR_BUF_T ir_data_s;

    if (!buf  || !len) {
        printf("%s err invalid parm\n", __func__);
        return -1;
    }

    printf("%s begin\n",__func__);
    
    
    ir_data_s.buf = malloc(sizeof(int)*TY_IR_TEST_MAX);
    if(NULL == ir_data_s.buf) {
        printf("%s Malloc failed.\n", __func__);
        return -1;
    }

    if(tkl_ir_init(IR_ID_0) < 0) {
        printf("tkl_ir_init fail\n");
	ret = -1;
        goto exit;
    }

    ir_data_s.cnts = 0;
    ir_data_s.size = sizeof(int)*TY_IR_TEST_MAX;
    memset(ir_data_s.buf, 0, ir_data_s.size);
    
    tkl_ir_set_tx_carrier(IR_ID_0, TY_IR_TEST_FEQ);
       

    if(tkl_ir_recv(IR_ID_0,&ir_data_s,20*1000) < 0) {
        printf("ir_rawdata_recv: hal_ir_rawcode_rx failed, errcode = %d\n", ret);
	ret = -1;
        goto exit;
    }

    if (ir_data_s.cnts < 6) {
        printf("hal_ir_rawcode_rx recv count=%d, is invailed\n", ir_data_s.cnts);
        ret = -1;
        goto exit;
    }
    __product_test_ir_rec_to_string(&ir_data_s, buf, len);
    
exit:
    if(NULL != ir_data_s.buf) {
        free(ir_data_s.buf);
        ir_data_s.buf = NULL;
    }

    printf("%s ret = %d\n", __func__, ret);
    tkl_ir_deinit(IR_ID_0);

    return ret;
}
