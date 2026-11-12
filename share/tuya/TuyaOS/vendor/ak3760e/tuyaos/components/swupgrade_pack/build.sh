#!/bin/sh

gcc -I. -I bsp_sign -I bsp_verify bsp_sign/tuya_sign.c bsp_sign/cJSON.c bsp_verify/ecdsa.c bsp_verify/sha256.c bsp_verify/tuya_verify.c tuya_pack.c main.c -lcrypto -lcurl -o tuya_pack
