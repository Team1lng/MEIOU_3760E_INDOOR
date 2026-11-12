# BSP 安全启动开发指南
--------------------------------
## Tuya BSP 配置
menuconfig中打开如下配置:  
CONFIG_TUYA_TARGET_SECURE_BOOT=y

生成公私钥对，建议使用 `RSA 4096`
```shell
cd $BOARD_DIR
mkdir secure_keys
openssl genrsa -F4 -out secure_keys/tuya.key 4096
openssl req -batch -new -x509 -key secure_keys/tuya.key -out secure_keys/tuya.crt
```
然后在 `CONFIG_TUYA_SECURE_KEY` 中指定secure_keys的路径。

## U-Boot 配置
U-Boot中需要开启FITImage的支持，打开如下配置：
- CONFIG_FIT - enable FIT image support
- CONFIG_FIT_SIGNATURE - enable signing and verfication in FITs
- CONFIG_RSA - enable RSA algorithm for signing

## Linux Kernel 配置
内核中需要开启dm-verity的支持:
- CONFIG_MD=y
- CONFIG_BLK_DEV_DM=y
- CONFIG_DM_INIT=y
- CONFIG_DM_VERITY=y

对于v5.1以下的kernel版本，需要打上DM_INIT的补丁，以获得直接将DM块设备挂载为root的支持。
补丁共三个，请从下面的链接中下载：  
- [0001-dm-add-support-to-directly-boot-to-a-mapped-device.patch](https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/patch/?id=6bbc923dfcf57d6b97388819a7393835664c7a8e)
- [0002-dm-init-fix-max-devices-targets-checks.patch](https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/patch/?id=8e890c1ab1b1e0f765cd8da82c4dee011698a5e8)
- [0003-dm-ioctl-fix-hang-in-early-create-error-condition.patch](https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/patch/?id=0f41fcf78849c902ddca564f99a8e23ccfc80333)