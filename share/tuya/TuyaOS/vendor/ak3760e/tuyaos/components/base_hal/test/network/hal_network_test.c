/*
 * Copyright (C) 2021 Tuya Inc.
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <ctype.h>
#include <sys/file.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>

#include "hal_types.h"
#include "hal_network.h"
#include "hal_network_test.h"


#define NO_ARGS "not need args"
#define GET_DNS_USAGE NO_ARGS
#define GET_ALL_IFACE_USAGE NO_ARGS
#define GET_ROUTE_USAGE NO_ARGS
#define ADD_ROUTE_USAGE  "add default gw x.x.x.x dev eth0" 
#define DEL_ROUTE_USAGE  "del default gw x.x.x.x" 
#define SET_IP_USAGE "< --ifname name > [ --ip x.x.x.x ] [ --netmask x.x.x.x ] [ --gateway x.x.x.x ]"
#define GET_IP_USAGE "< --ifname name >"
#define SET_DNS_USAGE "[ --primary_dns severip(x.x.x.x) ] [ --secondary_dns severip(x.x.x.x) ]"
#define COMM_IFACE_USAGE  "< --ifname name >"
#define ENABLE_DHCP_USAGE  "-i eth0 -p /etc/udhcpc/udhcpc-eth0.pid -s /usr/share/udhcpc/eth0.sh -a 5 &" 
#define DISABLE_DHCP_USAGE  "-i eth0 -p /etc/udhcpc/udhcpc-eth0.pid" 

static struct option set_dns_options[] = {
    { "primary_dns", required_argument, 0, 0 },
    { "secondary_dns", required_argument, 0, 0 },
    { 0, 0, 0, 0 },
};

static struct option ifname_options[] = {
    { "ifname", required_argument, 0, 0 },
    { 0, 0, 0, 0 },
};

static struct option set_ip_options[] = {
    { "ifname", required_argument, 0, 0 },
    { "ip", required_argument, 0, 0 },
    { "netmask", required_argument, 0, 0 },
    { "gateway", required_argument, 0, 0 },
    { 0, 0, 0, 0 },
};

static arg_menu_s menu_args[] = {
    { "set_ip", "set ip", SET_IP_USAGE, s_set_ip, set_ip_options },
    { "get_ip", "get ip", GET_IP_USAGE, s_get_ip, ifname_options },
    { "get_dns", "get dns", GET_DNS_USAGE, s_get_dns, NULL },
    { "set_dns", "set dns", SET_DNS_USAGE, s_set_dns, set_dns_options },
    { "get_all_iface", "get all interface", GET_ALL_IFACE_USAGE, s_get_all_iface, NULL },
    { "open_iface", "open interface", COMM_IFACE_USAGE, s_enable_iface, ifname_options },
    { "close_iface", "close interface", COMM_IFACE_USAGE, s_enable_iface, ifname_options },
    { "get_route", "get route", GET_ROUTE_USAGE, s_get_route, NULL },
    { "add_route", "add route", ADD_ROUTE_USAGE, s_conf_route, NULL },
    { "del_route", "del route", DEL_ROUTE_USAGE, s_conf_route, NULL },
    { "set_default_iface", "set default interface", COMM_IFACE_USAGE, s_set_default_iface, ifname_options },
    { "enable_dhcp", "dhcp enable", ENABLE_DHCP_USAGE, s_conf_dhcp, NULL },
    { "disable_dhcp", "dhcp disable", DISABLE_DHCP_USAGE, s_conf_dhcp, NULL },
};

static int cmd_count = sizeof( menu_args ) / sizeof( arg_menu_s );

static int s_conf_route( void *cmd_opt, int argc, char **argv ) 
{
    int ret = -1;
    int opt;
    int option_index = 0;
    arg_menu_s *tmp = ( arg_menu_s * )cmd_opt;
    struct option *long_options = tmp->long_options;
    if ( argc < 3 ) {
        printf( "route cmd: %s\n", tmp->s_usage );
        return -1;
    }

    if ( strcmp( tmp->m_cmd, "add_route") == 0 ) {
        argv += 2;
        ret = hal_network_add_route( argc - 2, argv );
    }
    else {
        argv += 2;
        ret = hal_network_del_route( argc - 2, argv );
    }
     
    if ( ret < 0 ) {
        printf( "%s failure\n", tmp->m_cmd );
    }
    else {
        printf( "%s success\n", tmp->m_cmd );
    }
    return ret;

}

static int s_set_ip( void *cmd_opt, int argc, char **argv ) 
{

    int ret = -1;
    int opt;
    int option_index = 0;
    int valid = 0;
    arg_menu_s *tmp = ( arg_menu_s * )cmd_opt;
    struct option *long_options = tmp->long_options;
    hal_network_ip_s ip;
    memset( &ip, 0x0, sizeof( ip ) );
    if ( argc < 6 ) {
        printf( "s_set_ip:%s\n", tmp->s_usage );
        return -1;
    }

    while ( ( opt = getopt_long( argc, argv, "h", long_options, &option_index ) ) != -1 ) {
        switch ( opt ) {
            case 0:
                if ( strcmp( long_options[option_index].name, "ifname" ) == 0 )
                {
                    printf(" ifname: %s\n", optarg );
                    memcpy( ip.ifname, optarg, sizeof( ip.ifname ) );
                    valid++;
                    break;
                }
                else if ( strcmp( long_options[option_index].name, "ip" ) == 0 )
                {
                    printf(" ip: %s\n", optarg );
                    memcpy( ip.ip, optarg, sizeof( ip.ip ) );
                    valid++;
                    break;
                }
                else if ( strcmp( long_options[option_index].name, "netmask" ) == 0 )
                {
                    printf(" netmask: %s\n", optarg );
                    memcpy( ip.mask, optarg, sizeof( ip.mask ) );
                    valid++;
                    break;
                }
                else if ( strcmp( long_options[option_index].name, "gateway" ) == 0 )
                {
                    printf(" gateway: %s\n", optarg );
                    memcpy( ip.gw, optarg, sizeof( ip.gw ) );
                    valid++;
                    break;
                }
                /* fall through to default */
            default:
                case 'h':
                    printf( "s_set_ip:\n%s\n", tmp->s_usage );
                    break;
        }
    }

    if ( !valid ) {
        printf( "%s arg is invalid\n", tmp->m_cmd );
        goto exit_end;
    }

    ret = hal_network_set_ip( &ip );
    if ( ret < 0 ) {
        printf( "%s failure\n", tmp->m_cmd );
    }
    else {
        printf( "%s success\n", tmp->m_cmd );
    }

