#include "xls/xls.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdbool.h>
#include "language_xls.h"
#include "language.h"

// ========================= 配置与宏定义 =========================
// 日志宏：支持颜色输出，保留原格式但优化可读性
#define LAN_LOG(level, color, format, ...)                   \
    printf("\e[0;1;%sm[LANGUAGE][%s:%04u]\t" format "\e[0m", \
           color, __FUNCTION__, __LINE__, ##__VA_ARGS__)

#define LAN_ERR(format, ...) LAN_LOG(ERROR, "31", format "\n", ##__VA_ARGS__)
#define LAN_WARN(format, ...) LAN_LOG(WARN, "33", format "\n", ##__VA_ARGS__)
#define LAN_INFO(format, ...) LAN_LOG(INFO, "32", format "\n", ##__VA_ARGS__)

// 错误码定义：替代直接返回NULL，便于问题定位
typedef enum
{
    LAN_SUCCESS = 0,
    LAN_ERR_FILE_NOT_EXIST = -1,
    LAN_ERR_XLS_OPEN = -2,
    LAN_ERR_XLS_SHEET = -3,
    LAN_ERR_MEM_ALLOC = -4,
    LAN_ERR_INVALID_PARAM = -5,
    LAN_ERR_XLS_PARSE = -6,
    LAN_ERR_INIT_FAILED = -7
} LanErrCode;

// ========================= 私有数据封装 =========================
// 核心上下文：封装所有状态，避免全局变量
typedef struct
{
    char ***buffer;     // 存储解析后的语言数据（行×列）
    int row_total;      // 有效总行数（含表头）
    int col_total;      // 有效总列数（语言种类）
    int null_str_num;   // 空字符串数量
    bool is_inited;     // 初始化成功标志
    bool is_file_exist; // XLS文件存在标志
} LanXlsCtx;

// 静态上下文：仅当前文件可见，避免全局暴露
static LanXlsCtx s_lan_ctx = {
    .buffer = NULL,
    .row_total = 0,
    .col_total = 0,
    .null_str_num = 0,
    .is_inited = false,
    .is_file_exist = false};

// ========================= 私有工具函数 =========================
/**
 * @brief 安全字符串替换
 * @param source 源字符串（必须是可修改的缓冲区，且长度足够）
 * @param find 待替换子串
 * @param replace 替换子串
 */
static void str_replace_safe(char *source, const char *find, const char *replace)
{
    if (!source || !find || !replace)
        return;

    const int find_len = strlen(find);
    const int replace_len = strlen(replace);
    if (find_len == 0)
        return;

    // 计算替换后的总长度，避免溢出（原函数未做此检查）
    int source_len = strlen(source);
    int replace_count = 0;
    char *ptr = source;

    // 统计替换次数，计算所需缓冲区大小
    while ((ptr = strstr(ptr, find)) != NULL)
    {
        replace_count++;
        ptr += find_len;
    }

    // 重新分配足够大的缓冲区
    int new_len = source_len + replace_count * (replace_len - find_len) + 1;
    char *new_str = (char *)malloc(new_len);
    if (!new_str)
    {
        LAN_WARN("str_replace malloc failed");
        return;
    }

    // 执行替换
    char *dst = new_str;
    ptr = source;
    while (*ptr != '\0')
    {
        if (strncmp(ptr, find, find_len) == 0)
        {
            strncpy(dst, replace, replace_len);
            dst += replace_len;
            ptr += find_len;
        }
        else
        {
            *dst++ = *ptr++;
        }
    }
    *dst = '\0';

    // 拷贝回原字符串（确保原缓冲区足够大，否则截断）
    strncpy(source, new_str, new_len - 1);
    source[new_len - 1] = '\0';
    free(new_str);
}

/**
 * @brief 释放三维缓冲区内存（避免内存泄漏）
 */
static void lan_buffer_free(void)
{
    if (s_lan_ctx.buffer)
    {
        for (int i = 0; i < s_lan_ctx.row_total; i++)
        {
            if (s_lan_ctx.buffer[i])
            {
                for (int j = 0; j < s_lan_ctx.col_total; j++)
                {
                    free(s_lan_ctx.buffer[i][j]);
                }
                free(s_lan_ctx.buffer[i]);
            }
        }
        free(s_lan_ctx.buffer);
        s_lan_ctx.buffer = NULL;
    }
    // 重置上下文状态
    s_lan_ctx.row_total = 0;
    s_lan_ctx.col_total = 0;
    s_lan_ctx.null_str_num = 0;
}

/**
 * @brief 申请三维缓冲区
 * @return LAN_SUCCESS 成功，否则错误码
 */
static LanErrCode lan_buffer_alloc(int row_total, int col_total)
{
    if (row_total <= 0 || col_total <= 0)
    {
        LAN_ERR("invalid buffer size: row=%d, col=%d", row_total, col_total);
        return LAN_ERR_INVALID_PARAM;
    }

    // 先释放旧内存，避免重复申请导致泄漏
    lan_buffer_free();

    // 申请行指针
    s_lan_ctx.buffer = (char ***)malloc(sizeof(char **) * row_total);
    if (!s_lan_ctx.buffer)
    {
        LAN_ERR("malloc buffer rows failed: %s", strerror(errno));
        return LAN_ERR_MEM_ALLOC;
    }

    // 申请列指针和字符串缓冲区
    for (int i = 0; i < row_total; i++)
    {
        s_lan_ctx.buffer[i] = (char **)malloc(sizeof(char *) * col_total);
        if (!s_lan_ctx.buffer[i])
        {
            LAN_ERR("malloc buffer cols failed: row=%d", i);
            lan_buffer_free(); // 失败时释放已申请内存
            return LAN_ERR_MEM_ALLOC;
        }

        // 初始化列缓冲区为NULL，便于后续释放
        memset(s_lan_ctx.buffer[i], 0, sizeof(char *) * col_total);
    }

    s_lan_ctx.row_total = row_total;
    s_lan_ctx.col_total = col_total;
    return LAN_SUCCESS;
}

/**
 * @brief 解析XLS工作表数据到缓冲区
 */
static LanErrCode lan_xls_parse_data(xlsWorkSheet *ws)
{
    if (!ws)
        return LAN_ERR_INVALID_PARAM;

    // 获取XLS实际行列数
    int row_total = ws->rows.lastrow + 1;
    int col_total = ws->rows.lastcol;
    LAN_INFO("xls sheet info: rows=%d, cols=%d", row_total, col_total);

    // 申请缓冲区
    LanErrCode ret = lan_buffer_alloc(row_total, col_total);
    if (ret != LAN_SUCCESS)
    {
        return ret;
    }

    // 解析每个单元格数据
    for (int i = 0; i < row_total; i++)
    {
        xlsRow *row = &ws->rows.row[i];
        for (int j = 0; j < col_total; j++)
        {
            xlsCell *cell = &row->cells.cell[j];
            char *cell_str = (char *)cell->str;

            if (!cell_str || strlen(cell_str) == 0)
            {
                // 空字符串处理：分配"NULL"
                s_lan_ctx.buffer[i][j] = (char *)malloc(5);
                if (!s_lan_ctx.buffer[i][j])
                {
                    LAN_ERR("malloc null string failed: row=%d, col=%d", i, j);
                    lan_buffer_free();
                    return LAN_ERR_MEM_ALLOC;
                }
                strcpy(s_lan_ctx.buffer[i][j], "NULL");
                s_lan_ctx.null_str_num++;
                LAN_WARN("empty cell: row=%d, col=%d", i, j);
            }
            else
            {
                // 非空字符串：分配内存并替换换行符
                int str_len = strlen(cell_str);
                s_lan_ctx.buffer[i][j] = (char *)malloc(str_len + 1);
                if (!s_lan_ctx.buffer[i][j])
                {
                    LAN_ERR("malloc cell string failed: row=%d, col=%d", i, j);
                    lan_buffer_free();
                    return LAN_ERR_MEM_ALLOC;
                }
                strcpy(s_lan_ctx.buffer[i][j], cell_str);
                str_replace_safe(s_lan_ctx.buffer[i][j], "\\n", "\n"); // 替换转义换行
            }
        }
    }

    return LAN_SUCCESS;
}

// ========================= 公开API函数 =========================
/**
 * @brief 初始化XLS语言文件（内部调用，不暴露给外部）
 * @param sheet_num 工作表序号（从0开始）
 * @return 错误码
 */
static LanErrCode lang_xls_init_internal(int sheet_num)
{
    // 检查文件是否存在
    if (access(XLS_TMP_PATH, F_OK) != 0)
    {
        LAN_ERR("xls file not exist: %s", XLS_TMP_PATH);
        s_lan_ctx.is_file_exist = false;
        return LAN_ERR_FILE_NOT_EXIST;
    }
    s_lan_ctx.is_file_exist = true;
    LAN_INFO("xls file exist: %s", XLS_TMP_PATH);

    // 打开XLS文件
    xlsWorkBook *wb = xls_open(XLS_TMP_PATH, CODE);
    if (!wb)
    {
        LAN_ERR("open xls file failed: %s", XLS_TMP_PATH);
        return LAN_ERR_XLS_OPEN;
    }

    // 获取指定工作表
    xlsWorkSheet *ws = xls_getWorkSheet(wb, sheet_num);
    if (!ws)
    {
        LAN_ERR("get sheet failed: sheet_num=%d", sheet_num);
        xls_close_WB(wb);
        return LAN_ERR_XLS_SHEET;
    }

    // 解析工作表
    xls_parseWorkSheet(ws);

    // 解析数据到缓冲区
    LanErrCode ret = lan_xls_parse_data(ws);

    // 关闭XLS资源（无论成功与否都要关闭）
    xls_close_WS(ws);
    xls_close_WB(wb);

    if (ret == LAN_SUCCESS)
    {
        s_lan_ctx.is_inited = true;
        LAN_INFO("xls init success: rows=%d, cols=%d, null_num=%d",
                 s_lan_ctx.row_total, s_lan_ctx.col_total, s_lan_ctx.null_str_num);
    }
    else
    {
        s_lan_ctx.is_inited = false;
        LAN_ERR("xls init failed: err_code=%d", ret);
    }

    return ret;
}

/**
 * @brief 初始化语言模块（对外公开API）
 * @return true 成功，false 失败
 */
bool init_language_xls_info(void)
{
    // 已初始化则直接返回成功
    if (s_lan_ctx.is_inited)
    {
        LAN_INFO("language already inited");
        return true;
    }

    // 内部初始化（默认使用第0个工作表）
    LanErrCode ret = lang_xls_init_internal(0);
    if (ret != LAN_SUCCESS)
    {
        LAN_ERR("init language failed: err_code=%d", ret);
        return false;
    }

    // 校验行数列数
    // if (s_lan_ctx.row_total != (STR_TOTAL))
    // {
    //     LAN_ERR("row count mismatch: xls=%d, config=%d",
    //             s_lan_ctx.row_total, STR_TOTAL);
    //     lan_buffer_free();
    //     s_lan_ctx.is_inited = false;
    //     return false;
    // }

    if (s_lan_ctx.col_total <= 0)
    {
        LAN_ERR("invalid column count: %d", s_lan_ctx.col_total);
        lan_buffer_free();
        s_lan_ctx.is_inited = false;
        return false;
    }

    // 空字符串警告（不影响初始化成功）
    if (s_lan_ctx.null_str_num > 0)
    {
        LAN_WARN("exist %d empty cells in xls", s_lan_ctx.null_str_num);
    }

    return true;
}

/**
 * @brief 获取XLS文件是否存在
 * @return true 存在，false 不存在
 */
bool lang_xls_file_state_get(void)
{
    return s_lan_ctx.is_file_exist;
}

/**
 * @brief 获取空字符串数量
 * @return 空字符串个数
 */
int lang_xls_null_str_num_get(void)
{
    return s_lan_ctx.null_str_num;
}

/**
 * @brief 获取语言种类数（列数）
 * @return 语言数量，未初始化返回0
 */
int lang_xls_language_num_get(void)
{
    return s_lan_ctx.is_inited ? s_lan_ctx.col_total : 0;
}

/**
 * @brief 获取字符串总数
 * @return 字符串数量，未初始化返回0
 */
int lang_xls_str_num_get(void)
{
    return s_lan_ctx.is_inited ? (s_lan_ctx.row_total - 1) : 0;
}

/**
 * @brief 获取指定行列的语言字符串
 * @param str_num 字符串序号（行号，从0开始）
 * @param lang_type 语言类型（列号，从0开始）
 * @return 字符串指针，错误返回"language error"
 */
char *lang_xls_str_get(int str_num, int lang_type)
{
    // 检查初始化状态
    if (!s_lan_ctx.is_inited)
    {
        LAN_ERR("not inited");
        return "language error";
    }

    // 检查参数合法性（str_num 对应数据行，需跳过表头行，所以 str_num < row_total-1）
    if (str_num < 0 || str_num >= (s_lan_ctx.row_total - 1) ||
        lang_type < 0 || lang_type >= s_lan_ctx.col_total)
    {
        LAN_ERR("invalid param: str_num=%d, lang_type=%d (max: str=%d, lang=%d)",
                str_num, lang_type, s_lan_ctx.row_total - 2, s_lan_ctx.col_total - 1);
        return "language error";
    }

    // 返回数据
    return s_lan_ctx.buffer[str_num][lang_type];
}

/**
 * @brief 销毁语言模块（释放内存，对外公开API）
 * @note 程序退出时必须调用，避免内存泄漏
 */
void deinit_language_xls_info(void)
{
    if (s_lan_ctx.is_inited || s_lan_ctx.buffer)
    {
        lan_buffer_free();
        s_lan_ctx.is_inited = false;
        s_lan_ctx.is_file_exist = false;
        LAN_INFO("language module deinited");
    }
}

/**
 * @brief 检查初始化是否成功
 * @return true 成功，false 失败
 */
bool is_lan_xls_init_ok_get(void)
{
    return s_lan_ctx.is_inited;
}