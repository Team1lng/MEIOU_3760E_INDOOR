/*
 * Copyright (C) 2021 Tuya Inc.
 */

#include <stdarg.h>
#include <stdio.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/reboot.h>
#include <pthread.h>
#include <time.h>
#include <sys/times.h>
#include <sys/select.h>
#include <time.h>
#include "bcmnvram.h"
#include "hal_log.h"
#include "hal_misc.h"
#include "hal_types.h"
#include "hal_file.h"
#include "hal_board.h"
#include "hal_config.h"
#include "hal_modem_4g.h"
#include "hal_modem_4g_core.h"

#define AT_RETRY_COUNT			5
#define AT_CMD_RESULT_OK		"OK"
#define AT_CMD_RECV_BUF_LEN		256

#define MODULE_NAME_NL668		"NL668"
#define MODULE_NAME_EC21		"EC21"
#define MODULE_NAME_EC200S		"EC200S"
#define MODULE_NAME_EC200U		"EC200U"
#define MODULE_NAME_BG95		"BG95"
#define MODULE_NAME_LZ201		"LZ201"

#define USB_COMMAND				"lsusb"
#define LS_OPTION1_CMD			"ls /sys/bus/usb-serial/drivers/option1/"

/*
 * AT command list
 */
static MODEM_CMD_T *cmds_tbl_modem = NULL;

/* EC21 Proprietary Commands */
static MODEM_CMD_T cmds_tbl_ec21[] = {
	{AT_CMD_USB_MODE, "AT+QCFG=\"usbnet\"", "+QCFG: \"usbnet\",1", "+QCFG: \"usbnet\",%d"},
	{AT_CMD_ECM_MODE, "AT+QCFG=\"usbnet\",1", NULL, NULL},
	{AT_CMD_SIM_HOT, "AT+QSIMDET=1,1", NULL, NULL},
	{AT_CMD_RNDIS, NULL, NULL, NULL},
	{AT_CMD_DATA_CALL, NULL, NULL, NULL},
	{AT_CMD_USER, "AT+QICSGP", NULL, NULL, (void*)3},
	{AT_CMD_DTMF, "AT+QTONEDET", "+QTONEDET:", "+QTONEDET: %d"},
	{AT_CMD_CALL_STATE, "AT+QENG=\"SERVINGCELL\"", "+QENG:", "CONNECT"},
};

/* EC200S Proprietary Commands */
static MODEM_CMD_T cmds_tbl_ec200s[] = {
	{AT_CMD_USB_MODE, "AT+QCFG=\"usbnet\"", "+QCFG: \"usbnet\",1", "+QCFG: \"usbnet\",%d"},
	{AT_CMD_ECM_MODE, "AT+QCFG=\"usbnet\",1", NULL, NULL},
	{AT_CMD_SIM_HOT, "AT+QSIMDET=1,1", NULL, NULL},
	{AT_CMD_RNDIS, NULL, NULL, NULL},
	{AT_CMD_DATA_CALL, "AT+QNETDEVCTL", NULL, NULL},
	{AT_CMD_USER, "AT+QICSGP", NULL, NULL, (void*)3},
	{AT_CMD_DTMF, NULL, NULL, NULL},
	{AT_CMD_CALL_STATE, "AT+QENG=\"SERVINGCELL\"", "+QENG:", "CONNECT"},
};

/* EC200U Proprietary Commands */
static MODEM_CMD_T cmds_tbl_ec200u[] = {
	{AT_CMD_USB_MODE, "AT+QCFG=\"usbnet\"", "+QCFG: \"usbnet\",1", "+QCFG: \"usbnet\",%d"},
	{AT_CMD_ECM_MODE, "AT+QCFG=\"usbnet\",1", NULL, NULL},
	{AT_CMD_SIM_HOT, "AT+QSIMDET=1,1", NULL, NULL},
	{AT_CMD_RNDIS, NULL, NULL, NULL},
	{AT_CMD_DATA_CALL, "AT+QNETDEVCTL", NULL, NULL},
	{AT_CMD_USER, "AT+QICSGP", NULL, NULL, (void*)3},
	{AT_CMD_DTMF, NULL, NULL, NULL},
	{AT_CMD_CALL_STATE, "AT+QENG=\"SERVINGCELL\"", "+QENG:", "CONNECT"},
};

/* BG95 Proprietary Commands */
static MODEM_CMD_T cmds_tbl_bg95[] = {
	{AT_CMD_USB_MODE, "AT+QCFGEXT=\"usbnet\"", "+QCFGEXT: \"usbnet\",\"ecm\"", "+QCFGEXT: \"usbnet\",\"%s\""},
	{AT_CMD_ECM_MODE, "AT+QCFGEXT=\"usbnet\",\"ecm\"", NULL, NULL},
	{AT_CMD_SIM_HOT, "AT+QSIMDET=1,1", NULL, NULL},
	{AT_CMD_RNDIS, NULL, NULL, NULL},
	{AT_CMD_DATA_CALL, NULL, NULL, NULL},
	{AT_CMD_USER, "AT+QICSGP", NULL, NULL, (void*)3},
	{AT_CMD_DTMF, NULL, NULL, NULL},
	{AT_CMD_CALL_STATE, NULL, NULL, NULL},
};

/* NL668 Proprietary Commands */
static MODEM_CMD_T cmds_tbl_nl668[] = {
	{AT_CMD_USB_MODE, "AT+GTUSBMODE?", "+GTUSBMODE: 18", "+GTUSBMODE: %d"},
	{AT_CMD_ECM_MODE, "AT+GTUSBMODE=18", NULL, NULL},
	{AT_CMD_SIM_HOT, "AT+MSMPD=1", NULL, NULL},
	{AT_CMD_RNDIS, "AT+GTRNDIS", "+GTRNDIS:", "+GTRNDIS: %d"},
	{AT_CMD_DATA_CALL, NULL, NULL, NULL},
	{AT_CMD_USER, "AT+MGAUTH", NULL, NULL, (void*)2},
	{AT_CMD_DTMF, "AT+GTDTMF", "+GTDTMF:", "+GTDTMF: %d"},
	{AT_CMD_CALL_STATE, NULL, NULL, NULL},
};

/* LZ201 Proprietary Commands */
static MODEM_CMD_T cmds_tbl_lz201[] = {
	{AT_CMD_USB_MODE, "AT+SYSNV=0,\"usbmode\"", "+SYSNV: \"usbmode\",5", NULL},
	{AT_CMD_ECM_MODE, "AT+SYSNV=1,\"usbmode\",5", NULL, NULL},
	{AT_CMD_SIM_HOT, NULL, NULL, NULL},
	{AT_CMD_RNDIS, NULL, NULL, NULL},
	{AT_CMD_DATA_CALL, NULL, NULL, NULL},
	{AT_CMD_USER, NULL, NULL, NULL},
	{AT_CMD_DTMF, NULL, NULL, NULL},
	{AT_CMD_CALL_STATE, NULL, NULL, NULL},
};

MODEM_INFO_T modem_table[] = {
	{2, 0x2C7C, 0x0121, MODULE_NAME_EC21, cmds_tbl_ec21}, //EC21
	{1, 0x2C7C, 0x6002, MODULE_NAME_EC200S, cmds_tbl_ec200s}, //EC200S
	{0, 0x2C7C, 0x0901, MODULE_NAME_EC200U, cmds_tbl_ec200u}, //EC200U
	{2, 0x2C7C, 0x0700, MODULE_NAME_BG95, cmds_tbl_bg95}, //BG95
	{2, 0x1508, 0x1000, MODULE_NAME_NL668, cmds_tbl_nl668}, //NL668
	{2, 0x1508, 0x1001, MODULE_NAME_NL668, cmds_tbl_nl668}, //NL668
	{0, 0x1782, 0x4D10, MODULE_NAME_LZ201, cmds_tbl_lz201}, //LZ201
	{0, 0x1782, 0x4D11, MODULE_NAME_LZ201, cmds_tbl_lz201}, //LZ201
	{0, 0x1782, 0x4D12, MODULE_NAME_LZ201, cmds_tbl_lz201}, //LZ201
	{0, 0x1782, 0x4D13, MODULE_NAME_LZ201, cmds_tbl_lz201}, //LZ201
	{0, 0x1782, 0x4D15, MODULE_NAME_LZ201, cmds_tbl_lz201}, //LZ201
};

