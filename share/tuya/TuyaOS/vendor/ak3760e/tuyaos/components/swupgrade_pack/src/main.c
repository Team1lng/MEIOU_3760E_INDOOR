/*
 * (C) Copyright 2020
 * Cheng Lei, TUYA Software Engineering, chenglei@tuya.com.
 * Tuya pack code
 */

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tuya_pack.h"

#define MAX_IMAGE_FNAME         512
#define PACK_VER                "1.0.3"

static void usage(char *programname)
{
	printf("%s (compiled %s)\n", programname, __DATE__);
	printf("Usage %s [OPTION]\n", programname);
	printf(
			" -c, --config       : json file which include zone table\n"
			" -d, --dump         : dump ota firmware information\n"
			" -e, --extract      : extract zones from ota firmware\n"
			" -h, --help         : print this help and exit\n"
			" -v, --version      : version of tuya pack\n"
			" -k, --key          : private key of secp256k1\n"
			" -s, --sign         : file to sign\n"
			" -a, --addsign      : pack to sign\n"
			" -l, --local        : local signature(must have -a)\n"
	       );
}

static struct option long_options[] = {
	{"help", no_argument, NULL, 'h'},
	{"config", required_argument, NULL, 'c'},
	{"dump", required_argument, NULL, 'd'},
	{"extract", required_argument, NULL, 'e'},
	{"version", no_argument, NULL, 'v'},
	{"key", required_argument, NULL, 'k'},
	{"sign", required_argument, NULL, 's'},
	{"addsign", no_argument, NULL, 'a'},
	{"local", no_argument, NULL, 'l'},
	{NULL, 0, NULL, 0}
};

int tuya_upgrade_version(char ver[32])
{
	if (!ver)
		return -1;

	strncpy(ver, PACK_VER, 32);

	return 0;
}

extern int add_sign;
int main(int argc, char **argv)
{
	int opt_c = 0;
        int opt_d = 0;
        int opt_e = 0;
        int opt_k = 0;
        int opt_s = 0;
	int opt_a = 0;
	int opt_l = 0;
	int c;
	int ret = -1;
	char cfg_fname[MAX_IMAGE_FNAME] = {0};
	char fw_fname[MAX_IMAGE_FNAME] = {0};
	char key_fname[MAX_IMAGE_FNAME] = {0};
	char sign_fname[MAX_IMAGE_FNAME] = {0};
	char version[32] = {0};

	while ((c = getopt_long(argc, argv, "lavhe:d:c:k:s:",
				long_options, NULL)) != EOF) {
		switch (c) {
			case 'c':
				strncpy(cfg_fname, optarg, sizeof(cfg_fname));
				opt_c = 1;
				break;
			case 'd':
				strncpy(fw_fname, optarg, sizeof(fw_fname));
				opt_d = 1;
				break;
			case 'e':
				strncpy(fw_fname, optarg, sizeof(fw_fname));
				opt_e = 1;
				break;
			case 'k':
				strncpy(key_fname, optarg, sizeof(key_fname));
				opt_k = 1;
				break;
			case 's':
				strncpy(sign_fname, optarg, sizeof(sign_fname));
				opt_s = 1;
				break;
			case 'a':
				opt_a = 1;
				break;
			case 'l':
				opt_l = 1;
				break;
			case 'h':
				usage(argv[0]);
				exit(0);
				break;
			case 'v':
				tuya_upgrade_version(version);
				printf("%s\n", version);
				exit(0);
				break;
			default:
				usage(argv[0]);
				exit(1);
				break;
		}
	}

	if (opt_c == 0 && opt_d == 0 && opt_e == 0 && opt_s == 0) {
		usage(argv[0]);
		exit(1);
	}

	if (opt_k) {
		ret = tuya_upgrade_load_key(key_fname);
		if (ret)
			return ret;
	}

	if (opt_s) {
		char result_fname[MAX_IMAGE_FNAME] = {0};
		snprintf(result_fname, MAX_IMAGE_FNAME, "%s%s", sign_fname, ".sig");
		ret = tuya_upgrade_sign_file(sign_fname, result_fname);
	}

#ifndef TUYA_UNSIGN_PACK
	add_sign = CLOUD_SIGNED;
#else
	add_sign = NOT_SIGNED;
#endif

	if (opt_a) {
		add_sign = CLOUD_SIGNED;
	}

	if (opt_l && opt_a) {
		add_sign = LOCAL_SIGNED;
	}

	if (opt_c) {
		ret = tuya_upgrade_pack(cfg_fname);
	}

	if (opt_d) {
		ret = tuya_upgrade_dump(fw_fname);
	}

	if (opt_e) {
		ret = tuya_upgrade_extract(fw_fname);
	}

	return ret;
}
