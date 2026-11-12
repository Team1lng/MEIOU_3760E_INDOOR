#### 产测包命名规则

tuya_+平台名_product_+产测包发布日期.zip
例如：tuya_ak39ev330_product_20211208.zip

#### 产测包内容说明

~~~
config 目录
平台名.zip 
平台名.zip.sign
config.ini 
readme.txt
test_set.json
test.pcm
unzip
~~~

具体含义如下：
**config**：config 目录下放着对应平台的不同产品的配置文件，命名规则是 test_set_+产品名.json, 当同一个产测包需要用在不同产品上时，可以通过把config下的对应配置文件，覆盖当前目录下的test_set.json 配置文件即可。
**平台名.zip**: 对应平台的产测运行包文件，包含可执行文件，平台间不能混用
**平台名.zip.sign**：对应平台的产测运行包签名文件，目前默认不支持签名，如需要签名需要开发平台能够访问tuya云，并在BSP对应产品配置中添加CONFIG_PRODUCT_SIGN
**confg.ini**: 用于配置产测联网信息和测试模式
**readme.txt**：产测的一些说明
**test_set.json**：产测配置信息，用于统一产测包多产品使用
**test.pcm**：音频测试的测试文件，pcm 格式，可替换自己的对应pcm格式的音源。
**unzip**: 产测包解压工具

#### 产测包使用说明

工具： 
	产测工单
	sd 卡读卡器 
	sd 卡一张，已格式化成fat32 
	路由器一台，能访问涂鸦云，电脑和设备能正常网络通讯
    电脑一台 （安装涂鸦解决方案产测平台，支持网络连接，sd卡读卡器访问）

1.pc上用unzip解压产测包,并把产测包放内的内容拷贝到sd 卡的根目录

2.根据当前的网络环境和对应设备的网络连接方式修改config.ini，详见下面的修改config.ini

3.将含有产测包的sd 插入ipc 待测设备，上电设备，有喇叭的情况下,会听到提示音，这时就可与tuya 提供的上位机和工单进行产测了。

#### config.ini 需改

##### 1.修改产测的网络配置

打开产测固件包里面的config.ini文件，关于网络配置的内容如下：

~~~
ssid=mzyl
password=12345678
ipaddr=192.168.43.238
netmask=255.255.255.0
interface=wlan0
usbethernet=192.168.100.254
ethip=192.168.1.254
pc=192.168.43.102
~~~

具体含义如下：
**ssid**：给设备配置网络使用的路由器的网络名字
**password**：路由器网络的密码
**ipaddr**：给设备设定的IP地址
**netmask**：设备IP地址的网关
**interface**：无线接口名称
**usbethernet**：如果设备支持USB RNDIS功能，这个就是USB的IP地址
**ethip**：如果设备支持网线接口，这个就是设置的网口的IP地址
**pc**：上位机所在PC端的IP地址
一般的我们只需要按照实际情况修改ssid、password、ipaddr、pc这几个变量的设置

##### 2.产测模式切换

打开产测固件包里面的config.ini文件，关于网络配置的内容如下：

~~~
aging_test=0
run_time=12
~~~

aging_test表示产测的模式，目前支持3种模式：aging_test=0，普通的产测模式，上位机依次测试各个测试项；
aging_test=1，老化模式；aging_test=2，单板测试模式。
run_time表示产测程序运行的时间，单位是小时，一般在老化的时候，根据实际需求可以修改。

#### test_set.json修改

产测包用于当前不支持的产品型号或者外设接口改变时，需要修改sd卡根目录下的产测配置文件

##### 修改设备的固件名

打开与设备相关的配置文件test_set.json文件，固件名称格式如下：

~~~
"board": "sc084wn04_ipc_ty",
~~~

board的值就是固件的名称，通过固件的名称就可以判断使用的配置文件与设备是否相符，配置名必须与建工单时的固件名一致，在烧录授权的时候要比对。

##### 图像镜像反转配置

当图像与实际情况出现上下或左右反转时，或者sensor 型号 配置文件路径发生改变时，可用下面参数修改：

打开与设备相关的配置文件test_set.json文件，与镜像翻转相关的内容如下：

~~~
"camera_1": {
	"enable":"1",
	"sensor_type":"sc401ai",
    "sensor_version":"20211206.0.1",
    "sensor_config":"/tuya/app/skyeye/resource/isp/h3b_isp_sc401ai_mipi_2lane.conf",
		"sensor_mirror":"1",
		"sensor_flip":"1",
	},
