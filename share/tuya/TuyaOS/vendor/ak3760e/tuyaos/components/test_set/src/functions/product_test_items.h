/* 
 *  @file product_test_items.h
 *  @brief product_test_items module is supply hardware test interface
 *  @version 0.1
 *  @date 2021-05-07
 *
 *  @copyright Copyright (c) tuya.inc 2021
 *  
 */

#ifndef __PRODUCT_TEST_ITEMS_H__
#define __PRODUCT_TEST_ITEMS_H__

#ifdef __cplusplus
extern "C" {
#endif

/*@brief test command operation 
 *@param(in) type:subtype
 *@param(in/out)data:param buffer
 *@param(in/out)len: param len
 *@return 0:succeed,-1:fail
 */
typedef int (*CMD_OPS_T)(int type,void * data,int* len);

typedef struct items_tb_s
{
    int main_type; // main type
    CMD_OPS_T ops; // command operation
}ITEMS_TB_T;

typedef struct async_parm_s {
    int fd; // connect fd
    int type; // man type
    COM_SEND_T send; //send function pointer
    PT_ENCODE_T encode; //encode function pointer
} ASYNC_PARM_T;

/*@brief test command operation  
 *@param(in) async: async communication parameter,See also ASYNC_PARM_T
 *@param(in)hbuf: See also PT_HBUF_T
 *@param(out)fbuf: param len,See alse PT_FBUF_T
 *@return 0:succeed,-1:fail
 */

extern int product_test_run_ops(ASYNC_PARM_T *async,PT_HBUF_T *hbuf,PT_FBUF_T *fbuf);



#ifdef __cplusplus
}
#endif

#endif
