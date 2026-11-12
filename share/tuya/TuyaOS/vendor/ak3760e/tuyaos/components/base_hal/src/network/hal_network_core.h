/*
 * Copyright (C) 2021 Tuya Inc.
 */

#ifndef __HAL_NETWORK_CORE_H__
#define __HAL_NETWORK_CORE_H__

#include "hal_network.h"

/* HAL interface defination */
#define  DNS_CFG_PATH "/etc/resolv.conf"
#define  DNS_CFG_BAK_PATH "/tmp/resolv_bak.conf"

int hal_net_set_ip( hal_network_ip_s *ip );
int hal_net_get_ip( hal_network_ip_s *ip );
int hal_net_set_dns( hal_network_dns_s *dns );
int hal_net_get_dns( hal_network_dns_s *dns );
int hal_net_get_all_interface( hal_network_iface_s *iface );
int hal_net_interface_enable(const char *ifname, int enable );
int hal_net_set_default_interface( const char *name );
int hal_net_conf_route( int argc, char **argv );
int hal_net_get_route( char *buf, int size );
int hal_net_set_dhcp( int enable, int argc, char **argv );
#endif
