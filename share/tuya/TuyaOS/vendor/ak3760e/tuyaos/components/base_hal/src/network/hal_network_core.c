/*
 * Copyright (C) 2021 Tuya Inc.
 */

#include <stdarg.h>
#include <stdio.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/reboot.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <linux/sockios.h>
#include <net/route.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include "hal_network.h"
#include "hal_network_core.h"

static int hal_net_get_ip_from_string( const char *str, char *ip, int ip_len ) 
{
    int ret = -1;
    int i = 0;
    char *tmp = NULL;
    char buf[16];
    int len = 0;
    int j = 0;
    int start = 0; //only first ip
    int end = 0; 
    
    if ( !str || !ip ) {
        printf( "%s: arg is null\n", __func__ );
        return ret;
    }
    
    memset( ip, 0x0, ip_len );
    memset( buf, 0x0, sizeof( buf ) );
    tmp = ( char * )str;
    
    len = strlen( str );
    j = 0;
    for ( i = 0; i < len; i++ ) {

        if ( end ) {
            break;
        }

        if ( ( *( tmp + i) >= '0' && *( tmp + i) <= '9' ) || ( *( tmp + i) == '.') ) {
            if ( !start ) {
                start = 1;
            } 
            if ( j < 16 ) {
                buf[ j ] = *( tmp + i );
                j++;
            }
        }
        else {
            if ( start ) {
                end = 1; //has found first ip
            } 
        }

    }
    
    memcpy( ip, buf, ip_len );

    ret = 0;
    return ret;
}

static char *hal_net_space_trim( char *str )
{
    int i = 0;  
    int count = 0;
    int size = 0;

    char *tmp  = NULL;
    if ( !str ) {
        return NULL;
    }

    size = strlen( str );
    tmp = str;
    
    while ( *str == ' ' || (unsigned char)(*str - 9) <= (13 - 9) ) {
        str++;        
        count++;
    }
    
    if ( count ) {
        memmove( tmp, str, size - count );
        memset( tmp + ( size - count ), 0x0 , size - count );
    }
      
    return (char *) tmp;    
}

static int hal_net_validate_ip( char *value )
{
    struct sockaddr_in sa;
    int ret = inet_pton( AF_INET, value, &( sa.sin_addr ) );
    return ret;
}

static int hal_net_get_default_gateway( hal_network_ip_s *ip )
{
    FILE *fp = NULL;
    char buf[256] = {0};
    unsigned int dest = 0;
    struct in_addr addr;
    int get_gw = 0;
    int ret = -1;
    fp = fopen( "/proc/net/route", "r" );
    if ( fp == NULL ) {
        printf( "%s %d: failed to open file\n", __func__, __LINE__ );
        return -1;
    }

    while( fgets( buf, sizeof( buf ), fp ) ) {

        sscanf( buf, "%*s%x %x", &dest, &addr.s_addr );
        if ( ( dest == 0 ) && ( addr.s_addr != 0 ) ) {
            strncpy( ip->gw, inet_ntoa( addr ), sizeof( ip->gw ) );
            get_gw = 1;
            ret = 0;
            break;
        }
    }

    if ( get_gw == 0 ) {
        printf( "%s %d: failed to get gw\n", __func__, __LINE__ );
    }

    fclose( fp );
    return ret;
}


