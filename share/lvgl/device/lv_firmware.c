#include "lvgl.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "rtthread.h"
#include "lv_firmware.h"
#include "lv_firmware_button.h"
#include "lv_firmware_resource.h"
#include "lv_firmware_label.h"
#include "lv_firmware_line.h"
#include "lv_firmware_icon.h"
#include "lv_firmware_slider.h"
#include "lv_firmware_bar.h"
#include "lv_firmware_meter.h"
#include "lv_firmware_clock.h"
#include "Amy_log.h"
#include "md5.h"

#define LINE_CHAR_MAX (1024)
// 与QT的对齐数值爆粗一致
typedef enum
{
    AlignLeft = 0x0001,
    AlignLeading = AlignLeft,
    AlignRight = 0x0002,
    AlignTrailing = AlignRight,
    AlignHCenter = 0x0004,
    AlignJustify = 0x0008,
    AlignAbsolute = 0x0010,
    AlignHorizontal_Mask = AlignLeft | AlignRight | AlignHCenter | AlignJustify | AlignAbsolute,

    AlignTop = 0x0020,
    AlignBottom = 0x0040,
    AlignVCenter = 0x0080,
    AlignBaseline = 0x0100,
    // Note that 0x100 will clash with Qt::TextSingleLine = 0x100 due to what the comment above
    // this enum declaration states. However, since Qt::AlignBaseline is only used by layouts,
    // it doesn't make sense to pass Qt::AlignBaseline to QPainter::drawText(), so there
    // shouldn't really be any ambiguity between the two overlapping enum values.
    AlignVertical_Mask = AlignTop | AlignBottom | AlignVCenter | AlignBaseline,

    AlignCenter = AlignVCenter | AlignHCenter
} LvQTAligin;

lv_text_align_t qt_aligin_to_lvgl_text_aligin(unsigned int qt_aligin)
{
    lv_text_align_t lv_aligin = LV_TEXT_ALIGN_CENTER_CENTER;

    if (qt_aligin == (AlignLeft | AlignTop))
    {
        lv_aligin = LV_TEXT_ALIGN_LEFT;
    }
    else if (qt_aligin == (AlignLeft | AlignVCenter))
    {
        lv_aligin = LV_TEXT_ALIGN_LEFT_CENTER;
    }
    else if (qt_aligin == (AlignLeft | AlignBottom))
    {
        lv_aligin = LV_TEXT_ALIGN_LEFT_BOTTOM;
    }
    else if (qt_aligin == (AlignRight | AlignTop))
    {
        lv_aligin = LV_TEXT_ALIGN_RIGHT;
    }
    else if (qt_aligin == (AlignRight | AlignVCenter))
    {
        lv_aligin = LV_TEXT_ALIGN_RIGHT_CENTER;
    }
    else if (qt_aligin == (AlignRight | AlignBottom))
    {
        lv_aligin = LV_TEXT_ALIGN_RIGHT_BUTTOM;
    }
    else if (qt_aligin == (AlignHCenter | AlignTop))
    {
        lv_aligin = LV_TEXT_ALIGN_CENTER;
    }
    else if (qt_aligin == (AlignHCenter | AlignVCenter))
    {
        lv_aligin = LV_TEXT_ALIGN_CENTER_CENTER;
    }
    else if (qt_aligin == (AlignHCenter | AlignBottom))
    {
        lv_aligin = LV_TEXT_ALIGN_CENTER_BUTTOM;
    }

    return lv_aligin;
}

