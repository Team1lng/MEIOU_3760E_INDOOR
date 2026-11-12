#include "tuya_uuid_and_key.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include "xls/xls.h"

/***** 路径宏定义 *****/
#define TUYA_UUID_AND_KEY_XLS_PATH "/mnt/tf/tuya_key/" // SD卡中涂鸦UUID/KEY的XLS文件存储路径
#define TUYA_UUID_AND_KEY_CONF_PATH "/home/tuya/conf/" // 本地存储的UUID/KEY配置文件路径
#define XLS_FILE_NAME_LEN 14                           // XLS文件名长度（格式：xxxx-xx-xx.xls）
#define CONF_FILE_NAME_LEN 21                          // 配置文件名长度（格式：xxxx-xx-xx-xxxxx.conf）
#define MAX_BUFFER_LEN 1024                            // 通用缓冲区最大长度
#define MIN_YEAR 2022                                  // 合法文件名起始年份
#define MAX_YEAR 2098                                  // 合法文件名结束年份

/***** 全局变量 *****/
static char tuya_uuid_and_key_find_file[256] = {0}; // 找到的XLS/配置文件名
static bool is_tuya_uuid_and_key_register = false;  // 涂鸦UUID/KEY是否已注册（本地有配置文件）

/**
 * @brief 检查本地是否存在已注册的UUID/KEY配置文件
 * @return true-存在有效配置文件，false-不存在或配置文件无效
 */
static bool tuya_uuid_and_key_exist_check(void)
{
    struct dirent *pdirent = NULL;
    struct tm tm = {0};
    int find_count = 0;
    int index = 0;

    is_tuya_uuid_and_key_register = false;
    memset(tuya_uuid_and_key_find_file, 0, sizeof(tuya_uuid_and_key_find_file));

    DIR *d_info = opendir(TUYA_UUID_AND_KEY_CONF_PATH);
    if (d_info == NULL)
    {
        return false;
    }

    while ((pdirent = readdir(d_info)) != NULL)
    {
        // 筛选普通文件、后缀为.conf、长度符合要求
        if ((pdirent->d_type & DT_REG) && (strstr(pdirent->d_name, ".conf")) && (strlen(pdirent->d_name) == CONF_FILE_NAME_LEN))
        {
            // 解析文件名中的日期和索引
            if (sscanf(pdirent->d_name, "%04d-%02d-%02d-%05d.conf", &tm.tm_year, &tm.tm_mon, &tm.tm_mday, &index) != 4)
            {
                continue;
            }
            // 校验日期合法性
            if ((tm.tm_year >= MIN_YEAR) && (tm.tm_year <= MAX_YEAR) &&
                (tm.tm_mon > 0) && (tm.tm_mon < 13) &&
                (tm.tm_mday > 0) && (tm.tm_mday < 32))
            {
                snprintf(tuya_uuid_and_key_find_file, sizeof(tuya_uuid_and_key_find_file), "%s", pdirent->d_name);
                find_count++;
                break;
            }
        }
    }
    closedir(d_info);

    if (find_count > 0)
    {
        is_tuya_uuid_and_key_register = true;
        return true;
    }
    return false;
}

/**
 * @brief 检查SD卡中是否存在有效的涂鸦UUID/KEY XLS文件
 * @return true-存在且唯一有效XLS文件，false-不存在或存在多个无效文件
 */