int hal_net_set_ip( hal_network_ip_s *ip )
{
    int sk;
    int ret = 0;
    struct ifreq ifr; 
    struct sockaddr_in *sin;
    struct rtentry  rt;
    
    if ( ip == NULL || ( strlen( ip->ifname ) == 0 ) ) {
        printf( "%s %d: arg is error\n", __func__, __LINE__ );
        return -1;
    }

    sk = socket( AF_INET, SOCK_DGRAM, 0 );
    if( sk < 0 ) {
        printf( "%s %d: failed to create sk\n", __func__, __LINE__ );
        return -1;     
    }

    memset( &ifr, 0, sizeof( ifr ) ); 
    printf( "%s %d: ifname %s\n", __func__, __LINE__, ip->ifname );
    strncpy( ifr.ifr_name, ip->ifname, sizeof( ifr.ifr_name ) - 1 );

    sin = ( struct sockaddr_in * )&ifr.ifr_addr;     
    sin->sin_family = AF_INET;     

    if( inet_aton( ip->ip, &( sin->sin_addr ) ) != 0 ) {     
        printf( "%s %d: ip %s\n", __func__, __LINE__, ip->ip);
        if( ioctl( sk, SIOCSIFADDR, &ifr ) < 0 ) {     
            printf( "%s %d: ip ioctl %s, error=%d,%s\n", __func__, __LINE__, ip->ip, errno, strerror( errno ) );
            ret = -1;
        }
    }
    else {
        printf( "%s %d: error ip %s\n", __func__, __LINE__, ip->ip );
    }    

    if( inet_aton( ip->mask, &( sin->sin_addr ) ) != 0 ) {     
        printf( "%s %d: mask %s\n", __func__, __LINE__, ip->mask);
        if( ioctl( sk, SIOCSIFNETMASK, &ifr ) < 0 ) {
            printf( "%s %d: mask iotcl %s, error=%d,%s\n", __func__, __LINE__, ip->mask, errno, strerror( errno ) );
            ret = -1;
        }
    }    
    else {
        printf( "%s %d: error mask %s\n", __func__, __LINE__, ip->mask );
    }

    memset( &rt, 0, sizeof( struct rtentry ) );
    sin = (struct sockaddr_in *)&rt.rt_gateway;    
    sin->sin_family = AF_INET;    
    if ( inet_aton(ip->gw, &(sin->sin_addr)) != 0 ) {    

        sin = (struct sockaddr_in *)&rt.rt_dst;    
        sin->sin_family = AF_INET;    
        sin->sin_addr.s_addr = INADDR_ANY;    

        sin = (struct sockaddr_in*)&rt.rt_genmask;    
        sin->sin_family = AF_INET;    
        sin->sin_addr.s_addr = INADDR_ANY;    

        if ( ioctl(sk, SIOCDELRT, &rt ) < 0) {    
            printf( "%s %d: gateway iotcl %s, error=%d,%s\n", __func__, __LINE__, ip->gw, errno, strerror( errno ) );
            if ( errno == ESRCH ) {
                ret = 0;
            }
            else {
                ret = -1;
                goto exit_end;
            }
        }    

        rt.rt_flags = (RTF_UP | RTF_GATEWAY);    
        rt.rt_dev = ip->ifname;    

        if ( ioctl(sk, SIOCADDRT, &rt ) < 0) {    
            printf( "%s %d: gateway iotcl %s, error=%d,%s\n", __func__, __LINE__, ip->gw, errno, strerror( errno ) );
            if ( errno == EEXIST ) {
                ret = 0;
            }
            else {
                ret = -1;
            }
        }    

    }    
    else {
        printf( "%s %d: set gw error %s\n", __func__, __LINE__, ip->gw );
    }
    
exit_end:
    if ( sk )
        close( sk );
    return ret;
}