/*
 * For generic instance, set all the members to NULL
 */
hal_if_modem_4g_t hal_if_modem_4g = {0};

static pthread_mutex_t tty_mutex = PTHREAD_MUTEX_INITIALIZER;

static int exec_cmd_with_ret(const char *cmd, char ret_buf[], int ret_len)
{
	FILE *stream;
	int ret;
	int len;

	stream = popen(cmd, "r");

	if (NULL == stream) {
		hal_log(HAL_LOG_ERR,"popen fail\n");
		return -1;
	}

	int fd = fileno(stream);
	int flags = fcntl(fd, F_GETFL, 0);
	flags |= (O_NONBLOCK);

	if (fcntl(fd, F_SETFL, flags) < 0) {
		perror("fcntl:");
		goto end_handle;
	}

	fd_set readfds;
	struct timeval timeout;

	FD_ZERO(&readfds);

	timeout.tv_sec = 5;
	timeout.tv_usec = 0;

	FD_SET(fd, &readfds);

	ret = select(fd+1, &readfds, NULL, NULL, &timeout);
	if (ret < 0) {
		hal_log(HAL_LOG_ERR, "%s:select failed\n", cmd);
		goto end_handle;
	} else if (ret == 0) {
		hal_log(HAL_LOG_ERR, "%s:select timeout\n", cmd);
		goto end_handle;
	} else {
		if (FD_ISSET(fd, &readfds)) {
			if (NULL != ret_buf) {
				memset(ret_buf, 0, ret_len);
				fread(ret_buf, ret_len, 1, stream);
			}

			if (NULL != ret_buf) {
				len = strlen(ret_buf);
				if ('\n' == ret_buf[len - 1]) {
					ret_buf[len - 1] = '\0';
				}
			}
		}
	}

end_handle:
	ret = pclose(stream);
	if (-1 == ret) {
		hal_log(HAL_LOG_ERR, "pclose fail\n");
		return -1;
	}

	return 0;
}

static int modem_4g_get_info(MODEM_INFO_T *info)
{
	char *p;
	int i, mnum;
	int buf_len;
	int vid, pid;
	char buf[4096] = {0};

	if (info == NULL) {
		hal_log(HAL_LOG_ERR, "%s:invalid parm\n", __func__);
		return -1;
	}

	if (exec_cmd_with_ret(USB_COMMAND, buf, sizeof(buf)))
		return -1;

	buf_len = strlen(buf) +1;
	p = &buf[0];

	for (i = 0; i < buf_len; i++) {
		char *at;

		if(buf[i] != '\n' && buf[i] != '\0')
			continue;

		buf[i] = 0;

		at = strstr(p, "ID ");
		if (at)
			sscanf(at, "ID %x:%x", &vid, &pid);

		for (mnum = 0; mnum < MAX_DEVICE; mnum++) {
			if (vid == modem_table[mnum].vid && pid == modem_table[mnum].pid) {
				info->vid = vid;
				info->pid = pid;
				info->off = modem_table[mnum].off;
				strncpy(info->name, modem_table[mnum].name, sizeof(info->name) - 1);
				cmds_tbl_modem = modem_table[mnum].cmd_table;
				return 0;
			}
		}
		p = &buf[i+1];
	}

	hal_log(HAL_LOG_ERR,"%s: no matching device found\n", __func__);
	return -1;
}

static int modem_4g_at_port_set(hal_modem_4g_dev_attr_t *attr)
{
	int tty_num = 0;
	char *t_buf = NULL;
	char buf[4096] = {0};
	MODEM_INFO_T info = {0};

	if (exec_cmd_with_ret(LS_OPTION1_CMD, buf, sizeof(buf)))
        return -1;

	t_buf = strstr(buf, "ttyUSB");
	if (t_buf) {
		sscanf(t_buf, "ttyUSB%d", &tty_num);
		hal_log(HAL_LOG_INFO, "modem first ttyUSB num:%d\n", tty_num);
	}

	/* Get module information through lsusb instruction */
	if (modem_4g_get_info(&info))
		return -1;

	/* Set at command port */
	tty_num += info.off;
	memset(attr->at_port, 0, sizeof(attr->at_port));
	sprintf(attr->at_port, "/dev/ttyUSB%d", tty_num);
	hal_log(HAL_LOG_INFO, "AT port is %s\n", attr->at_port);

	/* Save module name */
	memset(attr->module_name, 0, sizeof(attr->module_name));
	strncpy(attr->module_name, info.name, sizeof(attr->module_name) - 1);
	hal_log(HAL_LOG_INFO, "module name is %s\n", attr->module_name);

	return 0;
}

static int modem_xfer(int fd, unsigned char *send_buf, unsigned char *recv_buf)
{
	int n;
	int iret=0;
	fd_set read_set;

	struct timeval timeout = {
	    .tv_sec = 6,
	    .tv_usec = 0
	};

	pthread_mutex_lock(&tty_mutex);
	iret = write(fd,send_buf,strlen(send_buf));

	if(iret < 0){
		hal_log(HAL_LOG_ERR,"write at command port failed\n");
		pthread_mutex_unlock(&tty_mutex);
		return -1;
	}

	usleep(100000);

	FD_ZERO(&read_set);
	FD_SET(fd,&read_set);

	if (select(fd+1, &read_set,NULL,NULL,&timeout) <= 0){
		hal_log(HAL_LOG_ERR,"respond timeout\n");
		pthread_mutex_unlock(&tty_mutex);
		return -1;
	}

	if(FD_ISSET(fd,&read_set)){
		if((n=read(fd,recv_buf,AT_CMD_RECV_BUF_LEN-1)) <= 0){
			hal_log(HAL_LOG_ERR,"read at command port failed\n");
			pthread_mutex_unlock(&tty_mutex);
			return -1;
		}
		iret = 0;
	}

	pthread_mutex_unlock(&tty_mutex);
	return iret;
}

static int modem_recv(int fd,unsigned char *buf,int size,int time)
{
	int n;
	fd_set fds;
	int iret = 0;
	struct timeval timeout;

	if (buf == NULL || size < 0 || time < 0) {
		hal_log(HAL_LOG_ERR,"invalid paramter\n");
		return -1;
	}

	timeout.tv_sec  = time;
	timeout.tv_usec = 1000000;

	FD_ZERO(&fds);
	FD_SET(fd, &fds);

	iret = select(fd + 1, &fds, NULL, NULL, &timeout);
	if (iret <= 0) {
		hal_log(HAL_LOG_ERR,"respond timeout, iret:%d\n", iret);
		return iret;
	}

	if (FD_ISSET(fd, &fds)) {
		pthread_mutex_lock(&tty_mutex);
		n =  read(fd, buf, size - 1);
		if (n <= 0) {
			hal_log(HAL_LOG_ERR,"read data failed\n");
			pthread_mutex_unlock(&tty_mutex);
			return -1;
		}
		hal_log(HAL_LOG_INFO,"Recv %d bytes:%s\n", n, buf);
		pthread_mutex_unlock(&tty_mutex);
	}

	return n;
}

static int modem_send_recv(int fd, unsigned char *send_buf, unsigned char *recv_buf)
{
	int ret;
	char *str;
	
	if(send_buf == NULL || recv_buf == NULL){
		hal_log(HAL_LOG_ERR,"%s invalid paramter\n",__func__);
		return -1;
	}

	/* Refresh buffered data */
	modem_xfer(fd, "AT\r\n", recv_buf);

	memset(recv_buf,0,AT_CMD_RECV_BUF_LEN);
	ret = modem_xfer(fd,send_buf,recv_buf);
	if (strstr(send_buf,"AT+CMGS")) {
		str = strstr(recv_buf,">");
	} else {
		str = strstr(recv_buf,AT_CMD_RESULT_OK);
	}
	if(( ret == 0) && (str != NULL)){
		return 0;
	}else{
		return -1;
	}
}

