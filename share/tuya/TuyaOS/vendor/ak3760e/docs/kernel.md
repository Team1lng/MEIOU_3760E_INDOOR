# Linux Kernel 上 CI 指南
--------------------------------

## 从厂商原始 SDK 剥离 Linux Kernel
如果厂商的SDK是以git仓库的形式提供，我们可以使用 `git subtree` 命令来剥离。
- Step1:  
    在gitlab上创建新的空仓库linux-hisi3518e_v300
- Step2:  
    创建本地空仓库。
    ```
    /# cd ~/WorkSpace
    /# mkdir linux-hisi3518e_v300
    /# cd linux-hisi3518e_v300
    /# git init --bare
    ```
- Step3:  
    在SDK的仓库中创建只包含Linux代码和日志的分支。
    ```
    /# cd ${SDK}
    /# git subtree split --prefix=${path/to/kernel/dir} -b linux-split
    ```
- Step4:  
    将分离的linux-split分支推送到Step2中创建的本地仓库。
    ```
    /# git push ~/WorkSpace/linux-hisi3518e_v300) linux-split:master
    ```
- Step5:  
    发布新仓库到gitlab。
    ```
    /# cd ~/WorkSpace/linux-hisi3518e_v300
    /# git remote add origin ssh://git@registry.code.tuya-inc.top:10023/ci_bsp_group/linux-hisi3518e_v300.git
    /# git push origin master
    ```

## 添加 CI 编译脚本
BSP CI使用xmake构建系统来编译kernel，xmake的语法详见 [xmake 编译框架说明](https://code.registry.wgine.com/embed_ci_space_group/xmake/blob/master/README.md)。  
内核目录下新建 local.mk
```makefile
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_PKG_NAME := kernel

define prepare/kernel
        #可以在这里进行打补丁等操作
endef

define configure/kernel
        #配置文件一般存放在板级/平台相关的组件中，配置详情请参见board.md
        cp $(TARGET_KERNEL_CONFIG) $(kernel_build)/.config
endef

define compile/kernel
        #内核镜像需要以zImage格式提供
        #kernel_src和kernel_build是xmake创建的源码目录和编译目录变量
        make -C $(kernel_src) O=$(kernel_build) ARCH=$(TARGET_ARCH) CROSS_COMPILE=$(CROSS_COMPILE) zImage -j$(CPUS)
endef

define install/kernel
        @cp $(kernel_build)/arch/arm/boot/zImage-dtb $(TARGET_IMAGES)
endef

include $(BUILD_PACKAGE)
```