int hal_net_get_ip( hal_network_ip_s *ip )
{
    int sk = -1;
    struct ifreq ifr;
    struct sockaddr_in *sin;
    int ret = -1;

    if ( ip == NULL || ( strlen( ip->ifname ) == 0 ) ) {
        printf( "%s %d: arg is error\n", __func__, __LINE__ );
        return -1;
    }

    sk = socket( AF_INET, SOCK_DGRAM, 0 );
    if ( sk < 0 ) {
        printf( "%s %d: arg is error\n", __func__, __LINE__ );
        return -1;
    }

    memset( &ifr, 0, sizeof( ifr ) );
    strncpy( ifr.ifr_name, ip->ifname, sizeof( ifr.ifr_name ) - 1 );
    sin = ( struct sockaddr_in * )&( ifr.ifr_addr );

    ret = ioctl( sk, SIOCGIFADDR, &ifr );
    if ( ret < 0) {
        printf( "%s %d: failed to get ip\n", __func__, __LINE__ );
        goto exit_end;
    }
    strncpy( ip->ip, inet_ntoa( sin->sin_addr ), sizeof( ip->ip ));

    ret = ioctl( sk, SIOCGIFNETMASK, &ifr );
    if (  ret < 0 ) {
        printf( "%s %d: failed to get mask\n", __func__, __LINE__ );
        goto exit_end;
    }
    strncpy( ip->mask, inet_ntoa( sin->sin_addr ), sizeof( ip->mask ));

    if ( hal_net_get_default_gateway( ip ) < 0 ) {
        ret = -1;
        goto exit_end;
    }

exit_end:
    close( sk );
    return ret;
}

static int hal_net_get_abs_path( const char *file,  char *path, int size ) 
{
    int ret = -1;
    struct stat info;    
    char buf[ 128 ];
    int count = 0;
    int len = sizeof( buf );

    if ( !file ) {
        printf( "%s %d: %s is null\n", __func__, __LINE__, file );
        return -1;
    }

    ret = lstat( file, &info );    
    if( ret == -1 ) {    
        printf( "%s %d: %s lstat error\n", __func__, __LINE__, file );
        return ret;
    }    


    memset( path, 0x0, size );
    if ( S_ISREG( info.st_mode ) ) {    
        printf( "%s %d: %s is file\n", __func__, __LINE__, file );
        memcpy( path, file, strlen( file ) );
        ret = 0;
    }    
    else if(S_ISLNK(info.st_mode)) {    
        printf( "%s %d: %s is soft link\n", __func__, __LINE__, file );
        memset( buf, 0x0, len );    

        count = readlink( file, buf, len -1 );    
        if ( count < 0 || count > len -1 ) {    
            printf( "%s %d: %s readlink error, %d,%s \n", __func__, __LINE__, file, errno, strerror( errno ) );
        }    
        else {    
            printf( "%s %d: %s\n", __func__, __LINE__, buf );
            if ( (*( buf ) == '.') && (*( buf + 1 ) == '.') ) {
                printf( "%s %d: abs path %s\n", __func__, __LINE__, buf + 2 );
                memcpy( path, buf + 2, strlen( buf + 2 ) );
            }    
            else {
                memcpy( path, buf, strlen( buf ) );
            }
            ret = 0;
        }
    }
    else {
        printf( "%s %d: %s error, mode=0x%x\n", __func__, __LINE__, file, info.st_mode );
    }

    return ret;
}