static int modem_send_then_recv(int fd, unsigned char *send_buf, unsigned char *recv_buf)
{
	int i = 0;
	int ret = -1;

	sprintf(send_buf, "%s\r\n", send_buf);
	hal_log(HAL_LOG_INFO, "at cmd:%s", send_buf);
	while (i < AT_RETRY_COUNT) {
		ret = modem_send_recv(fd, send_buf, recv_buf);
		if (ret == 0)
			break;
		i++;
		hal_log(HAL_LOG_ERR, "retry %d cmd: %s\n", i, send_buf);
		usleep(1000000);
	}
	hal_log(HAL_LOG_INFO, "return:%s\n", recv_buf);

	if (ret)
		return -1;

	return 0;
}

static void modem_4g_at_cmd_table_init(hal_modem_4g_dev_attr_t *attr)
{
	/* General Commands */
	memcpy(attr->at_cmd_table[AT_CMD_AT], "AT", strlen("AT"));
	memcpy(attr->at_cmd_table[AT_CMD_ATE], "ATE", strlen("ATE"));
	memcpy(attr->at_cmd_table[AT_CMD_CGMM], "AT+CGMM", strlen("AT+CGMM"));
	memcpy(attr->at_cmd_table[AT_CMD_CGSN], "AT+CGSN", strlen("AT+CGSN"));
	memcpy(attr->at_cmd_table[AT_CMD_CFUN], "AT+CFUN", strlen("AT+CFUN"));
	memcpy(attr->at_cmd_table[AT_CMD_CMEE], "AT+CMEE", strlen("AT+CMEE"));
	memcpy(attr->at_cmd_table[AT_CMD_CIMI], "AT+CIMI", strlen("AT+CIMI"));
	memcpy(attr->at_cmd_table[AT_CMD_CCID], "AT+CCID", strlen("AT+CCID"));
	memcpy(attr->at_cmd_table[AT_CMD_CPIN], "AT+CPIN", strlen("AT+CPIN"));
	memcpy(attr->at_cmd_table[AT_CMD_COPS], "AT+COPS", strlen("AT+COPS"));
	memcpy(attr->at_cmd_table[AT_CMD_CREG], "AT+CREG?", strlen("AT+CREG?"));
	memcpy(attr->at_cmd_table[AT_CMD_CSQ], "AT+CSQ", strlen("AT+CSQ"));
	memcpy(attr->at_cmd_table[AT_CMD_CGDCONT], "AT+CGDCONT", strlen("AT+CGDCONT"));
	memcpy(attr->at_cmd_table[AT_CMD_CGACT], "AT+CGACT", strlen("AT+CGACT"));
	memcpy(attr->at_cmd_table[AT_CMD_CGPADDR], "AT+CGPADDR", strlen("AT+CGPADDR"));
	memcpy(attr->at_cmd_table[AT_CMD_CGREG], "AT+CGREG?", strlen("AT+CGREG?"));
	memcpy(attr->at_cmd_table[AT_CMD_CEREG], "AT+CEREG?", strlen("AT+CEREG?"));
	memcpy(attr->at_cmd_table[AT_CMD_CSCS], "AT+CSCS", strlen("AT+CSCS"));
	memcpy(attr->at_cmd_table[AT_CMD_CMGF], "AT+CMGF", strlen("AT+CMGF"));
	memcpy(attr->at_cmd_table[AT_CMD_CMGS], "AT+CMGS", strlen("AT+CMGS"));

	memcpy(attr->at_cmd_table[AT_CMD_COLP], "AT+COLP", strlen("AT+COLP"));
	memcpy(attr->at_cmd_table[AT_CMD_CHUP], "AT+CHUP", strlen("AT+CHUP"));
	memcpy(attr->at_cmd_table[AT_CMD_ATD], "ATD", strlen("ATD"));
	memcpy(attr->at_cmd_table[AT_CMD_CNUM], "AT+CNUM", strlen("AT+CNUM"));
	memcpy(attr->at_cmd_table[AT_CMD_CGMR], "AT+CGMR", strlen("AT+CGMR"));
}

static int modem_4g_get_dev_attr(hal_modem_4g_dev_attr_t *attr)
{
    char buf[MAX_PATH] = {0};
    int baud;
    int i,ret;

    /* default serial attribute: 8N1N */
    int databits = 8;
    hal_serial_parity_e parity = HAL_SERIAL_P_NONE;
    hal_serial_stopbits_e stopbits = HAL_SERIAL_STOP_1;
    hal_serial_flowctrl_e flowctrl = HAL_SERIAL_FC_NONE;

    if (NULL == attr) {
        return -1;
    }

    // get 4g device attributes

    memset(buf, 0, sizeof(buf));
    ret = hal_cfg_get_item_string(ITEM_MODEM_4G, KEY_MODEM_4G_NET_DEV, buf, sizeof(buf));
    if (ret != 0) {
        hal_log(HAL_LOG_ERR, "Failed to get net_dev of %s\n", ITEM_MODEM_4G);
        return -1;
    }
    memset(attr->net_dev, 0, sizeof(attr->net_dev));
    strncpy(attr->net_dev, buf, sizeof(attr->net_dev) - 1);

	ret = modem_4g_at_port_set(attr);
	if (ret != 0) {
		hal_log(HAL_LOG_ERR, "Failed to set at_ports\n");
		return -1;
	}

    memset(buf,0,sizeof(buf));
    ret = hal_cfg_get_item_int(ITEM_MODEM_4G, KEY_MODEM_4G_AT_PORT_BAUD, &baud);
    if (ret != 0) {
        hal_log(HAL_LOG_ERR, "Failed to get baud of %s\n", ITEM_MODEM_4G);
        return -1;
    }
    attr->at_port_attr.baud = baud;
    attr->at_port_attr.databits = databits;
    attr->at_port_attr.parity = parity;
    attr->at_port_attr.stopbits = stopbits;
    attr->at_port_attr.flowctrl = flowctrl;

    //load at cmd table
	modem_4g_at_cmd_table_init(attr);
    
    return 0;
}

static int modem_4g_set_dev_attr(hal_modem_4g_dev_attr_t *attr)
{
    struct termios ttyAttr;
    speed_t speed;

    attr->at_uart_fd = open(attr->at_port,O_RDWR|O_NOCTTY,0);
    if(attr->at_uart_fd < 0){
	hal_log(HAL_LOG_INFO,"Failed to open %s\n",attr->at_port);
	return -1;
    }

    memset(&ttyAttr,0,sizeof(struct termios));
    tcgetattr(attr->at_uart_fd,&ttyAttr);

    switch(attr->at_port_attr.baud)
    {
	case 1800:
		speed = B1800;break;
	case 2500:
		speed = B2400;break;
	case 4800:
		speed = B4800;break;
	case 9600:
		speed = B9600;break;
	case 38400:
		speed = B38400;break;
	case 57600:
		speed = B57600;break;
	case 115200:
		speed = B115200;break;
	default:
		speed = B9600;break;
    }
    cfsetospeed(&ttyAttr,speed);
    cfsetispeed(&ttyAttr,speed);

    ttyAttr.c_cflag |= CREAD | CLOCAL;
    ttyAttr.c_cflag &= ~PARENB;
    ttyAttr.c_cflag &= ~CSTOPB;
    ttyAttr.c_cflag &= ~CSIZE;
    ttyAttr.c_cflag |= CS8;
    ttyAttr.c_cflag |= HUPCL;
    ttyAttr.c_cflag &= ~OPOST;
    ttyAttr.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    ttyAttr.c_cc[VMIN] = 1;

    if(tcsetattr(attr->at_uart_fd,TCSANOW,&ttyAttr) < 0)
    {
    	hal_log(HAL_LOG_ERR,"Failed to set at uart port\n");
	return -1;
    }

    return 0;
}

