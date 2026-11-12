#!/bin/sh

echo "Tuyaos Openssl gen Public/Private keypair"

#secp256k1
openssl ecparam -genkey -name secp256k1 -out tuyaos_ecprikey.pem -param_enc explicit

openssl ec -in tuyaos_ecprikey.pem -pubout -out tuyaos_ecpubkey.pem

openssl ec -in tuyaos_ecprikey.pem -text