int hal_net_set_dns( hal_network_dns_s *dns )
{
    struct sockaddr_in sa;
    int ret = -1;
    int primary = 0;
    int secondary = 0;
    char buf[128];
    char path[128];
    int count = 0;
    int fd = -1;

    if ( dns == NULL ) {
        printf( "%s %d: dns is null\n", __func__, __LINE__ );
        return ret;
    }
    printf("%s %d: dns_primary:%s dns_secondary=%s\n", __func__, __LINE__, dns->dns_primary, dns->dns_secondary );

    memset( buf, 0x0, sizeof( buf ) );
    if ( !hal_net_validate_ip( dns->dns_primary ) ) {
        printf("%s %d: not vaild ip, dns_primary: %s\n", __func__, __LINE__, dns->dns_primary );
    }
    else {
        primary = 1;
        snprintf( buf, 128, "nameserver %s\n", dns->dns_primary );
        ret = 0;
    } 

    if ( !hal_net_validate_ip( dns->dns_secondary ) ) {  
        printf("%s %d: not valid ip dns_secondary: %s\n", __func__, __LINE__, dns->dns_secondary );
    }
    else {
        secondary = 1;
        snprintf( buf + strlen( buf ), 128 - strlen( buf ), "nameserver %s\n", dns->dns_secondary );
        ret = 0;
    }

    if ( !primary && !secondary ) {
        ret = -1;
        goto exit_end; 
    }
    if ( hal_net_get_abs_path( DNS_CFG_PATH, path, sizeof( path ) ) < 0 ) {
        ret = -1;
        goto exit_end; 
    }

    if ( remove( DNS_CFG_BAK_PATH ) == -1 ) {
        printf( "%s %d: %s remove error, %d,%s \n", __func__, __LINE__, DNS_CFG_BAK_PATH, errno, strerror( errno ) );
    }

    fd = open( DNS_CFG_BAK_PATH, O_WRONLY | O_CREAT, 0600);
    if ( fd < 0 ) {
        printf("%s %d: failed to open file %s\n", __func__, __LINE__, DNS_CFG_PATH );
        ret = -1;
        goto exit_end; 
    }

    count = write( fd, buf, strlen( buf ) );
    if ( count != strlen( buf ) ) {
        printf("%s %d: failed to write file %s\n", __func__, __LINE__, DNS_CFG_PATH );
    }

    if ( fd ) {
        close( fd );
    }

    if ( rename( DNS_CFG_BAK_PATH, path ) == -1 ) {
        printf( "%s %d: error=%d,%s \n", __func__, __LINE__, errno, strerror( errno ) );
    }

exit_end:
    return ret;
}

int hal_net_get_dns( hal_network_dns_s * dns )
{
    int ret = -1;
    char *cfg_name = DNS_CFG_PATH;
    struct stat st;
    char *buf = NULL;
    int fd = -1;
    int fsize = 0;
    ssize_t sz;
    char *filter = "nameserver";
    char *primary = NULL;
    char *secondary = NULL;
    char ipaddr[16];

    if ( dns == NULL ) {
        printf( "%s %d: dns is null\n", __func__, __LINE__ );
        return -1;
    }

    if ( stat(cfg_name, &st ) == -1 ) {
        printf( "%s %d: failed to open dns config file\n", __func__, __LINE__ );
        goto end_exit1;
    }

    if ( st.st_size == 0 ) {
        printf( "%s %d: config file size is 0\n", __func__, __LINE__ );
        goto end_exit1;
    }

    fsize = st.st_size + 1;
    
    buf = malloc( fsize );
    if ( !buf ) {
        printf( "%s %d: failed to alloc\n", __func__, __LINE__ );
        goto end_exit1;

    }
    memset( buf, 0x0, fsize );
    
    fd = open( cfg_name, O_RDONLY ); if ( fd < 0 ) {
        printf( "%s %d: failed to open %s\n", __func__, __LINE__, cfg_name );
        goto end_exit2;
    }
     
    sz = read( fd, buf, st.st_size ); 
    if ( sz != st.st_size ) {
        printf( "%s %d: failed to read conf file %s\n", __func__, __LINE__, cfg_name);
        goto end_exit2;
    }

    if ( fd ) {
        close( fd );
    }

    primary = strstr( buf, filter );
    if ( !primary ) {
        printf( "%s %d: not found \n", __func__, __LINE__ );
        goto end_exit2;
    }
    else  {
        memset( ipaddr, 0x0, sizeof( ipaddr ) );
        if ( hal_net_get_ip_from_string( primary, ipaddr , sizeof( ipaddr ) ) == 0 ) {
            printf( "%s %d: dns_primary=%s\n", __func__, __LINE__, ipaddr );
            if ( hal_net_validate_ip( ipaddr ) ) {
                memcpy( dns->dns_primary, ipaddr, sizeof( ipaddr ) );
            }

        }
        ret = 0;
    }

    secondary = strstr( primary + strlen( filter ), filter );
    if ( !secondary ) {
        printf( "%s %d: not found \n", __func__, __LINE__ );
    }
    else {
        memset( ipaddr, 0x0, sizeof( ipaddr ) );
        if ( hal_net_get_ip_from_string( secondary, ipaddr , sizeof( ipaddr ) ) == 0 ) {
            printf( "%s %d: dns_secondary=%s\n", __func__, __LINE__, ipaddr );
            if ( hal_net_validate_ip( ipaddr ) ) {
                memcpy( dns->dns_secondary, ipaddr, sizeof( ipaddr ) );
            }

        }
        ret = 0;
    }
    
end_exit2:
    if ( buf ) {
        free( buf );
        buf = NULL;
    }

end_exit1:
    return ret;
}

