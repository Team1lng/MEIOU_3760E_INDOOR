/* 
 *  @file interface_uart.h
 *  @brief interface_uart module is used to product test 
 *  communicate with  pc  product software
 *  @version 0.1
 *  @date 2021-05-07
 *
 *  @copyright Copyright (c) tuya.inc 2021
 *  
 */
#ifndef __INTERFACE_UART_H__
#define __INTERFACE_UART_H__

#ifdef __cplusplus
extern "C" {
#endif


/*@brief  uart initialize
 *@param(in) name: uart device name
 *@param(in) baud: uart baud rate
 *@param(out) fd  : uart open fd
 *@return 0:succeed,-1:fail
 */
extern int interface_uart_init(char *name,int baud,int *fd);


/* @brief uart uninitialize
 * @param(in) fd: uart fd 
 * @returns void:none
 */
extern void interface_uart_uinit(int fd);

/* @brief uart send data
 * @param(in) fd: uart fd 
 * @param(in) buf: uart send buffer 
 * @param(in) size: uart send bytes
 * @returns -1:send fail, >=0: send bytes
 */
extern int interface_uart_send(int fd,char *buf,int size);

/* @brief uart receive data
 * @param(in) fd: uart fd 
 * @param(out) buf: uart receive buffer 
 * @param(in) size: uart receive max bytes
 * @returns  0: close -1:receive fail, >0: receive bytes
 */
extern int interface_uart_recv(int fd,char *buf,int size); 

#ifdef __cplusplus
}
#endif

#endif