lv_align_t qt_aligin_to_lvgl_aligin(unsigned int qt_aligin)
{
    lv_align_t lv_aligin = LV_ALIGN_TOP_LEFT;

    if (qt_aligin == (AlignLeft | AlignTop))
    {
        lv_aligin = LV_ALIGN_TOP_LEFT;
    }
    else if (qt_aligin == (AlignLeft | AlignVCenter))
    {
        lv_aligin = LV_ALIGN_LEFT_MID;
    }
    else if (qt_aligin == (AlignLeft | AlignBottom))
    {
        lv_aligin = LV_ALIGN_BOTTOM_LEFT;
    }
    else if (qt_aligin == (AlignRight | AlignTop))
    {
        lv_aligin = LV_ALIGN_TOP_RIGHT;
    }
    else if (qt_aligin == (AlignRight | AlignVCenter))
    {
        lv_aligin = LV_ALIGN_RIGHT_MID;
    }
    else if (qt_aligin == (AlignRight | AlignBottom))
    {
        lv_aligin = LV_ALIGN_BOTTOM_RIGHT;
    }
    else if (qt_aligin == (AlignHCenter | AlignTop))
    {
        lv_aligin = LV_ALIGN_TOP_MID;
    }
    else if (qt_aligin == (AlignHCenter | AlignVCenter))
    {
        lv_aligin = LV_ALIGN_CENTER;
    }
    else if (qt_aligin == (AlignHCenter | AlignBottom))
    {
        lv_aligin = LV_ALIGN_BOTTOM_MID;
    }

    return lv_aligin;
}
// 提取字符串的数据，数值或者字符串,返回字符串下一个位置
char *firmware_extract_between_to_string(char *str, const char *start, const char *end, char **extract)
{
    // 查找起始字符串的位置
    if ((str == NULL) || (str[0] == '\0'))
    {
        return NULL;
    }

    char *start_pos = str;
    if (start != NULL)
    {
        start_pos = strstr(str, start);
        if (start_pos == NULL)
        {
            return NULL; // 起始字符串未找到
        }
        start_pos += strlen(start); // 移动到起始字符串之后
    }

    // 查找结束字符串的位置
    char *end_pos = strstr(start_pos, end);
    if (end_pos == NULL)
    {
        return NULL; // 结束字符串未找到
    }
    *end_pos = '\0';
    *extract = start_pos;
    return end_pos + strlen(end);
}
char *firmware_extract_between_to_value(char *str, const char *start, const char *end, unsigned long *extract, int base)
{
    // 查找起始字符串的位置
    if ((str == NULL) || (str[0] == '\0'))
    {
        return NULL;
    }
    *extract = 0;
    char *start_pos = str;
    if (start != NULL)
    {
        start_pos = strstr(str, start);
        if (start_pos == NULL)
        {
            return NULL; // 起始字符串未找到
        }
        start_pos += strlen(start); // 移动到起始字符串之后
    }

    // 查找结束字符串的位置
    char *end_pos = strstr(start_pos, end);
    if (end_pos == NULL)
    {
        return NULL; // 结束字符串未找到
    }
    *end_pos = '\0';

    char *end_ptr;
    *extract = strtoul(start_pos, &end_ptr, base);
    return end_pos + strlen(end);
}

char *firmware_alloc_extract_between(const char *str, const char *start, const char *end)
{
    // 查找起始字符串的位置
    const char *start_pos = strstr(str, start);
    if (start_pos == NULL)
    {
        return NULL; // 起始字符串未找到
    }
    start_pos += strlen(start); // 移动到起始字符串之后

    // 查找结束字符串的位置
    const char *end_pos = strstr(start_pos, end);
    if (end_pos == NULL)
    {
        return NULL; // 结束字符串未找到
    }

    // 计算要提取的字符串长度
    size_t length = end_pos - start_pos;

    // 分配内存并复制子字符串
    char *result = (char *)lv_mem_alloc(length + 1);
    if (result == NULL)
    {
        return NULL; // 内存分配失败
    }

    // 添加字符串结束符
    strncpy(result, start_pos, length);
    result[length] = '\0';

    return result;
}

int firmware_parse_value(const char *src_str, const char *prefix, unsigned long *out_value, int base)
{
    if (src_str == NULL || prefix == NULL)
    {
        return -1;
    }

    const char *start_pos = strstr(src_str, prefix);
    if (!start_pos)
    {
        return -1;
    }

    size_t prefix_len = strlen(prefix);

    const char *value_str = start_pos + prefix_len;
    char *endptr = NULL;

    // 转换十六进制字符串
    unsigned long value = strtoul(value_str, &endptr, base);

    // 检查转换是否成功
    if (value > 0xFFFFFFFF)
    {
        return -3;
    }

    *out_value = value;
    return 0;
}

