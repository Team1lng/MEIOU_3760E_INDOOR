/**
 * @file tuya_os_adapter.h
 * @author sunkz@tuya.com
 * @brief 
 * @version 0.1
 * @date 2020-05-15
 * 
 * @copyright Copyright (c) tuya.inc 2019
 * 
 */
#ifndef _TUYA_OS_ADAPTER_H
#define _TUYA_OS_ADAPTER_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "tuya_os_adapter_error_code.h"

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************
*************************micro define***********************
***********************************************************/
typedef enum {
    INTF_MEMORY,
    INTF_MUTEX,
    INTF_SEMAPHORE,
    INTF_THREAD,
    INTF_FILE_SYSTEM,
    INTF_NETWORK,
    INTF_OUTPUT,
    INTF_SYSTEM,
    INTF_INTERRUPT,
    INTF_QUEUE,
    ALL_INTF_NUMS
} INTF_TYPE_ENUM;

/********************************************************************************
 *********************************tuya_os_fs_intf********************************
 ********************************************************************************/
// 文件访问权限
#define	TUYA_IRUSR  0400    /* Read by owner.  */
#define	TUYA_IWUSR  0200    /* Write by owner.  */
#define	TUYA_IXUSR  0100    /* Execute by owner.  */


// 缓冲区搜索起始位置类型
#define TUYA_SEEK_SET   0   /* Seek from beginning of file.  */
#define TUYA_SEEK_CUR   1   /* Seek from current position.  */
#define TUYA_SEEK_END   2   /* Seek from end of file.  */

typedef void* TUYA_DIR;
typedef void* TUYA_FILEINFO;
typedef void* TUYA_FILE;

typedef struct {
    OPERATE_RET   (*fs_mkdir)      (CONST CHAR_T* path);
    OPERATE_RET   (*fs_remove)     (CONST CHAR_T* path);
    OPERATE_RET   (*fs_mode)       (CONST CHAR_T* path, UINT_T* mode);
    OPERATE_RET   (*fs_is_exist)   (CONST CHAR_T* path, BOOL_T* is_exist);
    OPERATE_RET   (*fs_rename)     (CONST CHAR_T* path_old, CONST CHAR_T* path_new);
    OPERATE_RET   (*dir_open)      (CONST CHAR_T* path, TUYA_DIR* dir);
    OPERATE_RET   (*dir_close)     (TUYA_DIR dir);
    OPERATE_RET   (*dir_read)      (TUYA_DIR dir, TUYA_FILEINFO* info);
    OPERATE_RET   (*dir_name)      (TUYA_FILEINFO info, CONST CHAR_T** name);
    OPERATE_RET   (*dir_is_dir)    (TUYA_FILEINFO info, BOOL_T* is_dir);
    OPERATE_RET   (*dir_is_regular)(TUYA_FILEINFO info, BOOL_T* is_regular);
    TUYA_FILE     (*fopen)         (CONST CHAR_T* path, CONST CHAR_T* mode);
    OPERATE_RET   (*fclose)        (TUYA_FILE file);
    OPERATE_RET   (*fread)         (VOID_T* buf, INT_T bytes, TUYA_FILE file);
    OPERATE_RET   (*fwrite)        (VOID_T* buf, INT_T bytes, TUYA_FILE file);
    OPERATE_RET   (*fsync)         (TUYA_FILE file);
    CHAR_T*       (*fgets)         (CHAR_T* buf, INT_T len, TUYA_FILE file);
    OPERATE_RET   (*feof)          (TUYA_FILE file);
    OPERATE_RET   (*fseek)         (TUYA_FILE file, INT64_T offs, INT_T whence);
    INT64_T       (*ftell)         (TUYA_FILE file);
} TUYA_OS_FS_INTF;


/********************************************************************************
 *********************************tuya_os_memory_intf****************************
 ********************************************************************************/
typedef struct {
    VOID_T*  (*malloc)    (CONST size_t size);
    VOID_T   (*free)      (VOID_T* ptr);
} TUYA_OS_MEMORY_INTF;


/********************************************************************************
 *********************************tuya_os_mutex_intf*****************************
 ********************************************************************************/
typedef void* MUTEX_HANDLE;

typedef struct {
    OPERATE_RET   (*init)      (MUTEX_HANDLE *pMutexHandle);
    OPERATE_RET   (*lock)      (const MUTEX_HANDLE mutexHandle);
    OPERATE_RET   (*unlock)    (const MUTEX_HANDLE mutexHandle);
    OPERATE_RET   (*release)   (const MUTEX_HANDLE mutexHandle);
} TUYA_OS_MUTEX_INTF;


/********************************************************************************
 *********************************tuya_os_network_intf***************************
 ********************************************************************************/
 
/* tuya sdk definition of 127.0.0.1 */
#define TY_IPADDR_LOOPBACK     ((uint32_t)0x7f000001UL)
/* tuya sdk definition of 0.0.0.0 */
#define TY_IPADDR_ANY          ((uint32_t)0x00000000UL)
/* tuya sdk definition of 255.255.255.255 */
#define TY_IPADDR_BROADCAST    ((uint32_t)0xffffffffUL)

