#ifndef _PAGE_PASSWORD_INPUT_H_
#define _PAGE_PASSWORD_INPUT_H_
#include <stdbool.h>
#include "lvgl/lvgl.h"

void page_password_input_set_title_visible(bool visible);
void page_password_input_set_title_text(const char *text);
void page_password_input_set_placeholder(const char *text);
void page_password_input_set_password_mode(bool enabled);
void page_password_input_set_pwd_toggle(bool show);

/* 自定义回调（传 NULL 恢复默认行为：back→setting, confirm→admin） */
void page_password_input_set_back_cb(lv_event_cb_t cb);
void page_password_input_set_confirm_cb(lv_event_cb_t cb);

/* 设置输入最大长度（0 = 不限） */
void page_password_input_set_max_length(uint32_t len);

/* 设置/获取文本区域内容 */
void page_password_input_set_text(const char *text);
const char *page_password_input_get_text(void);

#endif