exit_end:
    return ret;
}

static int s_get_ip( void *cmd_opt, int argc, char **argv )  
{
    int ret = -1;
    int opt;
    int option_index = 0;
    int valid = 0;
    arg_menu_s *tmp = ( arg_menu_s * )cmd_opt;
    struct option *long_options = tmp->long_options;
    hal_network_ip_s ip;

    if ( argc < 4 ) {
        printf( "%s\n", tmp->s_usage );
        return -1;
    }

    memset( &ip, 0x0, sizeof( ip ) );
    while ( ( opt = getopt_long( argc, argv, "h", long_options, &option_index ) ) != -1 ) {
        switch ( opt ) {
            case 0:
                if ( strcmp( long_options[option_index].name, "ifname" ) == 0 )
                {
                    printf(" ifname: %s\n", optarg );
                    memcpy( ip.ifname, optarg, sizeof( ip.ifname ));
                    valid++;
                    break;
                }
                /* fall through to default */
            default:
                case 'h':
                    printf( "usage: %s\n", tmp->s_usage );
                    break;
        }
    }

    if ( !valid ) {
        printf( "%s arg is invalid\n", tmp->m_cmd );
        printf( "usage: %s\n", tmp->s_usage );
        goto exit_end;
    }

    ret = hal_network_get_ip(  &ip );
    if ( ret < 0 ) {
        printf( "%s failure\n", tmp->m_cmd );
    }
    else {
        printf( "%s success\n", tmp->m_cmd );
        printf( "ifname :%s\n", ip.ifname );
        printf( "ip     :%s\n", ip.ip );
        printf( "mask   :%s\n", ip.mask );
        printf( "gw     :%s\n", ip.gw );
    }
exit_end:
    return ret;
}