/* tuya sdk definition of shutdown type */
#define UNW_SHUT_RD   0
#define UNW_SHUT_WR   1
#define UNW_SHUT_RDWR 2

#if defined(SYSTEM_LINUX) && (OPERATING_SYSTEM == SYSTEM_LINUX)
/* fd 最大个数, 不同平台根据实际情况定义 */
#define UNW_FD_MAX_COUNT    (1024)
#else
#define UNW_FD_MAX_COUNT    (64)
#endif
/* tuya sdk definition of fd operations */
typedef struct {
    UINT8_T placeholder[(UNW_FD_MAX_COUNT+7)/8];
} UNW_FD_SET_T;

/* tuya sdk definition of socket protocol */
typedef enum {
    PROTOCOL_TCP = 0,
    PROTOCOL_UDP = 1,
    PROTOCOL_RAW = 2,
}UNW_PROTOCOL_TYPE;

/* tuya sdk definition of transfer type */
typedef enum {
    TRANS_RECV = 0,
    TRANS_SEND = 1,
}UNW_TRANS_TYPE_E;

/* tuya sdk definition of IP addr */
typedef UINT_T UNW_IP_ADDR_T;

/* tuya sdk definition of IP info */
typedef struct
{
    char ip[16];    /* ip addr:  xxx.xxx.xxx.xxx  */
    char mask[16];  /* net mask: xxx.xxx.xxx.xxx  */
    char gw[16];    /* gateway:  xxx.xxx.xxx.xxx  */
}NW_IP_S;

//the MAC addr len
#define TY_MAC_ADDR_LEN 6

/* tuya sdk definition of MAC info */
typedef struct
{
    uint8_t mac[TY_MAC_ADDR_LEN]; /* mac address */
}NW_MAC_S;

/* add begin: by sunkz, interface regist */
typedef struct {
    int           (*get_errno)     (void);
    int           (*fd_set)        (int fd, UNW_FD_SET_T* fds);
    int           (*fd_clear)      (int fd, UNW_FD_SET_T* fds);
    int           (*fd_isset)      (int fd, UNW_FD_SET_T* fds);
    int           (*fd_zero)       (UNW_FD_SET_T* fds);
    int           (*select)        (const int maxfd, UNW_FD_SET_T *readfds, UNW_FD_SET_T *writefds, UNW_FD_SET_T *errorfds, const UINT_T ms_timeout);
    int           (*get_nonblock)  (const int fd);
    int           (*set_block)     (const int fd, const bool_t block);
    int           (*close)         (const int fd);
    int           (*shutdown)      (const int fd, const int how);
    int           (*socket_create) (const UNW_PROTOCOL_TYPE type);
    int           (*connect)       (const int fd, const UNW_IP_ADDR_T addr, const UINT16_T port);
    int           (*connect_raw)   (const int fd, void *p_socket, const int len);
    int           (*bind)          (const int fd, const UNW_IP_ADDR_T addr, const UINT16_T port);
    int           (*listen)        (const int fd, const int backlog);
    int           (*send)          (const int fd, const void *buf, const UINT_T nbytes);
    int           (*send_to)       (const int fd, const void *buf, const UINT_T nbytes, const UNW_IP_ADDR_T addr, const UINT16_T port);
    int           (*recv)          (const int fd, void *buf, const UINT_T nbytes);
    int           (*recvfrom)      (const int fd, void *buf, const UINT_T nbytes, UNW_IP_ADDR_T *addr, UINT16_T *port);
    int           (*set_timeout)   (const int fd, const int ms_timeout, const UNW_TRANS_TYPE_E type);
    int           (*set_bufsize)   (const int fd, const int buf_size, const UNW_TRANS_TYPE_E type);
    int           (*set_reuse)     (const int fd);
    int           (*disable_nagle) (const int fd);
    int           (*set_boardcast) (const int fd);
    int           (*gethostbyname) (const char *domain, UNW_IP_ADDR_T *addr);
    int           (*accept)        (const int fd, UNW_IP_ADDR_T *addr, UINT16_T *port);
    int           (*recv_nd_size)  (const int fd, void *buf, const UINT_T buf_size, const UINT_T nd_size);
    UNW_IP_ADDR_T (*str2addr)      (signed char *ip_str);
    char*         (*addr2str)      (UNW_IP_ADDR_T ipaddr);
    int           (*set_keepalive) (int fd, const bool_t alive, const UINT_T idle, const UINT_T intr, const UINT_T cnt);
    int           (*socket_bind)   (int fd, const char *ip);
    int           (*set_cloexec)   (const int fd);
    int           (*get_socket_ip) (int fd, UNW_IP_ADDR_T *addr);
    UNW_IP_ADDR_T (*addr)          (const signed char *cp);
}TUYA_OS_NETWORK_INTF;