static int modem_4g_at_cmd_test(hal_modem_4g_dev_attr_t *attr)
{
	int ret = 0;
	char cmd_table[32] = {0};

	sprintf(cmd_table, "%s", attr->at_cmd_table[AT_CMD_AT]);
	ret = modem_send_then_recv(attr->at_uart_fd,cmd_table,attr->at_recv_buf);
	if (ret != 0) {
		hal_log(HAL_LOG_ERR,"modem_4g at cmd test failed\n");
		return -1;
	}

	return 0;
}

static int modem_name_is(hal_modem_4g_dev_attr_t *attr, char *name)
{
	if (strstr(attr->module_name, name))
		return 1;

	return 0;
}

/* set command echo mode, enable=0:OFF enable=1:ON */
static int modem_4g_at_cmd_echo_set(hal_modem_4g_dev_attr_t *attr, int enable)
{
	int ret = 0;
	char cmd_table[32] = {0};

	sprintf(cmd_table, "%s%d", attr->at_cmd_table[AT_CMD_ATE], enable);
	ret = modem_send_then_recv(attr->at_uart_fd,cmd_table,attr->at_recv_buf);
	if (ret != 0) {
		hal_log(HAL_LOG_ERR,"modem_4g echo set failed\n");
		return -1;
	}

	return 0;
}

/* set the format of the command error result code*/
static int modem_4g_at_cmd_cmee_set(hal_modem_4g_dev_attr_t *attr, int mode)
{
	int ret = 0;
	char cmd_table[32] = {0};

	sprintf(cmd_table, "%s=%d", attr->at_cmd_table[AT_CMD_CMEE], mode);
	ret = modem_send_then_recv(attr->at_uart_fd,cmd_table,attr->at_recv_buf);
	if (ret != 0) {
		hal_log(HAL_LOG_ERR,"modem_4g error result code set failed\n");
		return -1;
	}

	return 0;
}

/* Check the mode of usb; USB_NOECM_MODE:Non-ecm mode, USB_ECM_MODE:ecm mode */
static int modem_4g_usb_mode_check(hal_modem_4g_dev_attr_t *attr, int *mode)
{
	int ret;
	char *str;
	char cmd_table[32] = {0};
	MODEM_CMD_T *modem_cmd = &cmds_tbl_modem[AT_CMD_USB_MODE];

	if (modem_cmd->index != AT_CMD_USB_MODE) {
		hal_log(HAL_LOG_ERR,"cmd not find, index:%d\n", AT_CMD_USB_MODE);
		return -1;
	}
	if (modem_cmd->at_cmd == NULL)
		return NOT_SUPPORT;

	sprintf(cmd_table, "%s", modem_cmd->at_cmd);
	ret = modem_send_then_recv(attr->at_uart_fd, cmd_table, attr->at_recv_buf);
	str = strstr(attr->at_recv_buf, modem_cmd->result_flag);
	if (ret != 0) {
		hal_log(HAL_LOG_ERR,"get usb mode failed\n");
		return -1;
	} else if (str != NULL) {
		*mode = USB_ECM_MODE;
	} else {
		*mode = USB_NOECM_MODE;
	}

	return 0;
}

/* usb mode is set to ecm mode */
static int modem_4g_usb_ecm_mode_set(hal_modem_4g_dev_attr_t *attr)
{
	int ret;
	char cmd_table[32] = {0};
	MODEM_CMD_T *modem_cmd = &cmds_tbl_modem[AT_CMD_ECM_MODE];

	if (modem_cmd->index != AT_CMD_ECM_MODE) {
		hal_log(HAL_LOG_ERR,"cmd not find, index:%d\n", AT_CMD_ECM_MODE);
		return -1;
	}
	if (modem_cmd->at_cmd == NULL)
		return NOT_SUPPORT;

	sprintf(cmd_table, "%s", modem_cmd->at_cmd);
	ret = modem_send_then_recv(attr->at_uart_fd,cmd_table,attr->at_recv_buf);
	if (ret != 0) {
		hal_log(HAL_LOG_ERR, "set usb to ecm mode failed\n");
		return -1;
	}
	
	return 0;
}

/* Check RNDIS enable status */
static int modem_4g_rndis_check(hal_modem_4g_dev_attr_t *attr,int *state)
{
	int ret = -1;;
	char *str = NULL;
	char cmd_table[32] = {0};
	MODEM_CMD_T *modem_cmd = &cmds_tbl_modem[AT_CMD_RNDIS];

	if (modem_cmd->index != AT_CMD_RNDIS) {
		hal_log(HAL_LOG_ERR,"cmd not find, index:%d\n", AT_CMD_RNDIS);
		return -1;
	}
	if (modem_cmd->at_cmd == NULL)
		return NOT_SUPPORT;

	sprintf(cmd_table, "%s?", modem_cmd->at_cmd);
	ret = modem_send_then_recv(attr->at_uart_fd, cmd_table, attr->at_recv_buf);
	str = strstr(attr->at_recv_buf, modem_cmd->result_flag);
	if ((ret == 0) && (str != NULL)) {
		sscanf(str, modem_cmd->result_format, state);
		hal_log(HAL_LOG_INFO, "get RNDIS: %d\n", *state);
	} else {
		hal_log(HAL_LOG_ERR, "get RNDIS failed\n");
		return -1;
	}

	return 0;
}

/* enable/disable RNDIS function */
static int modem_4g_rndis_enable(hal_modem_4g_dev_attr_t *attr, int enable)
{
	int ret = -1;
	char cmd_table[32] = {0};
	MODEM_CMD_T *modem_cmd = &cmds_tbl_modem[AT_CMD_RNDIS];

	if (modem_cmd->index != AT_CMD_RNDIS) {
		hal_log(HAL_LOG_ERR,"cmd not find, index:%d\n", AT_CMD_RNDIS);
		return -1;
	}
	if (modem_cmd->at_cmd == NULL)
		return NOT_SUPPORT;

	sprintf(cmd_table, "%s=%d,1", modem_cmd->at_cmd, enable);
	ret = modem_send_then_recv(attr->at_uart_fd, cmd_table, attr->at_recv_buf);
	if (ret != 0) {
		hal_log(HAL_LOG_ERR, "RNDIS set failed\n");
		return -1;
	}

	return 0;	
}

/* RNDIS configuration */
static int modem_4g_rndis_set(hal_modem_4g_dev_attr_t *attr, int enable)
{
	int ret = -1;
	int state = 0;

	ret = modem_4g_rndis_check(attr,&state);
	if (ret != 0) {
		return ret;
	}
	if (state == enable) {
		hal_log(HAL_LOG_INFO,"RNDIS already set,no need to set\n");
		return 0;
	}

	ret = modem_4g_rndis_enable(attr, enable);
	if (ret != 0) {
		return ret;
	}

	return 0;
}

static int modem_4g_setup_data_call(hal_modem_4g_dev_attr_t *attr)
{
	int ret;
	char cmd_table[32] = {0};
	MODEM_CMD_T *modem_cmd = &cmds_tbl_modem[AT_CMD_DATA_CALL];

	if (modem_cmd->index != AT_CMD_DATA_CALL) {
		hal_log(HAL_LOG_ERR,"cmd not find, index:%d\n", AT_CMD_DATA_CALL);
		return -1;
	}
	if (modem_cmd->at_cmd == NULL)
		return NOT_SUPPORT;

	sprintf(cmd_table, "%s=1,1,1", modem_cmd->at_cmd);
	ret = modem_send_then_recv(attr->at_uart_fd, cmd_table, attr->at_recv_buf);
	if (ret != 0) {
		hal_log(HAL_LOG_ERR, "setup data call failed\n");
		return -1;
	}

	return 0;
}

