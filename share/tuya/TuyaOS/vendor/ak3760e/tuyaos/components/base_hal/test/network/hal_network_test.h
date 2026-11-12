/*
 * Copyright (C) 2021 Tuya Inc.
 */

#ifndef __HAL_NETWORK_TEST_H
#define __HAL_NETWORK_TEST_H

typedef int ( *sub_menu_handle )( void *cmd_opt,  int argc, char **argv );

typedef struct args_menu {
    char *m_cmd;
    char *m_usage;
    char *s_usage;
    sub_menu_handle s_handle;
    struct option *long_options;
}arg_menu_s;

static int s_set_ip( void *cmd_opt, int argc, char **argv );
static int s_get_ip( void *cmd_opt, int argc, char **argv );
static int s_set_dns( void *cmd_opt, int argc, char **argv );
static int s_get_dns( void *cmd_opt, int argc, char **argv );
static int s_get_all_iface( void *cmd_opt, int argc, char **argv );
static int s_enable_iface( void *cmd_opt, int argc, char **argv );
static int s_get_route( void *cmd_opt, int argc, char **argv );
static int s_conf_route( void *cmd_opt, int argc, char **argv );
static int s_conf_dhcp( void *cmd_opt, int argc, char **argv );
static int s_set_default_iface( void *cmd_opt, int argc, char **argv );
#endif
