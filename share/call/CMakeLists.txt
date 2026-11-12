# 核心模块源文件
set(CORE_SOURCES src/core/call_core.c)
# 事件模块源文件
set(EVENT_SOURCES src/event/call_event.c src/event/call_event_task_manager.c)
# 业务模块源文件e
set(BUSINESS_SOURCES src/business/call_action.c src/business/call_heartbeat.c src/business/call_record.c)
# # 通信接口实现文件
# set(INTERFACE_SOURCES src/interface/call_comm_interface.c src/interface/call_event_hook.c)
# 内部通用工具源文件
set(UTILS_SOURCES src/utils/common_utils.c)
# 对外接口适配层（补充你的 API_SOURCES，之前遗漏了定义）
set(API_SOURCES src/call_api.c)

# 合并所有源文件（OBJECT 库不变）
add_library(call OBJECT
    ${CORE_SOURCES}
    ${EVENT_SOURCES}
    ${BUSINESS_SOURCES}
    # ${INTERFACE_SOURCES}
    ${UTILS_SOURCES}
    ${API_SOURCES}
)

# 设置头文件搜索路径（关键：直接添加各模块头文件目录）
target_include_directories(
    call  # 目标名与 add_library 一致
    PRIVATE
        # 1. 核心模块头文件目录（src/core/）：支持 #include "call_core.h"
        ${CMAKE_CURRENT_SOURCE_DIR}/src/core
        # 2. 事件模块头文件目录（src/event/）：支持 #include "call_event.h"、#include "call_event_dispatcher.h"
        ${CMAKE_CURRENT_SOURCE_DIR}/src/event
        # 3. 业务模块头文件目录（src/business/）：支持 #include "call_action.h" 等
        ${CMAKE_CURRENT_SOURCE_DIR}/src/business
        # 4. 内部通用工具头文件目录
        ${CMAKE_CURRENT_SOURCE_DIR}/src/utils
        # 5. 对外接口头文件目录（include/）：支持 #include "call_api.h"
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        # # 6. 内部钩子函数文件目录
        # ${CMAKE_CURRENT_SOURCE_DIR}/src/interface
)