static int s_set_default_iface( void *cmd_opt, int argc, char **argv ) 
{

    int ret = -1;
    int opt;
    int option_index = 0;
    int valid = 0;
    char ifname[16];
    arg_menu_s *tmp = ( arg_menu_s * )cmd_opt;
    struct option *long_options = tmp->long_options;
    if ( argc < 3 ) {
        printf( "%s\n", tmp->s_usage );
        return -1;
    }

    while ( ( opt = getopt_long( argc, argv, "h", long_options, &option_index ) ) != -1 ) {
        switch ( opt ) {
            case 0:
                if ( strcmp( long_options[option_index].name, "ifname" ) == 0 )
                {
                    printf(" ifname: %s\n", optarg );
                    memcpy( ifname, optarg, sizeof( ifname ));
                    valid++;
                    break;
                }
                /* fall through to default */
            default:
                case 'h':
                    printf( "usage: %s\n", tmp->s_usage );
                    break;
        }
    }

    if ( !valid ) {
        printf( "%s arg is invalid\n", tmp->m_cmd );
        printf( "usage: %s\n", tmp->s_usage );
        goto exit_end;
    }

    if ( strcmp( tmp->m_cmd, "set_default_iface" ) == 0 ) {
        ret = hal_network_set_default_interface( ifname );
    }
    else {
        printf( "not recognize cmd %s\n", tmp->m_cmd );
        ret = -1;
        goto exit_end;
    }

    if ( ret < 0 ) {
        printf( "%s failure\n", tmp->m_cmd );
    }
    else {
        printf( "%s success\n", tmp->m_cmd );
    }

exit_end:
    return ret;
}


static int s_enable_iface( void *cmd_opt, int argc, char **argv ) 
{

    int ret = -1;
    int opt;
    int option_index = 0;
    int valid = 0;
    char ifname[16];
    arg_menu_s *tmp = ( arg_menu_s * )cmd_opt;
    struct option *long_options = tmp->long_options;
    if ( argc < 3 ) {
        printf( "%s\n", tmp->s_usage );
        return -1;
    }

    while ( ( opt = getopt_long( argc, argv, "h", long_options, &option_index ) ) != -1 ) {
        switch ( opt ) {
            case 0:
                if ( strcmp( long_options[option_index].name, "ifname" ) == 0 )
                {
                    printf(" ifname: %s\n", optarg );
                    memcpy( ifname, optarg, sizeof( ifname ));
                    valid++;
                    break;
                }
                /* fall through to default */
            default:
                case 'h':
                    printf( "usage: %s\n", tmp->s_usage );
                    break;
        }
    }

    if ( !valid ) {
        printf( "%s arg is invalid\n", tmp->m_cmd );
        printf( "usage: %s\n", tmp->s_usage );
        goto exit_end;
    }

    if ( strcmp( tmp->m_cmd, "open_iface") == 0 ) {
        ret = hal_network_open_interface( ifname );
    }
    else if ( strcmp( tmp->m_cmd, "close_iface") == 0 ) {
        ret = hal_network_close_interface( ifname );
    }
    else {
        printf( "not recognize cmd %s\n", tmp->m_cmd );
        ret = -1;
        goto exit_end;
    }

    if ( ret < 0 ) {
        printf( "%s failure\n", tmp->m_cmd );
    }
    else {
        printf( "%s success\n", tmp->m_cmd );
    }

exit_end:
    return ret;
}

static int s_get_all_iface( void *cmd_opt, int argc, char **argv )  
{
    int ret = -1;
    int opt;
    int option_index = 0;
    arg_menu_s *tmp = ( arg_menu_s * )cmd_opt;
    struct option *long_options = tmp->long_options;
    int i = 0;
    
    hal_network_iface_s iface;
    memset( &iface, 0x0, sizeof( iface ) );
    ret = hal_network_get_all_interface( &iface );
    if ( ret < 0 ) {
        printf( "%s failure\n", tmp->m_cmd );
    }
    else {
        printf( "%s success\n", tmp->m_cmd );
        printf( "found %d ifaces:\n", iface.count );
        for ( i = 0; i < iface.count; i++ ) {
            printf( "%s ", iface.ifaces[i].ifname );
        }
        printf( "\n" );
        
    }
    return ret;
}