char *firmware_parse_string(const char *src_str, const char *prefix)
{
    if (src_str == NULL || prefix == NULL)
    {
        return NULL;
    }

    const char *start_pos = strstr(src_str, prefix);
    if (!start_pos)
    {
        return NULL;
    }

    start_pos += strlen(prefix);
    if (!start_pos)
    {
        return NULL;
    }

    const char *end_pos = strstr(start_pos, ",");
    if (!end_pos)
    {
        return NULL;
    }

    int length = end_pos - start_pos;

    char *outstring = lv_mem_alloc(length + 1);
    strncpy(outstring, start_pos, length);
    outstring[length] = '\0';
    return outstring;
}

int firmware_parse_range(const char *src_str, const char *prefix, unsigned long *out_min, unsigned long *out_max)
{
    if (src_str == NULL || prefix == NULL || out_min == NULL || out_max == NULL)
    {
        return -1;
    }

    // 检查字符串是否以 "bgimg=[" 开头
    size_t prefix_len = strlen(prefix);
    const char *start_pos = strstr(src_str, prefix);
    if ((!start_pos) || (*(start_pos + prefix_len) != '['))
    {
        return -2;
    }

    const char *range_start = start_pos + prefix_len + 1;
    const char *range_end = strchr(range_start, ']');
    if (range_end == NULL)
    {
        return -3;
    }

    // 复制范围字符串
    char range_str[64];
    size_t range_len = range_end - range_start;
    if (range_len >= sizeof(range_str))
    {
        return -4;
    }
    strncpy(range_str, range_start, range_len);
    range_str[range_len] = '\0';

    // 解析两个数字
    char *endptr1, *endptr2;
    unsigned long val1 = strtoul(range_str, &endptr1, 10);
    if ((range_str == endptr1) || (*endptr1 != ','))
    {
        return -5;
    }

    const char *second_num = endptr1 + 1;
    unsigned long val2 = strtoul(second_num, &endptr2, 10);
    if ((range_str == endptr1) || (*endptr2 != '\0'))
    {
        return -6;
    }

    *out_min = val1;
    *out_max = val2;
    return 0;
}