static bool tuya_key_xls_exist_check(void)
{
    if (is_tuya_uuid_and_key_register)
    {
        return true;
    }

    struct tm tm = {0};
    struct dirent *pdirent = NULL;
    int find_count = 0;
    char xls_name[XLS_FILE_NAME_LEN + 1] = {0};

    DIR *d_info = opendir(TUYA_UUID_AND_KEY_XLS_PATH);
    if (d_info == NULL)
    {
        printf("open XLS path %s failed: %s\n", TUYA_UUID_AND_KEY_XLS_PATH, strerror(errno));
        return false;
    }

    while ((pdirent = readdir(d_info)) != NULL)
    {
        // 筛选普通文件、后缀为.xls、长度符合要求
        if ((pdirent->d_type & DT_REG) && (strstr(pdirent->d_name, ".xls")) && (strlen(pdirent->d_name) == XLS_FILE_NAME_LEN))
        {
            // 解析文件名中的日期
            if (sscanf(pdirent->d_name, "%04d-%02d-%02d.xls", &tm.tm_year, &tm.tm_mon, &tm.tm_mday) != 3)
            {
                continue;
            }
            // 校验日期合法性
            if ((tm.tm_year >= MIN_YEAR) && (tm.tm_year <= MAX_YEAR) &&
                (tm.tm_mon > 0) && (tm.tm_mon < 13) &&
                (tm.tm_mday > 0) && (tm.tm_mday < 32))
            {
                find_count++;
                snprintf(xls_name, sizeof(xls_name), "%s", pdirent->d_name);
            }
        }
    }
    closedir(d_info);

    // 仅当存在唯一有效XLS文件时返回true
    if (find_count == 1)
    {
        memset(tuya_uuid_and_key_find_file, 0, sizeof(tuya_uuid_and_key_find_file));
        snprintf(tuya_uuid_and_key_find_file, sizeof(tuya_uuid_and_key_find_file), "%s", xls_name);
        printf("find tuya XLS file: %s%s\n", TUYA_UUID_AND_KEY_XLS_PATH, tuya_uuid_and_key_find_file);
        return true;
    }

    printf("invalid XLS file count: %d\n", find_count);
    return false;
}

/**
 * @brief 涂鸦UUID/KEY初始化入口函数
 * @return true-已存在有效配置或SD卡有可用XLS文件（可正常使用），false-无有效配置且无可用XLS文件（需注册）
 */
bool tuya_key_and_uuid_init(void)
{
    if (tuya_uuid_and_key_exist_check())
    {
        return true;
    }
    return tuya_key_xls_exist_check();
}

/**
 * @brief 校验XLS文件表头格式是否合法（需为"uuid"和"key"）
 * @param sheet XLS工作表指针
 * @return true-表头格式合法，false-表头格式无效
 */
static bool tuya_xls_head_valid_check(const xlsWorkSheet *sheet)
{
    if (sheet == NULL || sheet->rows.lastcol < 1)
    {
        printf("XLS sheet is null or no column\n");
        return false;
    }

    struct st_row_data *row = &sheet->rows.row[0];
    unsigned char *uuid_header = row->cells.cell[0].str;
    if (uuid_header == NULL || strcmp((char *)uuid_header, "uuid") != 0)
    {
        printf("XLS first column header is not 'uuid'\n");
        return false;
    }

    unsigned char *key_header = row->cells.cell[1].str;
    if (key_header == NULL || strcmp((char *)key_header, "key") != 0)
    {
        printf("XLS second column header is not 'key'\n");
        return false;
    }

    return true;
}

/**
 * @brief 将UUID和KEY保存为本地配置文件
 * @param uuid 待保存的涂鸦UUID
 * @param key 待保存的涂鸦KEY
 * @param index 数据在XLS中的行索引（用于配置文件名）
 * @return true-保存成功，false-保存失败
 */
