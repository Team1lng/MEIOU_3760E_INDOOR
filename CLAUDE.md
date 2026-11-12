# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 项目概述

AmyIPSdk 是 AnyCloud AK3760E/3761E 平台的室内机（indoor monitor）固件应用。运行在 ARM Linux 嵌入式设备上，基于 LVGL 做 UI，通过 RTSP/ONVIF/Tuya 协议与门口机、CCTV 摄像头和涂鸦 IoT 平台通信。

## 构建

交叉编译工具链位于 `/opt/arm-anykav500-linux-uclibcgnueabi/bin/`。

```bash
# 资源打包 + CMake 构建
./make_image.sh

# 或者分步：
mkdir -p build && cd build
cmake .. && make -j$(nproc)
```

`./make_image.sh` 先运行 `script/mkbin.py` 将 `project/resource/` 打包为 `build/res.bin` 并生成 `include/res.h`（资源偏移/大小宏），再执行 cmake + make。

## 架构

```
project/app_main.c          # 入口：初始化资源/SDK/LVGL，从 lv_page(power) 启动
├── common/                  # SDK 封装层：媒体管理、音频播放、LVGL 事件系统、SD卡
│   ├── Amy_sdk.c/h         # 硬件/线程/时间等底层封装
│   ├── Amy_media_mgr.c/h   # 媒体文件管理（SQLite，位掩码过滤文件类型/通道/模式）
│   └── Amy_lvgl.c/h        # LVGL 事件总线，驱动页面切换
├── project/
│   ├── page/               # LVGL UI 页面（enter/quit 回调 + 上下文结构体）
│   ├── business/           # 业务逻辑（call/ 通话协议，device/ 设备配置同步与冲突处理）
│   ├── common/             # 跨页面共享：多语言、用户数据、网络、待机、时间、升级
│   ├── monitor.c/h         # 监控通道管理（门口机/CCTV 通道切换，录像/拍照）
│   └── intercom.c/h        # 对讲音频管理
├── driver/                  # 硬件驱动：音视频输入输出、GPIO、I2C、PWM、MJPEG
├── include/                 # 平台 SDK 头文件（ak_*.h：VI/VO/AI/AO/VENC/VDEC/TDE等）
├── lib/                     # 平台预编译静态库（libplat_*.a, libmpi_*.a等）
├── share/                   # 第三方库源码
│   ├── lvgl/ live/ onvif/ tuya/ call/ sqlite3/
│   ├── freetype-2.9.1/ jpeg-9f/ libmad-0.15.1b/
│   └── avilib/ mxml-3.3.1/ xls/
└── script/
    ├── mkbin.py            # 资源打包
    └── generate_firmware.c # 固件镜像生成
```

**数据流：** 硬件输入（摄像头/音频）→ driver 抽象 → 平台 SDK → SDK 封装层（common/）→ 应用逻辑（project/）→ 网络输出（RTSP/ONVIF/Tuya）

## 命名约定

| 前缀 | 含义 |
|------|------|
| `ak_` | 平台 SDK 接口（include/ak_*.h） |
| `Amy_` | 项目封装（common/Amy_*.c/h） |
| `page_` | LVGL UI 页面 |
| `lv_` | LVGL 库函数 |
| `sat_` | 设备/网络相关 |

## UI 页面模型

每个页面通过宏声明和定义：
```c
// page_tabel.h 中声明
extern_page(home);

// page_home.c 中定义
Amy_lv_page_create(home);  // 生成 lv_page_home 结构体，含 .enter/.quit
```

页面切换：`Amy_lv_page_goto(lv_page(xxx), LV_SCR_LOAD_ANIM_NONE, ret)`

页面间通信通过 `Amy_lvgl.c` 中的事件总线（`Amy_event_cb_register` / `Amy_xxx_event_put`）。

## 关键注意

- 构建系统自动编译 `project/page/*.c` 和 `project/business/**/*.c`，新增文件无需修改 CMakeLists.txt。
- 新增页面需在 `project/page/page_tabel.h` 中 `extern_page()` 声明。
- 资源文件通过 `mkbin.py` 打包，偏移和大小宏在 `include/res.h`。
- 编译开启了 `-Werror`，所有警告都会导致编译失败。
- `share/` 下第三方库在顶层 CMakeLists.txt 通过 `$<TARGET_OBJECTS:xxx>` 链接，新增库需同步修改。