static void obj_reversal_cb(lv_event_t *ev)
{
    lv_obj_t *obj = (lv_obj_t *)ev->current_target;
    lv_obj_t *action_obj = lv_obj_get_target_obj(obj);
    if (action_obj)
    {
        if (lv_obj_has_flag(action_obj, LV_OBJ_FLAG_HIDDEN) == true)
        {
            lv_obj_clear_flag(action_obj, LV_OBJ_FLAG_HIDDEN);
        }
        else
        {
            lv_obj_add_flag(action_obj, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

static void obj_hide_cb(lv_event_t *ev)
{
    lv_obj_t *obj = (lv_obj_t *)ev->current_target;
    lv_obj_t *action_obj = lv_obj_get_target_obj(obj);
    if (action_obj)
    {
        if (lv_obj_has_flag(action_obj, LV_OBJ_FLAG_HIDDEN) == false)
        {
            lv_obj_add_flag(action_obj, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

static void obj_show_cb(lv_event_t *ev)
{
    lv_obj_t *obj = (lv_obj_t *)ev->current_target;
    lv_obj_t *action_obj = lv_obj_get_target_obj(obj);
    if (action_obj)
    {
        if (lv_obj_has_flag(action_obj, LV_OBJ_FLAG_HIDDEN) == true)
        {
            lv_obj_clear_flag(action_obj, LV_OBJ_FLAG_HIDDEN);
        }
    }
}
static void obj_goto_cb(lv_event_t *ev)
{
    lv_obj_t *obj = (lv_obj_t *)ev->current_target;
    lv_obj_t *target_scr = lv_obj_get_target_scr(obj);
    if ((target_scr) && (target_scr->parent == NULL))
    {
        firmware_resource_destroy();
        lv_scr_load(target_scr);
    }
}

int firmware_parse_event_process(lv_obj_t *screen, lv_obj_t *obj, const char *line)
{
    char *event_start = strstr(line, "event=");
    if (!event_start)
    {
        return -1;
    }

    char *str = event_start + strlen("event=");
    if (!str)
    {
        return -1;
    }

    char *event_str = lv_mem_alloc(strlen(str) + 1);
    strcpy(event_str, str);
    char *token = strtok(event_str, ":");
    // 被按下
    lv_event_code_t event_code = LV_EVENT_ALL;
    if (strcmp(token, "PRESS") == 0)
    {
        event_code = LV_EVENT_SHORT_CLICKED;
    }
    else if (strcmp(token, "RELEASE") == 0)
    {
        event_code = LV_EVENT_RELEASED;
    }
    if (event_code != LV_EVENT_ALL)
    {
        token = strtok(NULL, ":");
        // 控件控制
        if (strcmp(token, "DISPCTR") == 0)
        {
            // 控件名称
            token = strtok(NULL, ":");
            lv_obj_set_target_obj_name(obj, token);
            // 控制方式
            token = strtok(NULL, ":");
            if (strcmp(token, "SHOW") == 0)
            {
                lv_obj_add_event_cb(obj, obj_show_cb, event_code, NULL);
            }
            else if (strcmp(token, "HIDE") == 0)
            {
                lv_obj_add_event_cb(obj, obj_hide_cb, event_code, NULL);
            }
            else if (strcmp(token, "REVERSAL") == 0)
            {
                lv_obj_add_event_cb(obj, obj_reversal_cb, event_code, NULL);
            }
        }
        else if (strcmp(token, "GOTO") == 0)
        {
            token = strtok(NULL, ":");
            lv_obj_set_target_obj_name(obj, token);
            lv_obj_add_event_cb(obj, obj_goto_cb, event_code, NULL);
        }
    }

    lv_mem_free(event_str);

    char *event_next = strstr(event_start, ",");
    if (!event_next)
    {
        return -1;
    }
    return firmware_parse_event_process(screen, obj, event_next);
}

static lv_obj_t *parse_screen_config(const char *line)
{
    lv_obj_t *scr = NULL;
    // 寻找屏幕名称
    char *name = firmware_alloc_extract_between(line, "@", "[");
    if (!name)
    {
        return NULL;
    }

    scr = lv_obj_create(NULL);
    lv_obj_set_id(scr, name);
    LV_LOG_ERROR("create screen:%s\n", name);
    lv_mem_free(name);
    unsigned long bg_color = 0x00;
    if (firmware_parse_value(line, "bgcolor=", &bg_color, 16) == 0)
    {
        lv_obj_set_style_bg_color(scr, lv_color_hex(bg_color), LV_PART_MAIN);
        unsigned char alpha = bg_color >> 24;
        lv_obj_set_style_bg_opa((lv_obj_t *)scr, alpha, LV_PART_MAIN);
        // LV_LOG_ERROR("BG COLOR =%#X\n", bg_color);
    }

    unsigned long offset, size;
    if (firmware_parse_range(line, "bgimg=", &offset, &size) == 0)
    {
        // 设置背景图片
        firamware_resource_init(data, offset, size);
        lv_obj_set_style_bg_img_src(scr, &data, LV_PART_MAIN);
    }

    unsigned long qt_Aligin;
    if (firmware_parse_value(line, "imgalign=", &qt_Aligin, 16) == 0)
    {
        // 设置背景图片对齐方式
        lv_align_t lv_aligin = qt_aligin_to_lvgl_aligin(qt_Aligin);
        lv_obj_set_style_bg_img_align(scr, lv_aligin, LV_PART_MAIN);
    }

    // 事件解析处理
    firmware_parse_event_process(scr, scr, line);

    return scr;
}

static void parse_widget_config(lv_obj_t *screen, const char *line)
{
    char *type = NULL;
    type = firmware_parse_string(line, "type=");
    if (strcmp(type, "button") == 0)
    {
        firmware_parse_btn_config(screen, line);
    }
    else if (strcmp(type, "icon") == 0)
    {
        firmware_parse_icon_config(screen, line);
    }
    else if (strcmp(type, "label") == 0)
    {
        firmware_parse_label_config(screen, line);
    }
    else if (strcmp(type, "line") == 0)
    {
        firmware_parse_line_config(screen, line);
    }
    else if (strcmp(type, "slider") == 0)
    {
        firmware_parse_slider_config(screen, line);
    }
    else if (strcmp(type, "bar") == 0)
    {
        firmware_parse_bar_config(screen, line);
    }
    else if (strcmp(type, "meter") == 0)
    {
        firmware_parse_meter_config(screen, line);
    }
    else if (strcmp(type, "clock") == 0)
    {
        firmware_parse_clock_config(screen, line);
    }
    if (type)
    {
        lv_mem_free(type);
    }
}

static int read_line_optimized(const struct fal_partition *part_dev, uint32_t addr, char *buf, int max_len)
{
    uint8_t temp_buf[64]; // 建议与Flash块大小对齐
    int total_read = 0;
    int remaining = max_len - 1; // 保留空间给'\0'

    while (remaining > 0)
    {
        // 计算本次读取大小（对齐优化）
        int chunk_size = LV_MIN(sizeof(temp_buf), remaining);
        int ret = fal_partition_read(part_dev, addr + total_read, temp_buf, chunk_size);

        // 处理读取失败或结束
        if (ret <= 0)
        {
            buf[total_read] = '\0';
           // AMY_LOG_DEBUG("read:%s\n", buf);
            return (total_read > 0) ? (total_read + 1) : -1;
        }

        // 搜索换行符
        char *newline = memchr(temp_buf, '\n', ret);
        if (newline)
        {
            int line_len = newline - (char *)temp_buf;
            if (total_read + line_len >= max_len - 1)
            {
                line_len = max_len - total_read - 1; // 确保不越界
            }

            memcpy(buf + total_read, temp_buf, line_len);
            buf[total_read + line_len] = '\0';
           // AMY_LOG_DEBUG("read:%s\n", buf);
            return total_read + line_len + 1;
        }

        // 无换行符则完整复制
        if (total_read + ret > max_len - 1)
        {
            ret = max_len - total_read - 1; // 截断保护
        }
        memcpy(buf + total_read, temp_buf, ret);
        total_read += ret;
        remaining -= ret;
    }

    buf[total_read] = '\0';
   // AMY_LOG_DEBUG("read:%s\n", buf);
    return total_read + 1; // 返回实际读取长度（不含'\0'）
}
static uint32_t swap_endian_uint32(uint32_t value)
{
    return ((value >> 24) & 0xFF) |      // 移动字节3到位置0
           ((value >> 8) & 0xFF00) |     // 移动字节2到位置1
           ((value << 8) & 0xFF0000) |   // 移动字节1到位置2
           ((value << 24) & 0xFF000000); // 移动字节0到位置3
}
int parse_lvgl_config(const struct fal_partition *part_dev)
{
    unsigned int resource_base_addres = 0; // (unsigned int)part_dev->offset;
    unsigned int cfg_data_addres = 0;
    unsigned int cfg_data_size = 0;
    unsigned int resource_data_addres = 0;
    unsigned int resource_data_size = 0;

    // md5:16byte+1byte +cfgsize 4byte+cfg内容+ resourcesize:4byte+resource内容

    // read md5
    unsigned char md5[16];
    if (fal_partition_read(part_dev, resource_base_addres, md5, sizeof(md5)) != sizeof(md5))
    {
        AMY_LOG_ERROR("read resouces md5 failed\n");
        return -1;
    }

    AMY_LOG_DEBUG("md5:");
    for (int i = 0; i < 16; i++)
    {
        printf("%#x", md5[i]);
    }
    printf("\n");

    // 读取cfg size;
    cfg_data_addres = resource_base_addres + sizeof(md5) + 1; // md5结算后有一个换行符
    if (fal_partition_read(part_dev, cfg_data_addres, (uint8_t *)&cfg_data_size, sizeof(cfg_data_size)) != sizeof(cfg_data_size))
    {
        AMY_LOG_ERROR("read resouces cfg failed\n");
        return -1;
    }
    // 跳过4字节文件大小
    cfg_data_addres += 4;

    AMY_LOG_DEBUG("cfg_data_size:%#x ->%#x\n", cfg_data_size, swap_endian_uint32(cfg_data_size));
    cfg_data_size = swap_endian_uint32(cfg_data_size);

    if (cfg_data_size > part_dev->len)
    {
        AMY_LOG_ERROR("cfg size(%d) over full(part size:%d)\n", cfg_data_size, part_dev->len);
        return -1;
    }

    // 读取resource size
    resource_data_addres = cfg_data_addres + cfg_data_size;
    if (fal_partition_read(part_dev, resource_data_addres, (uint8_t *)&resource_data_size, sizeof(resource_data_size)) != sizeof(resource_data_size))
    {
        AMY_LOG_ERROR("read resouces cfg failed\n");
        return -1;
    }
    // 跳过4字节文件大小
    resource_data_addres += 4;

    AMY_LOG_DEBUG("resource data size:%#x ->%#x\n", resource_data_size, swap_endian_uint32(resource_data_size));
    resource_data_size = swap_endian_uint32(resource_data_size);

    if ((resource_data_size > part_dev->len) || ((resource_data_size + cfg_data_size) > part_dev->len))
    {
        AMY_LOG_ERROR("resource size(%d) over full(part size:%d) cfg size:%d\n", resource_data_size, part_dev->len, cfg_data_size);
        return -1;
    }

    char *line = (char *)rt_malloc(LINE_CHAR_MAX);

    // 开始校验md5
    MD5_CTX md5_ctx;
    MD5Init(&md5_ctx);

    int read_total = 0;
    int remain_len = (cfg_data_size + resource_data_size + 4 + 4);

    unsigned int check_start_addres = resource_base_addres + sizeof(md5) + 1;
    while (remain_len > 0)
    {
        int read_len = remain_len;
        if (remain_len > LINE_CHAR_MAX)
        {
            read_len = LINE_CHAR_MAX;
        }

        read_len = fal_partition_read(part_dev, check_start_addres + read_total, (uint8_t *)line, read_len);
        if (read_len <= 0)
        {
            AMY_LOG_ERROR("fal patition read failed");
            return -1;
        }
        remain_len -= read_len;
        read_total += read_len;
        MD5Update(&md5_ctx, (unsigned char *)line, read_len);
    }
    unsigned char digest[16] = {0};
    MD5Final(digest, &md5_ctx);

    AMY_LOG_DEBUG("digest:");
    for (int i = 0; i < 16; i++)
    {
        printf("%#x", digest[i]);
    }
    printf("\n");

    if (memcmp(digest, md5, 16) != 0)
    {
        AMY_LOG_ERROR("resoruce data invalid\n");
        return -1;
    }

    firamware_resource_set(part_dev, resource_data_addres, resource_data_size);

    lv_obj_t *current_screen = NULL;

    read_total = 0;
    remain_len = cfg_data_size;
    while (remain_len > 0)
    {
        int read_len = remain_len;
        if (remain_len > LINE_CHAR_MAX)
        {
            read_len = LINE_CHAR_MAX;
        }
        read_len = read_line_optimized(part_dev, cfg_data_addres + read_total, line, read_len);
        if (read_len <= 0)
        {
            AMY_LOG_ERROR("read line optimized failed\n");
            break;
        }

        read_total += read_len;
        remain_len -= read_len;

        // 跳过注释和空行
        if ((line[0] == '#') || (line[0] == '\n'))
        {
            continue;
        }
        line[strcspn(line, "\n")] = '\0';

        // 解析屏幕定义
        if (line[0] == '@')
        {
            lv_obj_t *scr = parse_screen_config(line);
            if (current_screen == NULL)
            {
                firmware_resource_destroy();
                lv_scr_load(scr);
            }
            current_screen = scr;
        }
        else if (current_screen && (strncmp(line, "type=", 5) == 0))
        {
            parse_widget_config(current_screen, line);
        }
    }
    rt_free(line);
    return 0;
}