static bool tuya_conf_uuid_key_save(const unsigned char *uuid, const unsigned char *key, int index)
{
    if (uuid == NULL || key == NULL || strlen((char *)uuid) == 0 || strlen((char *)key) == 0)
    {
        printf("uuid or key is null/empty\n");
        return false;
    }

    struct tm tm = {0};
    char conf_path[256] = {0};

    // 从已找到的XLS文件名中解析日期
    if (sscanf(tuya_uuid_and_key_find_file, "%04d-%02d-%02d.xls", &tm.tm_year, &tm.tm_mon, &tm.tm_mday) != 3)
    {
        printf("parse XLS file name date failed\n");
        return false;
    }

    // 校验日期合法性
    if ((tm.tm_year < MIN_YEAR) || (tm.tm_year > MAX_YEAR) ||
        (tm.tm_mon <= 0) || (tm.tm_mon >= 13) ||
        (tm.tm_mday <= 0) || (tm.tm_mday >= 32))
    {
        printf("invalid date in XLS file name\n");
        return false;
    }

    // 检查配置目录，不存在则创建
    if (access(TUYA_UUID_AND_KEY_CONF_PATH, F_OK) != 0)
    {
        if (mkdir(TUYA_UUID_AND_KEY_CONF_PATH, 0755) != 0)
        {
            printf("create conf dir %s failed: %s\n", TUYA_UUID_AND_KEY_CONF_PATH, strerror(errno));
            return false;
        }
    }

    // 构造配置文件名和路径
    memset(tuya_uuid_and_key_find_file, 0, sizeof(tuya_uuid_and_key_find_file));
    if (snprintf(tuya_uuid_and_key_find_file, sizeof(tuya_uuid_and_key_find_file),
                 "%04d-%02d-%02d-%05d.conf", tm.tm_year, tm.tm_mon, tm.tm_mday, index) >= sizeof(tuya_uuid_and_key_find_file))
    {
        printf("conf file name too long\n");
        return false;
    }

    if (snprintf(conf_path, sizeof(conf_path), "%s%s", TUYA_UUID_AND_KEY_CONF_PATH, tuya_uuid_and_key_find_file) >= sizeof(conf_path))
    {
        printf("conf file path too long\n");
        return false;
    }

    // 打开/创建配置文件（清空原有内容）
    int fd = open(conf_path, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd < 0)
    {
        printf("open conf file %s failed: %s\n", conf_path, strerror(errno));
        return false;
    }

    // 写入UUID和KEY
    char buffer[MAX_BUFFER_LEN] = {0};
    int len = snprintf(buffer, sizeof(buffer), "%s %s", uuid, key);
    if (len < 0 || len >= sizeof(buffer))
    {
        printf("conf data buffer overflow\n");
        close(fd);
        return false;
    }

    if (write(fd, buffer, len) != len)
    {
        printf("write conf file %s failed: %s\n", conf_path, strerror(errno));
        close(fd);
        return false;
    }

    close(fd);
    return true;
}

/**
 * @brief 从XLS工作表指定行读取UUID和KEY
 * @param sheet XLS工作表指针
 * @param index 要读取的行索引（从1开始，跳过表头）
 * @return true-读取并保存成功，false-读取失败
 */
static bool tuya_xls_uuid_key_read(const xlsWorkSheet *sheet, int index)
{
    if (sheet == NULL || index < 1)
    {
        return false;
    }

    struct st_row_data *row = &sheet->rows.row[index];
    unsigned char *uuid = row->cells.cell[0].str;
    if (uuid == NULL || strlen((char *)uuid) == 0)
    {
        printf("read uuid null/empty at row %d\n", index);
        return false;
    }

    unsigned char *key = row->cells.cell[1].str;
    if (key == NULL || strlen((char *)key) == 0)
    {
        printf("read key null/empty at row %d\n", index);
        return false;
    }

    if (!tuya_conf_uuid_key_save(uuid, key, index))
    {
        printf("save uuid/key failed at row %d\n", index);
        return false;
    }

    return true;
}

/**
 * @brief 从SD卡XLS文件注册指定索引的涂鸦UUID和KEY（生成本地配置文件）
 * @param index 要注册的XLS行索引（从1开始，对应表头下的行）
 * @return true-注册成功，false-注册失败（已注册/文件打开失败/格式错误等）
 */