/********************************************************************************
 *********************************tuya_os_output_intf****************************
 ********************************************************************************/
typedef struct {
    VOID_T        (*output_log)  (CONST CHAR_T *str);
    OPERATE_RET   (*log_close)   (VOID_T);
    OPERATE_RET   (*log_open)    (VOID_T);
} TUYA_OS_OUTPUT_INTF;


/********************************************************************************
 *********************************tuya_os_semaphore_intf*************************
 ********************************************************************************/
typedef void* SEM_HANDLE;

typedef struct {
    OPERATE_RET   (*init)       (SEM_HANDLE *pHandle, CONST UINT_T semCnt, CONST UINT_T sem_max);
    OPERATE_RET   (*wait)       (CONST SEM_HANDLE semHandle);
    OPERATE_RET   (*waittimeout)(CONST SEM_HANDLE semHandle, unsigned int timeout);
    OPERATE_RET   (*post)       (CONST SEM_HANDLE semHandle);
    OPERATE_RET   (*release)    (CONST SEM_HANDLE semHandle);
} TUYA_OS_SEMAPHORE_INTF;



/********************************************************************************
 *********************************tuya_os_system_intf****************************
 ********************************************************************************/
#if OPERATING_SYSTEM == SYSTEM_LINUX
typedef UINT64_T SYS_TICK_T;
#else
typedef UINT_T SYS_TICK_T;
#endif

typedef enum {
	TY_RST_POWER_OFF = 0,
	TY_RST_HARDWARE_WATCHDOG,
	TY_RST_FATAL_EXCEPTION,
	TY_RST_SOFTWARE_WATCHDOG,
	TY_RST_SOFTWARE,
	TY_RST_DEEPSLEEP,
	TY_RST_HARDWARE,
	TY_RST_OTHER = 0xAA,
	TY_RST_UNSUPPORT = 0xFF,
} TY_RST_REASON_E;

typedef enum {
	TY_CPU_SLEEP,
	TY_CPU_DEEP_SLEEP,
} TY_CPU_SLEEP_MODE_E;

typedef struct {
    SYS_TICK_T      (*get_systemtickcount)    (void);
    UINT_T          (*get_tickratems)         (void);
    void            (*system_sleep)           (const unsigned long msTime);
    void            (*system_reset)           (void);
    unsigned int    (*watchdog_init_start)    (const unsigned int timeval);
    void            (*watchdog_refresh)       (void);
    void            (*watchdog_stop)          (void);
    int             (*system_getheapsize)     (void);
    TY_RST_REASON_E (*system_get_rst_info)    (void);
    OPERATE_RET     (*system_get_rst_ext_info)(CHAR_T** ext_info);
    int             (*get_random_data)        (const unsigned int range);
    int             (*set_cpu_lp_mode)        (const bool_t en, const TY_CPU_SLEEP_MODE_E mode);
} TUYA_OS_SYSTEM_INTF;

/********************************************************************************
 *********************************tuya_os_thread_intf****************************
 ********************************************************************************/
typedef void* THREAD_HANDLE;
typedef void (*THREAD_FUNC_T)(void*);

typedef struct {
    OPERATE_RET   (*create)         (THREAD_HANDLE* thread, CONST char* name, UINT_T stack_size, UINT_T priority, THREAD_FUNC_T func, VOID_T* CONST arg);
    OPERATE_RET   (*release)        (THREAD_HANDLE thread);
    OPERATE_RET   (*is_self)        (THREAD_HANDLE thread, BOOL_T* is_self);
    OPERATE_RET   (*set_self_name)  (CONST CHAR_T* name);
} TUYA_OS_THREAD_INTF;

/********************************************************************************
 *********************************tuya_os_queue_intf****************************
 ********************************************************************************/
 /* tuya sdk definition of queue context */
typedef void* QUEUE_HANDLE;

typedef struct {
    OPERATE_RET   (*init)   (QUEUE_HANDLE *queue, int size);
    VOID_T        (*free)   (QUEUE_HANDLE queue);
    OPERATE_RET   (*post)   (QUEUE_HANDLE queue, void *data, unsigned int timeout);
    OPERATE_RET   (*fetch)  (QUEUE_HANDLE queue, void **msg, unsigned int timeout);
} TUYA_OS_QUEUE_INTF;


/***********************************************************
*************************variable define********************
***********************************************************/

/***********************************************************
*************************function define********************
***********************************************************/

//该接口给开发环境各类型的系统接口注册使用
OPERATE_RET tuya_os_adapt_reg_intf(INTF_TYPE_ENUM type, void* intf_ptr);
//该接口给上层sdk调用用于初始化组件接口
VOID_T tuya_os_adapter_intf_impl_init(VOID_T);
//该接口给上层sdk获取各类型的系统接口列表
VOID_T* tuya_os_adapter_get_intf(INTF_TYPE_ENUM type);

#ifdef __cplusplus
}
#endif

#endif /* _TUYA_OS_ADAPTER_H */
