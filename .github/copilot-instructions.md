# Copilot 指南 — AmyIPSdk

目标：让 AI 编码助手在此代码库里立刻可用，快速找到关键模块、构建/打包流程、约定和示例路径。

## 1) 大局与模块边界
- **应用入口**：[project/app_main.c](project/app_main.c) 初始化资源、SDK 和 LVGL UI，从 `lv_page(power)` 开始。
- **公共 SDK 层**：[common/Amy_sdk.c](common/Amy_sdk.c) 封装硬件抽象；[common/Amy_media_mgr.c](common/Amy_media_mgr.c) 管理媒体文件存储（SQLite 数据库，文件类型掩码如 MEDIA_MASK_NUMBERS_TYPE_VIDEO）。
- **页面/UI 层**：[project/page/](project/page/) 使用 LVGL，页面如 `page_camera_common.c` 处理摄像头配置（品牌：海康/大华，输入类型：手动/搜索/RTSP）。
- **驱动/硬件接口**：`driver/` 包含 TP9950 视频解码器、音频输入/输出、GPIO 控制等；例如 [driver/tp9950.c](driver/tp9950.c) 处理视频格式检测。
- **第三方组件**：`share/` 下如 `lvgl`（UI）、`sqlite3`（数据库）、`onvif`（网络协议）、`tuya`（IoT 功能），编译进固件。

数据流：硬件输入（摄像头、音频）→ 驱动抽象 → SDK 封装 → 应用逻辑（媒体存储、UI 显示）→ 网络输出（RTSP、ONVIF）。

## 2) 典型开发/构建工作流
- **常规构建**（交叉编译 ARM）：
  ```bash
  mkdir -p build && cd build
  cmake ..  # 使用 /opt/arm-anykav500-linux-uclibcgnueabi/ 编译器
  make -j$(nproc)
  ```
- **生成固件**：
  ```bash
  ./make_image.sh  # 运行 script/mkbin.py 打包 project/resource/ 为 build/res.bin 和 include/res.h
  ```
- **单文件调试**：VS Code 任务 `C/C++: gcc 生成活动文件` 快速编译单个 C 文件。
- **验证**：修改后 `cd build && make`，错误用 AMY_LOG_ERROR 记录；运行 `./make_image.sh` 检查固件生成。

## 3) 项目风格与约定
- **命名**：`ak_` 前缀 SDK 接口（[include/ak_*.h](include/ak_common.h)）；`Amy_` 前缀项目函数（[common/Amy_*.c](common/Amy_sdk.c)）；`page_` 前缀 UI 页面；`lv_` 前缀 LVGL 函数。
- **错误处理**：使用 AMY_LOG_* 宏（[common/Amy_log.h](common/Amy_log.h)），如 AMY_LOG_WARNING("Invalid params")；函数参数校验后记录警告。
- **UI/交互**：LVGL 对象创建用 `lv_*` 函数，页面切换通过 `lv_page()`；页面使用上下文结构体存储状态，如 [project/page/page_camera_common.c](project/page/page_camera_common.c) 的 page_camera_common_ctx_t。
- **线程/同步**：使用 Amy_thread_t (pthread_t)、Amy_mutex_t、Amy_sem_t 封装；驱动如 TP9950 使用线程检测格式。
- **构建**：CMake 子目录，修改优先 `project/` 或 `common/`，第三方库选项在顶层 CMakeLists.txt；资源文件经 mkbin.py 生成二进制头文件。

## 4) 集成点与跨组件通信
- **硬件到应用**：驱动如 [driver/i2c_ctrl.c](driver/i2c_ctrl.c) 抽象 I2C，SDK 初始化调用 `i2c_device_init()`；TP9950 视频解码器支持多种格式枚举。
- **网络/协议**：ONVIF 在 `share/onvif/` 用于设备发现；RTSP 客户端集成，见 [project/app_main.c](project/app_main.c) 包含 "live/testRTSPClient.h"。
- **媒体管理**：文件存储用 SQLite，表如 MediaTable，索引如 MediaIndex；文件类型用位掩码定义，如 MEDIA_MASK_NUMBERS_TYPE_VIDEO。
- **IoT 集成**：Tuya SDK 在 `share/tuya/` 处理设备配对、媒体事件、状态变更。

## 5) 编辑/提交提示
- **改动范围**：仅相关文件，避免格式化第三方源码；新增功能优先修改 `project/` 或 `common/`。
- **示例引用**：新增页面参考 [project/page/page_camera_common.c](project/page/page_camera_common.c) 的上下文结构体和安全拷贝；UI 组件使用 [project/page/lv_common.c](project/page/lv_common.c) 的通用创建函数。
- **编译验证**：`make -j8` 后 `./make_image.sh` 检查固件生成；硬件相关更改描述风险。

## 6) 常见定位示例
- 启动：[project/app_main.c](project/app_main.c#L29) `Amy_sdk_init(...)`
- 媒体：[common/Amy_media_mgr.c](common/Amy_media_mgr.c#L15) 数据库定义
- UI：[project/page/lv_common.c](project/page/lv_common.c) 共享函数
- 驱动：[driver/tp9950.c](driver/tp9950.c#L25) 格式枚举
- 网络：[share/onvif/onvif.c](share/onvif/onvif.c) 协议处理

## 7) 如果信息不明确
- 硬件影响更改需 PR 描述风险，请求验证。
