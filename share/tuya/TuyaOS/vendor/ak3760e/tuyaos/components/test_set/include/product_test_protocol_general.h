/* 
 *  @file product_test_protocol_general.h
 *  @brief product_test_protocol_general module is supply tuya embeded linux product test general protocol
 *  @version 0.1
 *  @date 2021-05-07
 *
 *  @copyright Copyright (c) tuya.inc 2021
 *  
 */

#ifndef __PRODUCT_TEST_PROTOCOL_GENERAL_H__
#define __PRODUCT_TEST_PROTOCOL_GENERAL_H__

#ifdef __cplusplus
extern "C" {
#endif
// protocol parameter,fit for all protocol  
#include "product_test_protocol.h"



/*@brief check product whether enter 
 *@return 1:entered 0:unentered
 */
extern int product_test_is_entered(void);

/*@brief  encode information from hardware information format to protocol information format 
 *@param(in) src:information in hardware format,See PT_HBUF_T
 *@param(out)dst:information in protocol format
 *@return 0:succeed,-1:fail
 */
extern int product_test_pt_gen_encode(PT_HBUF_T *src,PT_FBUF_T *dst);

/*@brief  decode information from protocol information format to hardware information format
 *@param(in)src: information in protocol format ,See PT_FBUF_T
 *@param(out)dst:information in hardware format,See PT_HBUF_T
 *@return 0:succeed,-1:fail
 */

extern int product_test_pt_gen_decode(PT_FBUF_T *src,PT_HBUF_T *dst);


/*@brief check frame complete
 *@param(in)buf:frame buffer 
 *@param(in)len:frame len
 *@return 1:succeed,0,fail
 */

extern int product_test_pt_gen_complete(unsigned char *buf,int len);

#ifdef __cplusplus
}
#endif

#endif