static int modem_4g_close_data_call(hal_modem_4g_dev_attr_t *attr)
{
	int ret;
	char cmd_table[32] = {0};
	MODEM_CMD_T *modem_cmd = &cmds_tbl_modem[AT_CMD_DATA_CALL];

	if (modem_cmd->index != AT_CMD_DATA_CALL) {
		hal_log(HAL_LOG_ERR,"cmd not find, index:%d\n", AT_CMD_DATA_CALL);
		return -1;
	}
	if (modem_cmd->at_cmd == NULL)
		return NOT_SUPPORT;

	sprintf(cmd_table, "%s=0", modem_cmd->at_cmd);
	ret = modem_send_then_recv(attr->at_uart_fd, cmd_table, attr->at_recv_buf);
	if (ret != 0) {
		hal_log(HAL_LOG_ERR, "close data call failed\n");
		return -1;
	}

	return 0;
}

static int modem_4g_pdp_set(hal_modem_4g_dev_attr_t *attr, int active)
{
	int ret;
	char cmd_table[32] = {0};

	sprintf(cmd_table, "%s=%d,1", attr->at_cmd_table[AT_CMD_CGACT], active);
	ret = modem_send_then_recv(attr->at_uart_fd,cmd_table,attr->at_recv_buf);
	if (ret != 0) {
		hal_log(HAL_LOG_ERR, "pdp set failed\n");
		return -1;
	}

	return 0;
}

static int modem_4g_register_net_check(hal_modem_4g_dev_attr_t *attr)
{
	int ret;
	char *str;
	int mode = -1;
	int format = -1;
	int oper[64] = {0};
	int act = 0;
	int i = 0;
	char cmd_table[32] = {0};

	sprintf(cmd_table, "%s?", attr->at_cmd_table[AT_CMD_COPS]);
	while (i < 3) {
		ret = modem_send_then_recv(attr->at_uart_fd,cmd_table,attr->at_recv_buf);
		str = strstr(attr->at_recv_buf,"+COPS:");
		if ((ret == 0) && (str != NULL)) {
			sscanf(str,"+COPS: %d,%d,\"%[^\"]\",%d",&mode,&format,oper,&act);
			if((mode == 0) && (strlen(oper) > 0)){
				hal_log(HAL_LOG_INFO,"already registed to 4g network, oper:%s\n", oper);
				return 0;
			} else {
				hal_log(HAL_LOG_ERR,"not yet registed to 4g network\n");
				i++;
				sleep(3);
				continue;
			}
		} else {
			hal_log(HAL_LOG_ERR, "check register mode failed\n");
			return -1;
		}
	}

	return -1;
}

static int modem_4g_register_mode_set(hal_modem_4g_dev_attr_t *attr)
{
	int ret;
	char cmd_table[32] = {0};

	sprintf(cmd_table, "%s=0", attr->at_cmd_table[AT_CMD_COPS]);
	ret = modem_send_then_recv(attr->at_uart_fd,cmd_table,attr->at_recv_buf);
	if (ret != 0) {
		hal_log(HAL_LOG_ERR,"set modem_4g net auto register failed\n");
		return -1;
	}

	return 0;
}

/* set the functionality level, enable=0:Min enable=1:Full */
static int modem_4g_func_enable(hal_modem_4g_dev_attr_t *attr, int enable)
{
	int ret;
	char cmd_table[32] = {0};

	sprintf(cmd_table, "%s=%d", attr->at_cmd_table[AT_CMD_CFUN], enable);
	ret = modem_send_then_recv(attr->at_uart_fd,cmd_table,attr->at_recv_buf);
	if (ret != 0) {
		hal_log(HAL_LOG_ERR,"set func failed\n");
		return -1;
	}

	return 0;
}

/* Module reset */
int hal_modem_4g_reset_common(hal_modem_4g_dev_attr_t *attr)
{
	int ret;
	char cmd_table[32] = {0};

	sprintf(cmd_table, "%s=1,1", attr->at_cmd_table[AT_CMD_CFUN]);
	ret = modem_send_then_recv(attr->at_uart_fd,cmd_table,attr->at_recv_buf);
	if (ret != 0) {
		hal_log(HAL_LOG_ERR,"set func failed\n");
		return -1;
	}

	return 0;
}

/* Check Network Registration Status */
static int modem_4g_net_status_check(hal_modem_4g_dev_attr_t *attr, int *stat)
{
	int ret;
	char *str;
	int n;
	char cmd_table[32] = {0};

	sprintf(cmd_table, "%s", attr->at_cmd_table[AT_CMD_CREG]);
	ret = modem_send_then_recv(attr->at_uart_fd, cmd_table, attr->at_recv_buf);
	str = strstr(attr->at_recv_buf, "+CREG:");
	if ((ret == 0) && (str != NULL)) {
		sscanf(str, "+CREG:%d,%d", &n, stat);
		if (*stat == 1 || *stat == 5) {
			hal_log(HAL_LOG_INFO, "register to the network, stat=%d\n", *stat);
		} else {
			hal_log(HAL_LOG_ERR, "register to the network failed\n");
			return -1;
		}
	} else {
		hal_log(HAL_LOG_ERR, "check network status failed\n");
		return -1;
	}

	return 0;
}

/* Check EPS network registration status */
static int modem_4g_lte_data_check(hal_modem_4g_dev_attr_t *attr,int *stat)
{
	int ret;
	char *str;
	int type;
	char cmd_table[32] = {0};

	sprintf(cmd_table, "%s", attr->at_cmd_table[AT_CMD_CEREG]);
	ret = modem_send_then_recv(attr->at_uart_fd,cmd_table,attr->at_recv_buf);
	str = strstr(attr->at_recv_buf,"+CEREG:");
	if((ret == 0) && (str != NULL)){
		sscanf(str,"+CEREG:%d,%d",&type,stat);
		if(*stat == 1 || *stat == 5){
			hal_log(HAL_LOG_INFO,"lte data service available,stat=%d\n",*stat);
		}else{
			hal_log(HAL_LOG_ERR,"lte data service is not available\n");
			return -1;
		}
	}else{
		hal_log(HAL_LOG_ERR,"check lte domain failed\n");
		return -1;
	}

	return 0;
}

/* Select TE Character Set, chset = "GSM" / "IRA" / "UCS2" */
static int modem_4g_character_set(hal_modem_4g_dev_attr_t *attr, char *chset)
{
	int ret;
	char cmd_table[32] = {0};

	sprintf(cmd_table, "%s=\"%s\"", attr->at_cmd_table[AT_CMD_CSCS], chset);
	ret = modem_send_then_recv(attr->at_uart_fd,cmd_table,attr->at_recv_buf);
	if (ret != 0) {
		hal_log(HAL_LOG_ERR,"character set failed\n");
		return -1;
	}

	return 0;
}

/* Message Format set, mode=0:PDU mode / mode=1:Text mode */
static int modem_4g_message_format_set(hal_modem_4g_dev_attr_t *attr, int mode)
{
	int ret;
	char cmd_table[32] = {0};

	sprintf(cmd_table, "%s=%d", attr->at_cmd_table[AT_CMD_CMGF], mode);
	ret = modem_send_then_recv(attr->at_uart_fd,cmd_table,attr->at_recv_buf);
	if (ret != 0) {
		hal_log(HAL_LOG_ERR,"message format set failed\n");
		return -1;
	}

	return 0;
}

/* Send Message, phone_num:phone number, mesg_buf:message */
static int modem_4g_send_message(hal_modem_4g_dev_attr_t *attr, char *phone_num, char *mesg_buf)
{
	int ret;
	char cmd_table[32] = {0};
	char ctrl_z = 0x1a;

	sprintf(cmd_table, "%s=\"%s\"", attr->at_cmd_table[AT_CMD_CMGS], phone_num);
	ret = modem_send_then_recv(attr->at_uart_fd,cmd_table,attr->at_recv_buf);
	if (ret != 0) {
		hal_log(HAL_LOG_ERR,"phone number set failed\n");
		return -1;
	}

	/* The message ends with Ctrl+Z */
	sprintf(mesg_buf, "%s%c", mesg_buf, ctrl_z);
	ret = modem_send_then_recv(attr->at_uart_fd,mesg_buf,attr->at_recv_buf);
	if (ret != 0) {
		hal_log(HAL_LOG_ERR,"send message failed\n");
		return -1;
	}

	return 0;
}