~~~
sensor_type：sensor 类型，具体sensor 型号
sensor_version：sensor 版本，取件senor code 版本
sensor_config： sensor 配置文件路径
sensor_mirror与图像的左右镜像有关，如果在图像测试的时候发现图像左右镜像了，可以修改这个值，在0与1之间切换。
sensor_flip与图像的上下镜像有关，如果在图像测试的时候发现图像上下颠倒了，可以修改这个值，在0与1之间切换。



##### 图像测试参数配置

当出现图像卡顿，图像清晰度不够等情况可以通过下面参数修正：

~~~
"media_cfg":{
	"enable":"1",
	"video_num":"0",
	"video_fps":"20",
	"video_gop":"40",
	"video_bitrate":"1536",
	"video_width":"2560",
	"video_height":"1440",
	"video_codectype":"4",
	"video_minQp":"38",
	"video_maxQp":"51"
},
~~~

这些是与图像测试相关的参数，这些参数不能随意改动的，这里最重要的参数是video_width和video_height，即是分辨率，分辨率会影响摄像头的清晰度，video_width与video_height相乘，得到的就是像素，与实际的摄像头参数一样，清晰度才是最好的。



##### 光敏的相关设置

当夜视白天切换模式阀值或者软硬光敏性质改变，可以通过下面的参数修改

~~~
"adc_1": {
	"enable":"1",
	"index":"2",
	"type": "0",
	"day2night_threshold":"400",
	"night2day_threshold":"1180"
},
~~~

光敏实际上分为软光敏和硬光敏，光敏值用于白天和黑夜模式之间的自动切换
enable：使能设置，是否支持暗箱测试，1:支持，0:不支持；
type：光敏的类型，0：软光敏，1：硬光敏；
index：硬光敏对应的adc采集的通道；
day2night_threshold与night2day_threshold是白天和黑夜模式自动切换的阈值，每个产品的值都是不一样的。


##### 老化测试项的配置

用于修改老化测试时，对应测试项的支持。

~~~
	"aging_test_cfg":{
		"enable":"1",
		"camera_test":"1",
		"audio":"1",
		"irled_test":"1",
		"led_group_test":"1",
		"audio_test":"1",
		"white_led_test":"1",
		"motor_test":"1",
		"sleep_s":"1"
	},
~~~

camera_test、audio、irled_test、led_group_test、led_group_test、audio_test、white_led_test、motor_test就是各个测试项，这些测试项的值为1，就是选中，在老化的时候，会周期性的执行这些测试项，根据实际需求，可以将这些键的值设置为0或1。

##### 配置wifi mac 写位置和方式

配置 wifi 型号 和 mac 地址写位置
~~~
	"wifi":{
		"enable":"1",
		"wifi_type":"rtl8188",
		"mac_in_fuse":"1"
	},
~~~
enable： 1：wifi 测试支持，0： 不支持
mac_in_fuse： wifi mac是否写到efuse里
wifi_type: 用于区分wifi写efuse的方式

##### 电机参数配置 
~~~
	"motor":{
		"enable":"1",
		"dev_name":"/dev/tymotor",
		"pan_speed":"1",
		"tilt_speed":"1",
		"pan_max_step":"4000",
		"tilt_max_step":"2000",
		"limit_switch":"0"
	},
~~~

enable：1：motor 测试支持，0：不支持
dev_name： motor 驱动 节点路径
pan_speed: 水平转速
tilt_speed：垂直转速
pan_max_step：水平最大转矩 
tilt_max_step：垂直最大转矩
limit_switch：限位开关是否支持，1:支持，0，不支持

##### 类gpio设备的配置（ircut,led speaken,irled，key） 

~~~
"enable":"1",
"controlled_by_pwm":"0",
"gpio_num": "47",
"high_active": "1",
"is_in_group":"1"
~~~

enable：1，类gpio设备测试支持 2.不支持
controlled_by_pwm：是否通过pwm 来控制 
gpio_num： gpio number  
high_active:类gpio设备的有效控制电平：1，高电平，0，低电平
is_in_group: 类gpio设备，是否在同一组内测试，用于多个led或irled测试

最新的更新文档放在 venvor/产品/test_set/docs 下。
