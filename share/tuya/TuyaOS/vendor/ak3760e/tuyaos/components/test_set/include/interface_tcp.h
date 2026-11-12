/* 
 *  @file interface_tcp.h
 *  @brief interface_tcp module is used to product test 
 *  communicate with  pc  product software
 *  @version 0.1
 *  @date 2021-5-07
 *
 *  @copyright Copyright (c) tuya.inc 2021
 *  
 */

#ifndef __INTERFACE_TCP_H__
#define __INTERFACE_TCP_H__

#ifdef _cplusplus
extern "C" {
#endif

// max tcp listen 
#define COM_TCP_MAX_SOCK  10

/* @brief tcp initialize
 * @param(in) port: tcp socket port
 * @param(out) tfd: tcp socket fd
 * @returns 0: succeed, -1:fail
 */
extern int  interface_tcp_init(short port,int *tfd);


/* @brief tcp uninitialize
 * @param(in) fd: tcp socket fd 
 * @returns void:none
 */
extern void interface_tcp_uinit(int fd);

/* @brief tcp send data
 * @param(in) fd: tcp socket fd 
 * @param(in) buf: tcp send buffer 
 * @param(in) size: tcp send bytes
 * @returns -1:send fail, >=0: send bytes
 */
extern int  interface_tcp_send(int fd,char *buf,int size); 

/* @brief tcp receive data
 * @param(in) fd: tcp socket fd 
 * @param(out) buf: tcp receive buffer 
 * @param(in) size: tcp receive max bytes
 * @returns 0: sockect close, -1:receive fail, >0: receive bytes
 */
extern int  interface_tcp_recv(int fd,char *buf,int size);

#ifdef __cplusplus
}
#endif

#endif