int hal_modem_4g_send_message_common(hal_modem_4g_dev_attr_t *attr, char *phone_num, char *mesg_buf)
{
	int ret;

	ret = modem_4g_register_net_check(attr);
	if(ret != 0){
		hal_log(HAL_LOG_ERR,"modem_4g_register_net_check failed\n");
		return -1;
	}

	ret = modem_4g_character_set(attr, "GSM");
	if(ret != 0){
		hal_log(HAL_LOG_ERR, "modem_4g_character_set failed\n");
		return -2;
	}

	ret = modem_4g_message_format_set(attr, 1);
	if(ret != 0){
		hal_log(HAL_LOG_ERR, "modem_4g_message_format_set failed\n");
		return -3;
	}

	ret = modem_4g_send_message(attr, phone_num, mesg_buf);
	if(ret != 0){
		hal_log(HAL_LOG_ERR, "modem_4g_send_message failed\n");
		return -4;
	}

	return 0;
}

/* Sets the result code presentation status in the TA, mode=0:dis mode=1:en */
static int modem_4g_call_ret_mode(hal_modem_4g_dev_attr_t *attr, int mode)
{
	int ret;
	char cmd_table[32] = {0};

	sprintf(cmd_table, "%s=%d", attr->at_cmd_table[AT_CMD_COLP], mode);
	ret = modem_send_then_recv(attr->at_uart_fd,cmd_table,attr->at_recv_buf);
	if (ret != 0) {
		hal_log(HAL_LOG_ERR,"call result code mode set failed\n");
		return -1;
	}

	return 0;
}

/* Mobile Originated Call to Dial a Number */
static int modem_4g_voice_call(hal_modem_4g_dev_attr_t *attr, char *phone_num)
{
	int ret;
	char cmd_table[32] = {0};

	sprintf(cmd_table, "%s%s;", attr->at_cmd_table[AT_CMD_ATD], phone_num);
	ret = modem_send_then_recv(attr->at_uart_fd,cmd_table,attr->at_recv_buf);
	if (ret != 0) {
		hal_log(HAL_LOG_ERR,"voice call failed\n");
		return -1;
	}
	return 0;
}

int hal_modem_4g_voice_call_common(hal_modem_4g_dev_attr_t *attr, char *phone_num)
{
	int ret;

	ret = modem_4g_call_ret_mode(attr, 0);
	if(ret != 0){
		hal_log(HAL_LOG_ERR, "modem_4g_call_ret_mode failed\n");
		return -1;
	}

	ret = modem_4g_voice_call(attr, phone_num);
	if(ret != 0){
		hal_log(HAL_LOG_ERR, "modem_4g_voice_call failed\n");
		return -2;
	}

	return 0;
}

static int modem_4g_get_call_status(hal_modem_4g_dev_attr_t *attr, char *status)
{
	int ret;
	char cmd_table[32] = {0};
	MODEM_CMD_T *modem_cmd = &cmds_tbl_modem[AT_CMD_CALL_STATE];

	if (modem_cmd->index != AT_CMD_CALL_STATE) {
		hal_log(HAL_LOG_ERR,"cmd not find, index:%d\n", AT_CMD_CALL_STATE);
		return -1;
	}
	if (modem_cmd->at_cmd == NULL)
		return NOT_SUPPORT;

	sprintf(cmd_table, "%s", modem_cmd->at_cmd);
	ret = modem_send_then_recv(attr->at_uart_fd, cmd_table, attr->at_recv_buf);
	if (ret != 0) {
		hal_log(HAL_LOG_ERR, "get call status failed\n");
		return -1;
	}
	if (strstr(attr->at_recv_buf, modem_cmd->result_format)) {
		*status = 1;
	} else {
		*status = 0;
	}

	return 0;
}

int hal_modem_4g_get_call_status_common(hal_modem_4g_dev_attr_t *attr, char *status)
{
	int ret;

	if (status == NULL) {
		hal_log(HAL_LOG_ERR,"invalid param\n");
		return -1;
	}

	ret = modem_4g_get_call_status(attr, status);
	if(ret != 0)
		return ret;

	return 0;
}

/* Hang up Voice Call */
static int modem_4g_voice_hang_up(hal_modem_4g_dev_attr_t *attr)
{
	int ret;
	char cmd_table[32] = {0};

	sprintf(cmd_table, "%s", attr->at_cmd_table[AT_CMD_CHUP]);
	ret = modem_send_then_recv(attr->at_uart_fd,cmd_table,attr->at_recv_buf);
	if (ret != 0) {
		hal_log(HAL_LOG_ERR,"hang up voice call failed\n");
		return -1;
	}

	return 0;
}

int hal_modem_4g_voice_hang_up_common(hal_modem_4g_dev_attr_t *attr)
{
	int ret;

	ret = modem_4g_voice_hang_up(attr);
	if(ret != 0){
		hal_log(HAL_LOG_ERR, "modem_4g_voice_hang_up failed\n");
		return -1;
	}

	return 0;
}

static int hal_modem_4g_enable_dtmf_check(hal_modem_4g_dev_attr_t *attr, int enable)
{
	int ret;
	char cmd_table[32] = {0};
	MODEM_CMD_T *modem_cmd = &cmds_tbl_modem[AT_CMD_DTMF];

	if (modem_cmd->index != AT_CMD_DTMF) {
		hal_log(HAL_LOG_ERR,"cmd not find, index:%d\n", AT_CMD_DTMF);
		return -1;
	}
	if (modem_cmd->at_cmd == NULL)
		return NOT_SUPPORT;

	sprintf(cmd_table, "%s=%d", modem_cmd->at_cmd, enable);
	ret = modem_send_then_recv(attr->at_uart_fd, cmd_table, attr->at_recv_buf);
	if (ret != 0) {
		hal_log(HAL_LOG_ERR, "enable dtmf check failed\n");
		return -1;
	}

	return 0;
}

int hal_modem_4g_get_dtmf_value_common(hal_modem_4g_dev_attr_t *attr, int *value, int timeout)
{
	char *str;
	int ret = -1;
	int recv_size = -1;
	char recv_buf[128] = {0};

	if(value == NULL) {
		hal_log(HAL_LOG_ERR,"invalid param\n");
		return -1;
	}

	if (timeout < 0) {
		timeout = 0;
	}

	ret = hal_modem_4g_enable_dtmf_check(attr, 1);
	if (ret != 0)
		return ret;

	MODEM_CMD_T *modem_cmd = &cmds_tbl_modem[AT_CMD_DTMF];

	recv_size = modem_recv(attr->at_uart_fd, recv_buf, sizeof(recv_buf), timeout);
	str = strstr(recv_buf, modem_cmd->result_flag);
	if (recv_size > 0 && str != NULL) {
		sscanf(str, modem_cmd->result_format, value);
		hal_log(HAL_LOG_INFO,"recv value:%d\n",*value);
		return 0;
	}

	hal_log(HAL_LOG_ERR,"not get the dtmf value\n");
	return -1;
}

/* Get the subscribers own number(s) from the (U)SIM. */
int hal_modem_4g_get_phone_number_common(hal_modem_4g_dev_attr_t *attr, char *phone_num)
{
	int ret = 0;
	int type = 0;
	char *str = NULL;
	char cmd_table[32] = {0};

	sprintf(cmd_table, "%s", attr->at_cmd_table[AT_CMD_CNUM]);
	ret = modem_send_then_recv(attr->at_uart_fd,cmd_table,attr->at_recv_buf);
	str = strstr(attr->at_recv_buf, "+CNUM:");
	if ((ret == 0) && (str != NULL)) {
		//sscanf(str, "+CNUM: \"\",\"%s\",%d", phone_num, type);
		memcpy(phone_num, str, 64);
	} else {
		hal_log(HAL_LOG_ERR, "get phone number failed\n");
		return -1;
	}

	return 0;
}