static int s_set_dns( void *cmd_opt, int argc, char **argv ) 
{

    int ret = -1;
    int opt;
    int option_index = 0;
    int valid = 0;
    arg_menu_s *tmp = ( arg_menu_s * )cmd_opt;
    struct option *long_options = tmp->long_options;
    hal_network_dns_s dns;
    memset( &dns, 0x0, sizeof( dns ) );
    if ( argc < 4 ) {
        printf( "set_set_dns:\n%s\n", tmp->s_usage );
        return -1;
    }

    while ( ( opt = getopt_long( argc, argv, "h", long_options, &option_index ) ) != -1 ) {
        switch ( opt ) {
            case 0:
                if ( strcmp( long_options[option_index].name, "primary_dns" ) == 0 )
                {
                    printf(" dns_primary: %s\n", optarg );
                    memcpy( dns.dns_primary, optarg, sizeof( dns.dns_primary ) );
                    valid++;
                    break;
                }
                else if ( strcmp( long_options[option_index].name, "secondary_dns" ) == 0 )
                {
                    printf(" dns_secondary: %s\n", optarg );
                    memcpy( dns.dns_secondary, optarg, sizeof( dns.dns_secondary) );
                    valid++;
                    break;
                }
                /* fall through to default */
            default:
                case 'h':
                    printf( "set_set_dns:\n%s\n", tmp->s_usage );
                    break;
        }
    }

    if ( !valid ) {
        printf( "%s arg is invalid\n", tmp->m_cmd );
        goto exit_end;
    }

    ret = hal_network_set_dns( &dns );
    if ( ret < 0 ) {
        printf( "%s failure\n", tmp->m_cmd );
    }
    else {
        printf( "%s success\n", tmp->m_cmd );
    }

exit_end:
    return ret;
}

static int s_get_dns( void *cmd_opt, int argc, char **argv )  
{
    int ret = -1;
    int opt;
    int option_index = 0;
    arg_menu_s *tmp = ( arg_menu_s * )cmd_opt;
    struct option *long_options = tmp->long_options;
    hal_network_dns_s dns;
    memset( &dns, 0x0, sizeof( dns ) );
    ret = hal_network_get_dns( &dns );
    if ( ret < 0 ) {
        printf( "%s failure\n", tmp->m_cmd );
    }
    else {
        printf( "%s success\n", tmp->m_cmd );
    }
    return ret;
}

static int s_get_route( void *cmd_opt, int argc, char **argv )  
{
    int ret = -1;
    int opt;
    int option_index = 0;
    arg_menu_s *tmp = ( arg_menu_s * )cmd_opt;
    struct option *long_options = tmp->long_options;
    int size = 128 * 1024;
    char *buf = malloc( size );
    if ( !buf ) {
        printf(" failed to alloc mem\n");
        return -1;
    }

    memset( buf, 0x0, size );
    ret = hal_network_get_route( buf, size );
    if ( ret < 0 ) {
        printf( "%s failure\n", tmp->m_cmd );
    }
    else {
        printf( "%s success\n", tmp->m_cmd );
        printf( "%s\n", buf );
    }
    free( buf );
    return ret;
}



static void main_menu_usage( void )
{
    int i = 0;
    arg_menu_s *tmp = NULL;
    
    printf( "main_menu_usage:\n" );
    for ( i = 0; i < cmd_count; i++ ) {
        tmp = &menu_args[ i ];
        printf("\t%-20s: %s \n", tmp->m_cmd, tmp->m_usage );
    }
}

static int s_conf_dhcp( void *cmd_opt, int argc, char **argv ) 
{
    int ret = -1;
    int opt;
    int option_index = 0;
    arg_menu_s *tmp = ( arg_menu_s * )cmd_opt;
    struct option *long_options = tmp->long_options;
    if ( argc < 4 ) {
        printf( "udhcp cmd: %s\n", tmp->s_usage );
        return -1;
    }

    if ( strcmp( tmp->m_cmd, "enable_dhcp") == 0 ) {
        argv += 2;
        ret = hal_network_set_dhcp( 1, argc - 2, argv );
    }
    else if ( strcmp( tmp->m_cmd, "disable_dhcp") == 0 ) {
        argv += 2;
        ret = hal_network_set_dhcp( 0, argc - 2, argv );
    }
     
    if ( ret < 0 ) {
        printf( "%s failure\n", tmp->m_cmd );
    }
    else {
        printf( "%s success\n", tmp->m_cmd );
    }
    return ret;

}



int main( int argc, char **argv )
{
    int i;
    arg_menu_s *tmp = NULL;
    char *cmd = NULL;

    printf( "%s Compiled on %s %s\n", argv[0], __DATE__, __TIME__ );

    if ( argc < 2 ) {
        main_menu_usage();
        return -1;
    }
    cmd = argv[1];

    for ( i = 0; i < cmd_count; i++ ) {
        tmp = &menu_args[ i ];
        if ( strcmp( tmp->m_cmd, cmd ) == 0 ) {
            printf("found m_cmd: %s\n", tmp->m_cmd );
            tmp->s_handle( ( void* )tmp, argc, argv );
            return 0;
        }
    }

    printf("not found cmd: %s\n", cmd );
    return -1;
}