int hal_net_get_all_interface( hal_network_iface_s *iface)
{

    FILE *fp = NULL;
    char *fpath="/proc/net/dev";
    char buf[512];
    int ret = -1;
    char ifname[IFNAMSIZ];
    int sk = -1;
    struct ifreq ifr; 
    char *tmp = NULL;


    if ( !iface ) {
        printf( "%s %d: arg is null\n", __func__, __LINE__ );
        return -1;
    }
 
    fp = fopen( fpath, "r");
    if ( !fp ) {
        printf( "%s %d: %s is not exist\n", __func__, __LINE__, fpath );
        return -1;
    }

    sk = socket( AF_INET, SOCK_DGRAM, 0 );
    if ( sk < 0 ) {
        printf( "%s %d: failed to create sk\n", __func__, __LINE__ );
        goto exit_end;
    }

    memset( iface, 0x0, sizeof( iface ) );
    fgets( buf, sizeof( buf ), fp ); 
    fgets( buf, sizeof( buf ), fp );
 
    while ( fgets( buf, sizeof( buf ), fp) ) {
        sscanf( buf, "%[^:]:", ifname );
        memset( &ifr, 0, sizeof( ifr ) );
        
        tmp = hal_net_space_trim( ifname );
        if ( !tmp ) {
            printf( "%s %d: %s error\n", __func__, __LINE__, ifname );
            continue;
        } 
        strncpy( ifr.ifr_name, ifname, sizeof(ifr.ifr_name) - 1 );
        ret = ioctl( sk, SIOCGIFFLAGS, &ifr );
        if ( ret < 0 ) {
            printf( "%s %d: %s ioctl error=%d,%s\n", __func__, __LINE__, ifname, errno, strerror( errno ) );
        }

        if ( ifr.ifr_ifru.ifru_flags & IFF_UP ) {
            memcpy( iface->ifaces[ iface->count ].ifname, tmp, IFNAMSIZ );
            printf( "%s %d: %d %s\n", __func__, __LINE__, iface->count, iface->ifaces[ iface->count ].ifname );
            iface->count ++;
        }
    }

    if ( sk ) {
        close( sk );
    }

exit_end:

    if ( fp ) {
        fclose( fp );
    }

    return ret;
}

int hal_net_interface_enable(const char *ifname, int enable )
{

    int sk = -1;
    struct ifreq ifr; 
    int flag = IFF_UP;
    int ret = -1;

    if ( !ifname ) {
        printf( "%s %d: ifname is null\n", __func__, __LINE__ );
        return -1;
    }

    sk = socket(AF_INET, SOCK_DGRAM, 0);
    if ( sk < 0 ) {
        printf( "%s %d: failed to create sk\n", __func__, __LINE__ );
        return -1;
    }

    memset( &ifr, 0, sizeof( ifr ) );
    strncpy( ifr.ifr_name, ifname, sizeof(ifr.ifr_name) - 1 );

    ret = ioctl( sk, SIOCGIFFLAGS, &ifr );
    if ( ret < 0 ) {
        printf( "%s %d: failed to get iterface info \n", __func__, __LINE__ );
        goto exit_end;

    }

    if ( enable ) {
        ifr.ifr_ifru.ifru_flags |= IFF_UP;
    }
    else {
        ifr.ifr_ifru.ifru_flags &= ( ~IFF_UP );
    }

    ret = ioctl( sk, SIOCSIFFLAGS, &ifr );
    if ( ret <  0) {
        printf( "%s %d: failed to set \n", __func__, __LINE__ );
        goto exit_end;
    }

exit_end:
    if ( sk ) {
        close( sk );
    }
    return 0;
}

