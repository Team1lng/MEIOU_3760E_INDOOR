#!/bin/bash


VENDOR_IMAGE_PACK_SCRIPT="tools/build_vendor_image.sh"
SUPPORT_DRIVERS=

msg() {
	RED='\033[0;31m'
	NC='\033[0m' # No Color
	printf "\n${RED}\n%s\n%s\n%s${NC}\n\n" \
		"##################################" \
		"## $@ " \
		"##################################"
}

usage() {
	echo "###############################################"
	echo "## Support Options: "
	echo "##     -h, --help       print this message"
	echo "##     -d, --drivers    support driver list"
	echo "##     -a, --add cloud sign   support add cloud sign"
	echo "##     -al, --add local sign   support add local sign"
	echo "###############################################"
}

parse_yaml() {
   local prefix=$2
   local s='[[:space:]]*' w='[a-zA-Z0-9_]*' fs=$(echo @|tr @ '\034')
   sed -ne "s|^\($s\):|\1|" \
        -e "s|^\($s\)\($w\)$s:$s[\"']\(.*\)[\"']$s\$|\1$fs\2$fs\3|p" \
        -e "s|^\($s\)\($w\)$s:$s\(.*\)$s\$|\1$fs\2$fs\3|p"  $1 |
   awk -F$fs '{
      indent = length($1)/2;
      vname[indent] = $2;
      for (i in vname) {if (i > indent) {delete vname[i]}}
      if (length($3) > 0) {
         vn=""; for (i=0; i<indent; i++) {vn=(vn)(vname[i])("_")}
         printf("%s%s%s=\"%s\"\n", "'$prefix'",vn, $2, $3);
      }
   }'
}

# $1 source driver directory
# $2 target driver directory
# $3 selected drivers
prepare_drviers() {
	local DRV_LIST=$3
	if [ -z "${DRV_LIST}" ];then
		DRV_TAGS=$(ls $1 | awk -F ' ' '{print $1 " YES"}')
		if [ ! -z "$DRV_TAGS" ];then
			DRV_LIST=$(whiptail --noitem \
				--title "Support Drivers" \
				--checklist "Choose Kernel Drivers" \
				18 80 10 ${DRV_TAGS} 3>&1 1>&2 2>&3 | \
				sed s/\"//g)
		fi
	fi
	mkdir -p $2
	for drv in ${DRV_LIST};do
		cp -v $1/$drv $2
	done
}

while [[ $# -gt 0 ]]; do
	case $1 in
		-h|--help)
		usage
		exit 0
		;;
		-a)
                sign_param=$1
		shift
		;;
		-al)
                sign_param=$1
		shift
		;;
		-c|--config)
		CONF_FILE="$2"
		shift
		shift
		;;
	esac
done

if [ ! -z $CONF_FILE -a -f $CONF_FILE ];then
	msg "Parse config file"
	echo $CONF_FILE
	eval $(parse_yaml $CONF_FILE)
fi

declare -A drivers
for drv in ${!drivers_@};do
	name=`echo $drv | cut -d '_' -f 2`
	var=`echo $drv | cut -d '_' -f 3`
	drivers[$name]+="$var=${!drv} "
done
for x in "${!drivers[@]}"; do
	eval ${drivers[$x]}
	SUPPORT_DRIVERS+="${name} "
	if [ ! -z ${param} ];then
		echo "ro.driver.${name%.ko}=${param}" >> images/build.prop
	fi
	unset name param
done

rm -rf drv_tmp rootfs_tmp
if [ -d drv ];then
	cp -a drv drv_tmp
else
	mkdir drv_tmp
fi

if [ -f rootfs.tar.gz ];then
	rm -rf rootfs
        tar xf rootfs.tar.gz
fi

if [ -e drivers/nvram_kernel_driver.ko ];then
mkdir -p rootfs/lib/modules
cp -rf drivers/nvram_kernel_driver.ko rootfs/lib/modules/
fi


if [ -d rootfs ];then
	cp -a rootfs rootfs_tmp