/* Request MT Firmware Revision Identification */
int hal_modem_4g_get_firmware_version_common(hal_modem_4g_dev_attr_t *attr, char *version)
{
	int ret = 0;
	char cmd_table[32] = {0};

	sprintf(cmd_table, "%s", attr->at_cmd_table[AT_CMD_CGMR]);
	ret = modem_send_then_recv(attr->at_uart_fd,cmd_table,attr->at_recv_buf);
	if (ret != 0) {
		hal_log(HAL_LOG_ERR,"get firmware version failed\n");
		return -1;
	}
	sscanf(attr->at_recv_buf,"\n%s\n",version);

	return 0;
}

/* Enable SIM card hot plug */
static int modem_4g_sim_card_hot_plug(hal_modem_4g_dev_attr_t *attr)
{
	int ret;
	char cmd_table[32] = {0};
	MODEM_CMD_T *modem_cmd = &cmds_tbl_modem[AT_CMD_SIM_HOT];

	if (modem_cmd->index != AT_CMD_SIM_HOT) {
		hal_log(HAL_LOG_ERR,"cmd not find, index:%d\n", AT_CMD_SIM_HOT);
		return -1;
	}
	if (modem_cmd->at_cmd == NULL)
		return NOT_SUPPORT;

	sprintf(cmd_table, "%s", modem_cmd->at_cmd);
	ret = modem_send_then_recv(attr->at_uart_fd, cmd_table, attr->at_recv_buf);
	if (ret != 0) {
		hal_log(HAL_LOG_ERR, "sim card hot plug set failed\n");
		return -1;
	}

	return 0;
}

/* get PDP Address */
static int modem_4g_pdp_ip_query(hal_modem_4g_dev_attr_t *attr,char *ip)
{
	int ret;
	char *str;
	int cid;
	char cmd_table[32] = {0};

	sprintf(cmd_table, "%s", attr->at_cmd_table[AT_CMD_CGPADDR]);
	ret = modem_send_then_recv(attr->at_uart_fd,cmd_table,attr->at_recv_buf);
	str = strstr(attr->at_recv_buf,"+CGPADDR:");
	if((ret == 0) && (str != NULL)){
		sscanf(str,"+CGPADDR: %d,\"%[^\"]\"",&cid,ip);
		hal_log(HAL_LOG_INFO,"pdp ip query success,ip = %s\n",ip);
	}else{
		hal_log(HAL_LOG_ERR,"pdp ip query failed\n");
		return -1;
	}

	return 0;
}

static int modem_4g_at_uart_init(hal_modem_4g_dev_attr_t *attr)
{
	int ret;

	ret = modem_4g_get_dev_attr(attr);
	if (ret != 0) {
		hal_log(HAL_LOG_ERR, "modem_4g_get_dev_attr failed\n");
		return -1;
	}
	
	ret = modem_4g_set_dev_attr(attr);
	if (ret != 0) {
		hal_log(HAL_LOG_ERR, "modem_4g_set_dev_attr failed\n");
		return -1;
	}

	return 0;
}

static int modem_4g_dhcp_get_ip(void)
{
	int ret = 0;
	FILE *pp = NULL;
	char cmd[128] = {0};
	char *str;
	char ip[32] = {0x00};
	char strtmp[1024] = {0x00};

	snprintf(cmd,sizeof(cmd),MODEM_4G_CMD_DHCP);
	ret = system(cmd);
	if (ret != 0) {
		hal_log(HAL_LOG_ERR,"modem_4g_cmd_dhcp failed\n");
		return -1;
	}
		
	memset(cmd,0,sizeof(cmd));
	snprintf(cmd,sizeof(cmd),"ifconfig usb0");
	pp = popen(cmd,"r");
	if(pp == NULL)
		return -1;

	while(fgets(strtmp,sizeof(strtmp),pp) != NULL){
		str = strstr(strtmp,"inet addr:");
		if(str){
			sscanf(str+strlen("inet addr:"),"%[^ ]",ip);
			if(strlen(ip) != 0){
				printf(" modem_4g_dhcp_get_ip success,ip = %s\n",ip);
				break;
			}else{
				printf("modem_4g_dhcp_get_ip failed\n");
				return -1;
			}
		}else{
			continue;
		}
	}
	pclose(pp);

	return 0;

}

static int modem_4g_ping_test(void)
{
	return 0;
}

int hal_modem_4g_init_common(hal_modem_4g_dev_attr_t *attr)
{
	int ret;

	ret = modem_4g_at_uart_init(attr);
	if(ret != 0){
		hal_log(HAL_LOG_ERR, "modem_4g_at_uart_init failed\n");
		return AT_UART_INIT_FAILED;
	}
	
	ret = modem_4g_at_cmd_test(attr);
	if(ret != 0){
		hal_log(HAL_LOG_ERR, "modem_4g_at_cmd_test failed\n");
		return AT_TEST_FAILED;
	}

	ret = modem_4g_at_cmd_echo_set(attr, 0);
	if(ret != 0){
		hal_log(HAL_LOG_ERR, "modem_4g_at_cmd_echo_set failed\n");
		return AT_SET_ECHO_OFF_FAILED;
	}
	
	ret = modem_4g_at_cmd_cmee_set(attr, 2);
	if(ret != 0){
		hal_log(HAL_LOG_ERR, "modem_4g_at_cmd_cmee_set failed\n");
		return AT_SET_CMEE_FAILED;
	}

	ret = modem_4g_sim_card_hot_plug(attr);
	if(ret < 0){
		hal_log(HAL_LOG_ERR, "modem_4g_sim_card_hot_plug failed\n");
		return AT_SET_HOT_PLUG_FAILED;
	}

	hal_log(HAL_LOG_INFO,"%s module init ok\n",attr->module_name);

	return AT_PORT_INIT_SUCCESS;
}

int hal_modem_4g_set_ecm_modem_common(hal_modem_4g_dev_attr_t *attr)
{
	int ret, mode;

	ret = modem_4g_usb_mode_check(attr, &mode);
	if (ret != 0)
		return ret;

	if (mode == USB_ECM_MODE) {
		hal_log(HAL_LOG_INFO, "already in ecm mode, no need to set\n");
		return AT_USB_IN_ECM_MODE;
	}

	hal_log(HAL_LOG_INFO, "not in ecm mode, need to set\n");
	ret = modem_4g_usb_ecm_mode_set(attr);
	if (ret != 0)
		return ret;

	hal_log(HAL_LOG_INFO,"set usb to ecm modem success, need to reset the 4g module\n");
	system("echo 1 > /sys/class/gpio/gpio40/value");
	usleep(50000);
	system("echo 0 > /sys/class/gpio/gpio40/value");
	hal_log(HAL_LOG_INFO,"modem_4g_reset success, need to wait 30s\n");
	return AT_USB_RESET_SUCCESS;
}

/* Request Product Serial Number Identification Description */
int hal_modem_4g_imei_check_common(hal_modem_4g_dev_attr_t *attr,char *imei)
{
	int ret = 0;
	char cmd_table[32] = {0};

	sprintf(cmd_table, "%s", attr->at_cmd_table[AT_CMD_CGSN]);
	ret = modem_send_then_recv(attr->at_uart_fd,cmd_table,attr->at_recv_buf);
	if (ret != 0) {
		hal_log(HAL_LOG_ERR,"get imei failed\n");
		return -1;
	}
	sscanf(attr->at_recv_buf,"\n%s\n",imei);

	return 0;
}

/* Request Integrate Circuit Card Identity */
int hal_modem_4g_iccid_check_common(hal_modem_4g_dev_attr_t *attr,char *iccid)
{
	int ret = 0;
	char cmd_table[32] = {0};

	sprintf(cmd_table, "%s", attr->at_cmd_table[AT_CMD_CCID]);
	ret = modem_send_then_recv(attr->at_uart_fd,cmd_table,attr->at_recv_buf);
	if (ret != 0) {
		hal_log(HAL_LOG_ERR,"get iccid failed\n");
		return -1;
	}
	sscanf(attr->at_recv_buf,"\n+CCID: %s\n", iccid);

	return 0;
}

