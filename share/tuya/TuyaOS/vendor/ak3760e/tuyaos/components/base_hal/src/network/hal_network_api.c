/*
 * Copyright (C) 2021 Tuya Inc.
 */

#include <errno.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "hal_network.h"
#include "hal_network_core.h"

int hal_network_set_ip( hal_network_ip_s *ip )
{
    int ret = -1;
    ret = hal_net_set_ip( ip );
    return ret;
}

int hal_network_get_ip( hal_network_ip_s *ip )
{
    int ret = -1;
    ret = hal_net_get_ip( ip );
    return ret;
}


int hal_network_set_dns( hal_network_dns_s *dns )
{
    int ret = -1;
    ret = hal_net_set_dns( dns );
    return ret;
}

int hal_network_get_dns( hal_network_dns_s *dns )
{
    int ret = -1;
    ret = hal_net_get_dns( dns );
    return ret;
}

int hal_network_get_all_interface( hal_network_iface_s *iface)
{
    int ret = -1;
    ret = hal_net_get_all_interface( iface );
    return ret;
}

int hal_network_open_interface(const char *name)
{
    int ret = -1;
    ret = hal_net_interface_enable( name, 1 );
    return ret;
}

int hal_network_close_interface(const char *name)
{
    int ret = -1;
    ret = hal_net_interface_enable( name, 0 );
    return ret;
}

int hal_network_set_default_interface(const char *name)
{
    int ret = -1;
    ret = hal_net_set_default_interface( name );
    return ret;
}

int hal_network_add_route(int argc, char **argv )
{
    int ret = -1;
    ret = hal_net_conf_route( argc, argv );
    return ret;
}

int hal_network_del_route(int argc, char **argv )
{
    int ret = -1;
    ret = hal_net_conf_route( argc, argv );
    return ret;
}

int hal_network_get_route( char *buf, int size )
{
    int ret = -1;
    ret = hal_net_get_route( buf, size );
    return ret;
}

int hal_network_set_dhcp( int enable, int argc, char **argv )
{
    int ret = -1;
    ret = hal_net_set_dhcp( enable, argc, argv );
    return ret;
}


