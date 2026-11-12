/**
 * @file        call_event_internal.h
 * @brief       通话事件模块内部头文件
 * @details     仅内部模块使用，负责事件读取、处理、分发
 * @ingroup     call_event
 */

#ifndef __CALL_EVENT_INTERNAL_H__
#define __CALL_EVENT_INTERNAL_H__

/*********************************************************************
 * 头文件说明: 通话事件模块头文件（内部模块依赖，不对外发布）
 * 适用范围: 工程内部核心模块（call_core）及事件调度器模块
 * 核心作用: 1. 声明事件模块核心接口（初始化/反初始化）；
 *          2. 依赖事件调度器模块的类型定义，确保接口兼容性；
 *          3. 明确模块职责边界：仅负责事件读取、处理、分发，队列插入逻辑在外部实现。
 * 依赖说明: 依赖 call_event_dispatcher.h（事件类型、结构体定义）
 ********************************************************************/

/**
 * @brief 初始化事件模块（核心入口接口）
 * @param 无
 * @return true=初始化成功，false=失败（线程/锁创建失败）
 * @note 仅 call_core 调用，需在全局队列/内存池初始化后执行
 * @note 初始化后自动启动事件处理线程
 */
bool call_event_module_init(void);

/**
 * @brief 反初始化事件模块（资源释放接口）
 * @param 无
 * @return true=反初始化成功，false=失败（线程退出失败）
 * @note 仅 call_core 调用，需在系统反初始化阶段执行
 * @note 停止线程、释放内部锁，不影响外部队列/内存池
 */
bool call_event_module_uninit(void);

/**
 * @brief 内部接口 - 事件入队
 * @param attr 事件属性结构体指针（不可为NULL）
 * @return 0=成功，-1=失败（参数无效/系统未运行/资源不足）
 * @note 深拷贝事件数据，非阻塞入队
 * @note 仅内部使用，外部不可调用
 */
int call_call_event_put(call_cb_attr *attr);

#endif