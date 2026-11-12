# TuyaOS Kernel Adapter Layer

## 简介

Tuya OS Kernel 的目标是为上层软件提供稳定、可靠无差别的运行环境。开发者可以自主参考 Tuya 制定的平台导入标准完成导入，并通过 TuyaOS Test Tools 提供的自动化测试用例，完成开发验收。 



## 目录介绍

```
/kernel
├── config               #配置管理工具
├── docs                 #文档目录
├── include              #外部头文件
├── libs	             #生成的库文件目录
├── scripts              #使用过程需要的脚本目录
├── tools                #工具目录
├── samples              #示例代码目录
├── vendor               #原厂库代码、工具链、库、操作系统
├── adapter              #适配原厂库代码，组件化按配置裁剪
├── CHANGELOG            #修改记录
├── LICENSE              #开源LICENSE
├── Makefile             #编译入口
├── README               #整体介绍
```