#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include "hal_modem_4g.h"

hal_modem_4g_dev_attr_t modem_4g_test;
const char *optstr = "s:f:a:u:p:n:m:";

void showusage()
{
	printf("\t -s 显示信息, 0--imei 1--imsi 2--信号质量 3--sim卡状态 4--iccid\n");
	printf("\t             5--手机号 6--固件版本\n");
	printf("\t -f 功能测试, 0--断网 1--连网 2--设置apn 3--设置user 4--发短信\n");
	printf("\t             5--挂断 6--拨号 7--获取dtmf值 8--复位 9--获取通话状态\n");
	printf("\t -a apn\n");
	printf("\t -u user name\n");
	printf("\t -p password\n");
	printf("\t -n 手机号\n");
	printf("\t -m 短信内容\n");

	return;
}

int main(int argc,char **argv)
{
	int ret;
	int ch;
	int func = -1;
	int show = -1;
	int rssi = 0, ber = 0;
	char imei[32] = {0};
	char imsi[32] = {0};
	char apn[16] = {0};
	char username[16] = {0};
	char password[16] = {0};
	char phone_num[16] = {0};
	char mesg_buf[256] = {0};

	while ((ch = getopt(argc, argv, optstr)) != -1) {
		switch (ch) {
			case 's':
				show = atoi(optarg);
				break;
			case 'f':
				func = atoi(optarg);
				break;
			case 'a':
				strncpy(apn, optarg, sizeof(apn)-1);
				break;
			case 'u':
				strncpy(username, optarg, sizeof(username)-1);
				break;
			case 'p':
				strncpy(password, optarg, sizeof(password)-1);
				break;
			case 'n':
				strncpy(phone_num, optarg, sizeof(phone_num)-1);
				break;
			case 'm':
				strncpy(mesg_buf, optarg, sizeof(mesg_buf)-1);
				break;
			default:
				printf("Unknown option: %c\n",(char)optopt);
				showusage();
            	return -1;
		}
	}

	ret = hal_modem_4g_init(&modem_4g_test);
	if (ret) {
		printf("hal_modem_4g_init failed, ret is %d\n", ret);
		return -1;
	} else {
		printf("hal_modem_4g_init success\n");
		printf("net_dev = %s\n",modem_4g_test.net_dev);
		printf("module_name = %s\n",modem_4g_test.module_name);
		printf("at_port = %s\n",modem_4g_test.at_port);
		printf("baud = %d\n\n",modem_4g_test.at_port_attr.baud);
	}

	if (show == 0) {
		ret = hal_modem_4g_imei_check(&modem_4g_test, imei);
		if (ret) {
			printf("get modem_4g imei failed\n");
			goto err;
		}
		printf("get modem_4g imei success,imei = %s\n", imei);
	} else if (show == 1) {
		ret = hal_modem_4g_imsi_check(&modem_4g_test, imsi);
		if (ret) {
			printf("get modem_4g imsi failed\n");
			goto err;
		}
		printf("get modem_4g imsi success, imsi = %s\n", imsi);
	} else if (show == 2) {
		ret = hal_modem_4g_signal_quality_check(&modem_4g_test, &rssi, &ber);
		if (ret) {
			printf("get signal_quality failed\n");
			goto err;
		}
		printf("get signal_quality success, rssi = %d, ber = %d\n", rssi, ber);
	} else if (show == 3) {
		ret = hal_modem_4g_sim_card_detect(&modem_4g_test);
		if (ret) {
			printf("sim card detect failed, ret is %d\n", ret);
			goto err;
		}
		printf("sim card detect success\n");
	} else if (show == 4) {
		char iccid[32] = {0};
		ret = hal_modem_4g_iccid_check(&modem_4g_test, iccid);
		if (ret) {
			printf("sim card detect failed, ret is %d\n", ret);
			goto err;
		}
		printf("get modem_4g iccid success, iccid = %s\n", iccid);
	} else if (show == 5) {
		char phone_num[32] = {0};
		ret = hal_modem_4g_get_phone_number(&modem_4g_test, phone_num);
		if (ret) {
			printf("get phone_num failed, ret is %d\n", ret);
			goto err;
		}
		printf("get phone_num success, phone_num = %s\n", phone_num);
	} else if (show == 6) {
		char version[32] = {0};
		ret = hal_modem_4g_get_firmware_version(&modem_4g_test, version);
		if (ret) {
			printf("get firmware version failed, ret is %d\n", ret);
			goto err;
		}
		printf("get firmware version success, version = %s\n", version);
	}

	if (func == 0) {
		ret = hal_modem_4g_disconnect_net(&modem_4g_test);
		if (ret) {
			printf("disconnect net failed, ret is %d\n", ret);
			goto err;
		}
		printf("disconnect net success\n");
	} else if (func == 1) {
		ret = hal_modem_4g_set_ecm_modem(&modem_4g_test);
		if (ret < 0) {
			printf("hal_modem_4g_set_ecm_modem failed, ret is %d\n", ret);
			goto err;
		}
		printf("hal_modem_4g_set_ecm_modem success\n");

		ret = hal_modem_4g_connect_net(&modem_4g_test, 10000);
		if (ret) {
			printf("connect net failed, ret is %d\n", ret);
			goto err;
		}
		printf("connect net success\n");

		ret = hal_modem_4g_dail_up(&modem_4g_test);
		if (ret) {
			printf("at dail up failed, ret is %d\n", ret);
			goto err;
		}
		printf("at dail up success\n");
	} else if (func == 2) {
		ret = hal_modem_4g_apn_set(&modem_4g_test, apn);
		if (ret) {
			printf("set apn failed, ret is %d\n", ret);
			goto err;
		}
		printf("set apn success\n");
	} else if (func == 3) {
		ret = hal_modem_4g_user_set(&modem_4g_test, apn, username, password);
		if (ret) {
			printf("set user failed, ret is %d\n", ret);
			goto err;
		}
		printf("set user success\n");
	} else if (func == 4) {
		ret = hal_modem_4g_send_message(&modem_4g_test, phone_num, mesg_buf);
		if (ret) {
			printf("send message failed, ret is %d\n", ret);
			goto err;
		}
		printf("send message success\n");
	} else if (func == 5) {
		ret = hal_modem_4g_voice_hang_up(&modem_4g_test);
		if (ret) {
			printf("voice hang up failed, ret is %d\n", ret);
			goto err;
		}
		printf("voice hang up success\n");
	} else if (func == 6) {
		ret = hal_modem_4g_voice_call(&modem_4g_test, phone_num);
		if (ret) {
			printf("voice call failed, ret is %d\n", ret);
			goto err;
		}
		printf("voice call success\n");
	} else if (func == 7) {
		int value = -1;
		int timeout = 30;
		ret = hal_modem_4g_get_dtmf_value(&modem_4g_test, &value, timeout);
		if (ret) {
			printf("get dtmf value failed, ret is %d\n", ret);
			goto err;
		}
		printf("get dtmf value success,value:%c\n", value);
	} else if (func == 8) {
		ret = hal_modem_4g_reset(&modem_4g_test);
		if (ret) {
			printf("module reset failed, ret is %d\n", ret);
			goto err;
		}
		printf("module reset success\n");
	} else if (func == 9) {
		char status = 0;
		ret = hal_modem_4g_get_call_status(&modem_4g_test, &status);
		if (ret) {
			printf("get call status failed, ret is %d\n", ret);
			goto err;
		}
		printf("get call status success, status: %d\n", status);
	}

	close(modem_4g_test.at_uart_fd);
	return 0;

err:
	close(modem_4g_test.at_uart_fd);
	return -1;
}