bool tuya_key_and_key_xls_register(int index)
{
    if (is_tuya_uuid_and_key_register)
    {
        printf("tuya uuid/key already registered\n");
        return false;
    }

    char xls_path[256] = {0};
    // 构造XLS文件完整路径
    if (snprintf(xls_path, sizeof(xls_path), "%s%s", TUYA_UUID_AND_KEY_XLS_PATH, tuya_uuid_and_key_find_file) >= sizeof(xls_path))
    {
        printf("XLS file path too long\n");
        return false;
    }

    // 打开XLS文件
    xlsWorkBook *pxls_xls = xls_open(xls_path, "UTF-8");
    if (pxls_xls == NULL)
    {
        printf("open XLS file %s failed\n", xls_path);
        return false;
    }

    // 获取第一个工作表并解析
    xlsWorkSheet *pxls_sheet = xls_getWorkSheet(pxls_xls, 0);
    if (pxls_sheet == NULL)
    {
        printf("get XLS sheet failed\n");
        xls_close_WB(pxls_xls);
        return false;
    }
    xls_parseWorkSheet(pxls_sheet);

    // 校验索引合法性（最大行索引为工作表最后一行）
    if (index > pxls_sheet->rows.lastrow || index < 1)
    {
        printf("index %d invalid, max row is %d\n", index, pxls_sheet->rows.lastrow);
        xls_close_WS(pxls_sheet);
        xls_close_WB(pxls_xls);
        return false;
    }

    // 校验XLS表头格式
    if (!tuya_xls_head_valid_check(pxls_sheet))
    {
        xls_close_WS(pxls_sheet);
        xls_close_WB(pxls_xls);
        return false;
    }

    // 读取并保存指定索引的UUID和KEY
    bool result = tuya_xls_uuid_key_read(pxls_sheet, index);
    if (result)
    {
        is_tuya_uuid_and_key_register = true;
        printf("tuya uuid/key register success\n");
    }
    else
    {
        printf("tuya uuid/key register failed\n");
    }

    // 释放XLS资源
    xls_close_WS(pxls_sheet);
    xls_close_WB(pxls_xls);
    return result;
}

/**
 * @brief 从本地配置文件读取已注册的涂鸦UUID和KEY
 * @param uuid 存储读取到的UUID的缓冲区（需提前分配足够空间）
 * @param key 存储读取到的KEY的缓冲区（需提前分配足够空间）
 * @return true-读取成功，false-未注册/缓冲区为空/文件读取失败
 */
bool tuya_uuid_and_key_read(unsigned char *uuid, unsigned char *key)
{
    if (uuid == NULL || key == NULL || !is_tuya_uuid_and_key_register)
    {
        return false;
    }

    char conf_path[256] = {0};
    // 构造配置文件完整路径
    if (snprintf(conf_path, sizeof(conf_path), "%s%s", TUYA_UUID_AND_KEY_CONF_PATH, tuya_uuid_and_key_find_file) >= sizeof(conf_path))
    {
        printf("conf file path too long\n");
        return false;
    }

    // 打开配置文件
    int fd = open(conf_path, O_RDONLY);
    if (fd < 0)
    {
        printf("open conf file %s failed: %s\n", conf_path, strerror(errno));
        return false;
    }

    // 读取文件内容
    char buffer[MAX_BUFFER_LEN] = {0};
    ssize_t read_len = read(fd, buffer, sizeof(buffer) - 1);
    if (read_len < 0)
    {
        printf("read conf file %s failed: %s\n", conf_path, strerror(errno));
        close(fd);
        return false;
    }
    buffer[read_len] = '\0'; // 确保字符串结束符

    close(fd);

    // 解析UUID和KEY（空格分隔）
    if (sscanf(buffer, "%s %s", uuid, key) != 2)
    {
        printf("parse uuid/key from conf file failed\n");
        return false;
    }

    return true;
}

/**
 * @brief 获取已注册的涂鸦序列号（配置文件名前缀，不含.conf后缀）
 * @param serial 存储序列号的缓冲区（需提前分配足够空间）
 * @param serial_len 缓冲区最大长度
 * @return true-获取成功，false-未注册/缓冲区无效/解析失败
 */
bool tuya_serial_number_get(char *serial, int serial_len)
{
    if (!is_tuya_uuid_and_key_register || serial == NULL || serial_len <= 0)
    {
        return false;
    }

    // 拷贝配置文件名并去除.conf后缀
    strncpy(serial, tuya_uuid_and_key_find_file, serial_len - 1);
    serial[serial_len - 1] = '\0';

    char *dot_ptr = strchr(serial, '.');
    if (dot_ptr == NULL)
    {
        printf("serial number parse failed (no .conf suffix)\n");
        return false;
    }
    *dot_ptr = '\0';

    return true;
}