int hal_net_set_dhcp( int enable, int argc, char **argv )
{
    char buf[512];
    int len = 0;
    int ret = -1;
    int size = sizeof( 512 ) - 1;
    int status = 0;
    char pid[8];
    char pid_path[128];
    FILE *fp;

    if ( argc <= 0 ) {
        printf( "%s %d: error argc=%d\n", __func__, __LINE__, argc );
        return -1;
    }

    memset( buf, 0x0, sizeof( buf ) );
    memset( pid_path, 0x0, sizeof( pid_path ) );
    sprintf( buf, "%s ", "udhcpc " );
    len = strlen( buf );
    printf( "%s %d: cmd=%s\n", __func__, __LINE__, buf );
    while( argc && *argv ) {
        if ( !enable && strcmp( *argv,  "-p" ) == 0 ) {
            if ( strlen( *( argv + 1) ) > sizeof( pid_path ) -1 ) {
                printf( "%s %d: error pid path=%s\n", __func__, __LINE__, *(argv + 1) );
                return -1;
            }
            printf( "%s %d: pid path=%s\n", __func__, __LINE__, *(argv + 1) );
            memcpy( pid_path, *( argv + 1), strlen( *(argv + 1) ) );
        }
        sprintf( buf + len, "%s ", *argv );
        len = strlen( buf );
        argv++;
        argc--;
    }
    printf( "%s %d: cmd=%s\n", __func__, __LINE__, buf );

    if ( !enable ) {
        if ( access( pid_path, F_OK ) != 0 ) {
            printf( "%s %d: pid path error %s\n", __func__, __LINE__, pid_path );
            return -1;
        }


        memset( pid, 0x0, sizeof( pid ) );
        memset( buf, 0x0, sizeof( buf ) );
        sprintf( buf, "cat %s", pid_path );
        
        fp = popen( buf, "r" );
        if ( !fp ) {
            printf( "%s %d: open error=%d,%s\n", __func__, __LINE__, errno, strerror( errno ) );
            return -1;
        }
        fgets( pid, sizeof( pid ), fp );
        pclose( fp );
        memset( buf, 0x0, sizeof( buf ) );
        sprintf( buf, "kill -9 %s", pid );
        printf( "%s %d: cmd=%s\n", __func__, __LINE__, buf );
    }

    status = system( buf );
    if ( status == -1 ) {    
        printf( "%s %d: cmd failed\n", __func__, __LINE__ );
    }    
    else {    
        if( WIFEXITED( status ) ) {    
            if( 0 == WEXITSTATUS( status ) ) {    
                printf( "%s %d: cmd success\n", __func__, __LINE__ );
                ret = 0;    
            }    
            else {
                printf( "%s %d: cmd failed\n", __func__, __LINE__ );
                ret = -1;
            }
        }
        else {
            printf( "%s %d: failed exit code %d\n", __func__, __LINE__ ,WEXITSTATUS( status ) );
            ret = -1;
        }
    }

    return ret;
}

