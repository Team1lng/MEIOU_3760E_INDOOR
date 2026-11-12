/*
 * Copyright (C) 2021 Tuya Inc.
 */

#ifndef __HAL_NETWORK_H__
#define __HAL_NETWORK_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct    
{    
    char ifname[16]; //interface name
    char ip[16];     // ip addr:  xxx.xxx.xxx.xxx 
    char mask[16];   // net mask: xxx.xxx.xxx.xxx   
    char gw[16];     // gateway:  xxx.xxx.xxx.xxx      
}hal_network_ip_s;

typedef struct    
{    
    char dns_primary[16];      //ip addr:  xxx.xxx.xxx.xxx  
    char dns_secondary[16];    //ip addr:  xxx.xxx.xxx.xxx
}hal_network_dns_s;

typedef struct    
{    
    char ifname[16];
}hal_iface_s;

typedef struct    
{    
    int count; //interface count
    hal_iface_s ifaces[8];
}hal_network_iface_s;

/**
 * @brief  set network ip address
 * @param[in]  ip: ip info
 * @return 0 on success. Others on error
 */
int hal_network_set_ip( hal_network_ip_s *ip );

/**
 * @brief  get network ip address
 * @param[in && out]  ip: ip info, need to spcify ifname
 * @return 0 on success. Others on error
 */
int hal_network_get_ip( hal_network_ip_s *ip );


/**
 * @brief  set dns server
 * @param[in]  dns: dns server info
 * @return 0 on success. Others on error
 */
int hal_network_set_dns( hal_network_dns_s *dns );


/**
 * @brief  get dns server
 * @param[in]  dns: dns server info
 * @return 0 on success. Others on error
 */
int hal_network_get_dns( hal_network_dns_s *dns );


/**
 * @brief set dhcp  
 * @param[in]  enable: 0 stop dhcp, 1 start dhcp
 * @param[in]  argc: arg count
 * @param[in]  argv: refer to udhcpc cmd, must specify iterface name
 * @return 0 on success. Others on error
 */
int hal_network_set_dhcp( int enable, int argc, char **argv );

/**
 * @brief get all up interface info
 * @param[in]  iface: store iterface info
 * @return 0 on success. Others on error
 */
int hal_network_get_all_interface( hal_network_iface_s *iface);

/**
 * @brief open interface 
 * @param[in]  name: iterface name 
 * @return 0 on success. Others on error
 */
int hal_network_open_interface(const char *name);

/**
 * @brief close interface 
 * @param[in]  name: iterface name 
 * @return 0 on success. Others on error
 */
int hal_network_close_interface(const char *name);

/**
 * @brief  specify the default iterface through the default route
 * @param[in]  name: iterface name 
 * @return 0 on success. Others on error
 */
int hal_network_set_default_interface(const char *name);

/**
 * @brief  add route 
 * @param[in] argc: arg count
 * @param[in] args: refer to route add cmd
 * @return 0 on success. Others on error
 */
int hal_network_add_route( int argc, char **argv );

/**
 * @brief  del route 
 * @param[in] argc: arg count
 * @param[in] args: refer to route del cmd 
 * @return 0 on success. Others on error
 */
int hal_network_del_route( int argc, char **argv );

/**
 * @brief  get route table
 * @param[out] buf: store route info, refer to route -n cmd
 * @param[out] len: buf size
 * @return 0 on success. Others on error
 */
int hal_network_get_route( char *buf, int len );

#ifdef __cplusplus
}
#endif

#endif
