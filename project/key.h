#ifndef __APP_KEY_H__
#define __APP_KEY_H__

typedef enum
{
    KEY_TYPE_MECHANICAL = 0, // 机械按键
    KEY_TYPE_ADC,            // ADC触摸按键
    KEY_TYPE_MAX
} key_type_t;

/**
 * @brief 初始化按键模块，启动 GPIO/ADC 按键检测线程
 * @note  由 app_main.c 在启动时调用；事件通过 Amy_lvgl 事件总线推送，
 *        页面用 Amy_event_cb_register(AMY_EVENT_ID_KEY_MECHANICAL/KEY_ADC) 注册
 */
void key_module_init(void);

/**
 * @brief 推送按键事件（由 GPIO/ADC 检测线程调用）
 * @param type  按键类型
 * @param arg1  事件参数1
 * @param arg2  事件参数2
 */
void key_event_push(key_type_t type, unsigned long arg1, unsigned long arg2);

#endif