int hal_net_set_default_interface( const char *ifname )
{
    hal_network_ip_s ip;
    int sk;
    int ret = 0;
    struct ifreq ifr; 
    struct sockaddr_in *sin;
    struct rtentry  rt;

    if ( !ifname ) {
        printf( "%s %d: arg is null \n", __func__, __LINE__ );
        return -1;
    }

    sk = socket( AF_INET, SOCK_DGRAM, 0 );
    if( sk < 0 ) {
        printf( "%s %d: failed to create sk\n", __func__, __LINE__ );
        return -1;     
    }

    memset( &ip, 0x0, sizeof( ip ) );
    if ( hal_net_get_default_gateway( &ip ) < 0 ) {
        ret = -1;
        goto exit_end;
    }

    memset( &rt, 0, sizeof( struct rtentry ) );
    sin = (struct sockaddr_in *)&rt.rt_gateway;    
    sin->sin_family = AF_INET;    
    if ( inet_aton(ip.gw, &(sin->sin_addr)) != 0 ) {    

        sin = (struct sockaddr_in *)&rt.rt_dst;    
        sin->sin_family = AF_INET;    
        sin->sin_addr.s_addr = INADDR_ANY;    

        sin = (struct sockaddr_in*)&rt.rt_genmask;    
        sin->sin_family = AF_INET;    
        sin->sin_addr.s_addr = INADDR_ANY;    


        if ( ioctl(sk, SIOCDELRT, &rt ) < 0) {    
            printf( "%s %d: gateway iotcl %s, error=%d,%s\n", __func__, __LINE__, ip.gw, errno, strerror( errno ) );
            if ( errno == ESRCH ) {
                ret = 0;
            }
            else {
                ret = -1;
                goto exit_end;
            }
        }    

        rt.rt_flags = (RTF_UP | RTF_GATEWAY);    
        rt.rt_dev = (char *)ifname;    

        if ( ioctl(sk, SIOCADDRT, &rt ) < 0) {    
            printf( "%s %d: gateway iotcl %s, error=%d,%s\n", __func__, __LINE__, ip.gw, errno, strerror( errno ) );
            if ( errno == EEXIST ) {
                ret = 0;
            }
            else {
                ret = -1;
            }
        }    

    }    

exit_end:
    if ( sk )
        close( sk );
    return ret;
}

int hal_net_conf_route( int argc, char **argv )
{
    char buf[512];
    int len = 0;
    int ret = -1;
    int size = sizeof( 512 ) - 1;
    int status = 0;

    if ( argc <= 0 ) {
        printf( "%s %d: error argc=%d\n", __func__, __LINE__, argc );
        return -1;
    }

    memset( buf, 0x0, sizeof( buf ) );
    sprintf( buf, "%s ", "route" );
    len = strlen( buf );
    printf( "%s %d: cmd=%s\n", __func__, __LINE__, buf );
    while( argc && *argv ) {
        sprintf( buf + len, "%s ", *argv );
        len = strlen( buf );
        argv++;
        argc--;
    }
    printf( "%s %d: cmd=%s\n", __func__, __LINE__, buf );
    status = system( buf );
    if ( status == -1 ) {    
        printf( "%s %d: cmd failed\n", __func__, __LINE__ );
    }    
    else {    
        if( WIFEXITED( status ) ) {    
            if( 0 == WEXITSTATUS( status ) ) {    
                printf( "%s %d: cmd success\n", __func__, __LINE__ );
                ret = 0;    
            }    
            else {
                printf( "%s %d: cmd failed\n", __func__, __LINE__ );
                ret = -1;
            }
        }
        else {
            printf( "%s %d: failed exit code %d\n", __func__, __LINE__ ,WEXITSTATUS( status ) );
            ret = -1;
        }
    }

    return ret;
}

int hal_net_get_route( char *buf, int size )
{
    FILE *fp;
    char * str = NULL;
    int len = 0;
    if ( !buf || size <= 128 ) {
        printf( "%s %d: args is err, buf=%p, size=%d\n", __func__, __LINE__, buf, size );
        return -1;
    }
    fp = popen( "route -n ", "r" );
    if ( !fp ) {
        printf( "%s %d: open error=%d,%s\n", __func__, __LINE__, errno, strerror( errno ) );
        return -1;
    }

    while ( 1 ) {
        str = fgets( buf + len, size - len - 1, fp );
        if ( str ) {
            len = strlen( buf );
        }
        else {
            break;
        }
    }
    printf( "%s %d: route table:\n %s\n", __func__, __LINE__, buf );
    if ( fp )
        pclose(fp);

    return 0;
}



