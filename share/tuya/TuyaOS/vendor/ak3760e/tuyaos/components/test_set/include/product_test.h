/* 
 *  @file product_test.h
 *  @brief product_test module is supply prodcut test interface
 *  @version 0.1
 *  @date 2021-05-07
 *
 *  @copyright Copyright (c) tuya.inc 2021
 *  
 */
#ifndef __PRODUCT_TEST_H__
#define __PRODUCT_TEST_H__

#include "product_test_protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

#define COM_MASK (0xffff)
#define PT_MASK (0xffff0000)

typedef enum  {
    E_COM_UART = (1 << 0), // uart communication
    E_COM_TCP = (1 << 1),  // tcp cummunication
}COM_TYPE_E;

/*@brief communication init 
 *@param(in) param:init param
 *@param(out)fd:communication fd
 *@return 0:succeed,-1:fail
 */
typedef int  (*COM_INIT_T)(void *param,int *fd); // communication init

/*@brief communication uninit 
 *@param(in)fd:communication fd
 *@return 0:succeed,-1:fail
 */
typedef void (*COM_UINIT_T)(int fd);

/*@brief communication receive datas 
 *@param(in)fd:communication fd
 *@param(out)buf:receive buffer
 *@param(in)size:receive buffer size 
 *@return > 0:receive byts;0:disconnect;<0:fail
 */
typedef int  (*COM_RECV_T)(int fd,char *buf,int size);

/*@brief communication send datas 
 *@param(in)fd:communication fd
 *@param(in)buf:send buffer
 *@param(in)size:send buffer size 
 *@return >= 0:send bytss;<0:fail
 */
typedef int  (*COM_SEND_T)(int fd,char *buf,int size);

/*@brief transfer test result format information to  protocol format information
 *@param(in)src:test result information,See also PT_HBUF_T
 *@param(out)dst:protocol format information,See also PT_FBUF_T
 *@return >= 0:send bytss;<0:fail
 */
typedef int  (*PT_ENCODE_T)(PT_HBUF_T *src,PT_FBUF_T *dst);

/*@brief transfer  to  protocol format information to test result
 *@param(in)src:protocol format information,See also PT_FBUF_T 
 *@param(out)dst:test result information,See also PT_HBUF_T
 *@return >= 0:send bytss;<0:fail
 */
typedef int  (*PT_DECODE_T)(PT_FBUF_T *src,PT_HBUF_T *dst);

/*@brief check frame complete 
 *@param(in) buf:frame buffer;
 *@param(in) len:frame len;
 *@return >= 0:send bytss;<0:fail
 */
typedef int  (*PT_COMPLETE_T)(unsigned char *buf,int len);

/*@brief enter production test callback function
 *@param(in) void
 *@return void
 */
typedef void (*ENTER_HOOK) (char *value);


typedef struct com_intface_s 
{
    COM_TYPE_E type; // communication type
    void *parm;      // communication param
    COM_INIT_T  init;// communication init
    COM_UINIT_T uinit;//communication uinit
    COM_RECV_T  recv; //communication receive
    COM_SEND_T  send; //communication send
}COM_INTFACE_T;


/*@brief malloc  hbuf with  size bytes of buffer size 
 *@param(in) size:sizeof of hbuf's buffer bytes,totol malloc(size + sizeof(PT_HBUF_T));
 *@param(out)hbuf:hbuf pointer 
 *@return 0:succeed,-1:fail
 */
extern int product_test_zalloc_hbuf(size_t size,PT_HBUF_T **hbuf);

/*@brief  free hbuf
 *@param(in)hbuf:hbuf pointer 
 *@return 0:succeed,-1:fail
 */
extern int product_test_free_hbuf(PT_HBUF_T *hbuf);

/*@brief malloc fbuf with  size bytes of buffer size 
 *@param(in) size:sizeof of fbuf's buffer bytes,totol malloc(size + sizeof(PT_FBUF_T));
 *@param(out)fbuf:fbuf pointer 
 *@return 0:succeed,-1:fail
 */
extern int product_test_zalloc_fbuf(size_t size,PT_FBUF_T **fbuf);

/*@brief  free fbuf
 *@param(in)fbuf:fbuf pointer 
 *@return 0:succeed,-1:fail
 */
extern int product_test_free_fbuf(PT_FBUF_T *fbuf);

/*@brief product set wait secords for product test exit  
 *@param(in) seconds:wait seconds
 *@return 0:succeed,-1:fail
 */
extern int product_test_set_wait_times(int seconds);

/*@brief product test enter  
 *@param(in) void 
 *@return 0:succeed,-1:fail
 */
extern int  product_test_enter(void);

/*@brief product test force exit  
 *@param(in) void 
 *@return 0:succeed,-1:fail
 */
extern void product_test_exit(void);

/*@brief product test wait timeout exit  
 *@param(in) void 
 *@return 0:succeed,-1:fail
 */
extern void product_test_wait_exit(void);

/*@brief product test register communication protocol 
 *@param(in) bsize:protocol buffer
 *@param(in) encode:protocol encode function,See also PT_ENCODE_T
 *@param(in) decode:protocol decode function,See also PT_DECODE_T
 *@param(in) decode:protocol complete function,See also PT_COMPLETE_T
 *@return 0:succeed,-1:fail
 */

extern int product_test_register_protocol(int bsize,PT_ENCODE_T encode,PT_DECODE_T decode,PT_COMPLETE_T complete);

/*@brief product test register communication interface 
 *@param(in) interface:communication interface,See alse COM_INTFACE_T
 *@return 0:succeed,-1:fail
 */
extern int product_test_register_communication_interface(COM_INTFACE_T *interface); 

/*@brief set product test log level,>=6 debug,debug,<2,error
 *@param(in) log:log level 
 *@return 0:succeed,-1:fail
 */
extern int product_test_set_log_level(int log);

/*@brief get product test log level,>=6 debug,debug,<2,error
 *@param(out) log:log level 
 *@return 0:succeed,-1:fail
 */
extern int product_test_get_log_level(int *log); 

/*@brief product test prepare work
 *@param(in) void 
 *@return 0:succeed,-1:fail
 */
extern int product_test_prepare(void);

/*@brief registering enter production test callback function
 *@param(in) void
 *@return 0:succeed,-1:fail
 */
extern int product_test_register_enter_hook(ENTER_HOOK hook);

#ifdef __cplusplus
}
#endif

#endif