else
	mkdir rootfs_tmp
fi



msg "Select needed kernel drivers"
prepare_drviers drivers drv_tmp "${SUPPORT_DRIVERS}"

cp images/build.prop images/default.prop
chmod 0644 images/default.prop

update_pubkey() {
	line=$(grep -n "persist.ota.pubkey" images/default.prop |cut -d ":" -f 1)
#	if [[ "$line" == "" ]]; then
		echo "$1" >> images/default.prop
#	else
#		old_public_key=$(sed -n ${line}p images/default.prop)
#		if [[ "$1" != "$old_public_key" ]]; then
#			echo "update public key"
#			sed -i '/persist.ota.pubkey=/d' images/default.prop
#			echo "$1" >> images/default.prop
#		fi
#	fi
}

if [[ "$sign_param" == "-al" ]]; then
	# local signature
	echo "local signature"
	./tuya_make_pubkey_bin.sh
	pubkey=$(cat tuya_verify_pubkey.bin)
	public_key="persist.ota.pubkey=$pubkey"
	update_pubkey "$public_key"
elif [[ "$sign_param" == "-a" ]]; then
	# cloud signature
	echo "cloud signature"
	public_key="persist.ota.pubkey=da185725ede53e6446f699dc0ac95b4c6b2fbf8858872dd4fd8bd520ee2c2e728df235ac06a5ae5b06af03262370972e6a73a9ff3a20c3c09b33ce60d9d02662"
	update_pubkey "$public_key"
fi


msg "Update partitions.json"
if [ ! -f version.ini ];then
	declare -A partitions
	for part in ${!partitions_@};do
		name=`echo $part | cut -d '_' -f 2`
		val=`echo $part | cut -d '_' -f 3`
		partitions[$name]+="$val=${!part} "
	done

	for x in "${!partitions[@]}";do
		printf "[%s]\n%b\n" "$x" "${partitions[$x]// /\\n}" >> version.ini
	done
fi
./tools/flash_json_update.py -c version.ini partitions.json

#msg "Generate nvram image"
#./tools/gen_nvram_zone --size=0x20000 --config=images/build.prop --output=images/nvram.bin

msg "Generate nvram image for cfg.bin"
./tools/gen_nvram_zone --size=0x20000 --config=images/default.prop --output=cfg.bin

msg "Generate rootfs and drv images"
tools/mkrootfs
rm -rf drv_tmp rootfs_tmp

if [ -f tuya_cryptfs_tool ]; then

        msg "Generate rootfs cryptfs"
	./tuya_cryptfs_tool -i images/rootfs.squashfs.bin -o images/rootfs.squashfs.bin.cry
	mv images/rootfs.squashfs.bin.cry images/rootfs.squashfs.bin

        msg "Generate user cryptfs"

	if [ -e user.bin ] ;then
	./tuya_cryptfs_tool -i user.bin -o user.bin.cry
	mv user.bin.cry user.bin
	fi

        msg "Generate data cryptfs"
	mkdir -p jffs2
	echo "0.0.1" >> jffs2/version.ini
	#  need change --pad to partition size  
mkfs.jffs2 -r jffs2 -o jffs2.bin --pad=0x50000 -e 0x10000 -s 0x1000 -n
	./tuya_cryptfs_tool -i jffs2.bin -o jffs2.bin.cry
	rm -rf jffs2
	mv jffs2.bin.cry  mtd.bin

        msg "Generate drv cryptfs"
	./tuya_cryptfs_tool -i images/drv.squashfs.bin -o images/drv.squashfs.bin.cry
	mv images/drv.squashfs.bin.cry  images/drv.squashfs.bin
fi

msg "Packing tuya firmware images"
./tools/tuya_pack $sign_param -c partitions.json

if [ -e user.bin ] ;then
mv user.bin  .user.bin
fi

if [ -f ${VENDOR_IMAGE_PACK_SCRIPT} ];then
	msg "Packing SoC vendor firmware images"
	. ${VENDOR_IMAGE_PACK_SCRIPT}
fi