int hal_modem_4g_sim_card_detect_common(hal_modem_4g_dev_attr_t *attr)
{
	int ret;
	char *str;
	char cmd_table[32] = {0};

	sprintf(cmd_table, "%s?", attr->at_cmd_table[AT_CMD_CPIN]);
	ret = modem_send_then_recv(attr->at_uart_fd,cmd_table,attr->at_recv_buf);
	str = strstr(attr->at_recv_buf,"+CPIN: READY");
	if(ret || str == NULL){
		hal_log(HAL_LOG_ERR,"detect card failed\n");
		return -1;
	}

	return 0;
}

/* Request International Mobile Subscriber Identity (IMSI) */
int hal_modem_4g_imsi_check_common(hal_modem_4g_dev_attr_t *attr,char *imsi)
{
	int ret = 0;
	char cmd_table[32] = {0};

	sprintf(cmd_table, "%s", attr->at_cmd_table[AT_CMD_CIMI]);
	ret = modem_send_then_recv(attr->at_uart_fd,cmd_table,attr->at_recv_buf);
	if (ret != 0) {
		hal_log(HAL_LOG_ERR,"get imsi failed\n");
		return -1;
	}
	sscanf(attr->at_recv_buf,"\n%s\n",imsi);

	return 0;
}

int hal_modem_4g_signal_quality_check_common(hal_modem_4g_dev_attr_t *attr,int *rssi,int *ber)
{
	int ret;
	char *str;
	char cmd_table[32] = {0};

	sprintf(cmd_table, "%s", attr->at_cmd_table[AT_CMD_CSQ]);
	ret = modem_send_then_recv(attr->at_uart_fd,cmd_table,attr->at_recv_buf);
	str = strstr(attr->at_recv_buf,"+CSQ:");

	if((ret == 0) && (str != NULL)){
		sscanf(str,"+CSQ:%d,%d",rssi,ber);
		if((*rssi > 0) && (*rssi <= 99)){
			hal_log(HAL_LOG_INFO,"get signal quality +CSQ : %d %d\n",*rssi,*ber);
		}else{
			hal_log(HAL_LOG_ERR,"invalid rssi %d\n",*rssi);
			return -1;
		}
	}else{
		hal_log(HAL_LOG_ERR,"get signal quality failed\n");
		return -1;
	}

	return 0;
}

int hal_modem_4g_apn_set_common(hal_modem_4g_dev_attr_t *attr, char *apn)
{
	int ret;
	char cmd_table[32] = {0};

	if (apn) {
		sprintf(cmd_table, "%s=1,\"IP\",\"%s\"", attr->at_cmd_table[AT_CMD_CGDCONT], apn);
	} else {
		sprintf(cmd_table, "%s=1,\"IP\"", attr->at_cmd_table[AT_CMD_CGDCONT]);
	}

	ret = modem_send_then_recv(attr->at_uart_fd,cmd_table,attr->at_recv_buf);
	if (ret != 0) {
		hal_log(HAL_LOG_ERR,"set apn failed\n");
		return -1;
	}

	return 0;
}

int hal_modem_4g_set_user_common(hal_modem_4g_dev_attr_t *attr, char *apn, char *username, char *password)
{
	int ret;
	char cmd_table[128] = {0};
	MODEM_CMD_T *modem_cmd = &cmds_tbl_modem[AT_CMD_USER];

	if (modem_cmd->index != AT_CMD_USER) {
		hal_log(HAL_LOG_ERR,"cmd not find, index:%d\n", AT_CMD_USER);
		return -1;
	}
	if (modem_cmd->at_cmd == NULL)
		return NOT_SUPPORT;

	if (((int)modem_cmd->reserved) == 2) {
		sprintf(cmd_table, "%s=1,1,\"%s\",\"%s\"",modem_cmd->at_cmd, username, password);
	} else if (((int)modem_cmd->reserved) == 3) {
		sprintf(cmd_table, "%s=1,1,\"%s\",\"%s\",\"%s\",1",modem_cmd->at_cmd, apn, username, password);
	}

	ret = modem_send_then_recv(attr->at_uart_fd, cmd_table, attr->at_recv_buf);
	if (ret != 0) {
		hal_log(HAL_LOG_ERR,"set user failed\n");
		return -1;
	}

	return 0;
}

/* In order to be compatible with bg95, the registered operator is slow */
static int modem_4g_func_reset(hal_modem_4g_dev_attr_t *attr, int delay_ms)
{
	int ret;

	if (delay_ms <= 0)
		delay_ms = 3000;

	ret = modem_4g_func_enable(attr, 0);
	sleep(1);
	ret |= modem_4g_func_enable(attr, 1);
	if(ret != 0){
		hal_log(HAL_LOG_ERR,"modem_4g_func_enable failed\n");
		return -1;
	}
	usleep(delay_ms*1000);

	return 0;
}

int hal_modem_4g_connect_net_common(hal_modem_4g_dev_attr_t *attr, int delay_ms)
{
	int ret;
	int stat;

	ret = modem_4g_func_reset(attr, delay_ms);
	if(ret != 0){
		hal_log(HAL_LOG_ERR,"modem_4g_func_reset failed\n");
		return AT_FUNC_ENABLE_FAILED;
	}

	ret = modem_4g_register_mode_set(attr);
	if(ret != 0){
		hal_log(HAL_LOG_ERR,"modem_4g_register_mode_set failed\n");
		return AT_REG_MODE_SET_FAILED;
	}

	ret = modem_4g_register_net_check(attr);
	if(ret != 0){
		hal_log(HAL_LOG_ERR,"modem_4g_register_net_check failed\n");
		return AT_REGISTER_NET_FAILED;
	}

	ret = modem_4g_net_status_check(attr,&stat);
	ret &= modem_4g_lte_data_check(attr,&stat);
	if(ret != 0){
		hal_log(HAL_LOG_ERR,"modem_4g_lte_data_check failed\n");
		return AT_LTE_DATA_CHECK_FAILED;
	}

	ret = modem_4g_rndis_set(attr, 1);
	if(ret < 0){
		hal_log(HAL_LOG_ERR,"modem_4g_rndis_set failed\n");
		return AT_SET_RNDIS_FAILED;
	}

	ret = modem_4g_setup_data_call(attr);
	if(ret < 0){
		hal_log(HAL_LOG_ERR,"modem_4g_setup_data_call failed\n");
		modem_4g_close_data_call(attr);
		return AT_SET_RNDIS_FAILED;
	}

	if (modem_name_is(attr, MODULE_NAME_LZ201)) {
		ret = modem_4g_pdp_set(attr, 1);
		if(ret != 0){
			hal_log(HAL_LOG_ERR,"modem_4g_pdp_set failed\n");
			return AT_SET_RNDIS_FAILED;
		}
	}

	return AT_REGISTER_NET_SUCCESS;
}

int hal_modem_4g_disconnect_net_common(hal_modem_4g_dev_attr_t *attr)
{
	int ret;

	/* kill dhcp client related processes */
	system("kill -9 $(ps | grep usb0 | grep -v grep | awk '{print $1}')");

	ret = modem_4g_func_enable(attr, 0);
	if (ret != 0) {
		hal_log(HAL_LOG_ERR,"set func disable failed\n");
		return -1;
	}

	return 0;

}

int hal_modem_4g_dail_up_common(hal_modem_4g_dev_attr_t *attr)
{
	int ret = 0;
	char ip[32];
	
	ret = modem_4g_pdp_ip_query(attr,ip);
	if(ret != 0){
		hal_log(HAL_LOG_ERR,"modem_4g_pdp_ip_query failed\n");
		return AT_PDP_IP_QUERY_FAILED;
	}
	
	ret = modem_4g_dhcp_get_ip();
	if(ret != 0){
		hal_log(HAL_LOG_ERR,"modem_4g_dhcp_get_ip failed\n");
		return AT_DHCP_GET_IP_FAILED;
	}
	
	usleep(2000000);
	ret = modem_4g_ping_test();
	if(ret != 0){
		hal_log(HAL_LOG_ERR,"modem_4g_ping_test failed\n");
		return AT_PING_TEST_FAILED;
	}

	return AT_DAIL_UP_SUCCESS;
}
