#include "language.h"
#include "string.h"
#include "language_xls.h"
#include "stdio.h"
#include <stdbool.h>

// ========================= 配置与宏定义 =========================
// 默认字符串（全局统一，避免多处定义）
#define LANG_DEFAULT_STR "text"
// 输出文件路径（宏定义集中管理，便于修改）
#define LANGUAGE_OUTPUT_FILE "./language.txt"

// ========================= 全局状态管理 =========================
// 当前语言ID（静态全局，通过API访问）
static LANGUAGE_ID s_cur_lang_id = LANGUAGE_ID_ENGLISH;

// ========================= 私有工具函数 =========================
/**
 * @brief 校验XLS语言模块是否可用
 * @return true：可用，false：不可用
 */
static bool lang_xls_is_valid(void)
{
    return (lang_xls_file_state_get() &&
            lang_xls_language_num_get() > 0 &&
            lang_xls_str_num_get() == STR_TOTAL + 2);
}

/**
 * @brief 安全获取XLS语言字符串（含空值 fallback）
 * @param str_id：字符串ID
 * @param lang_id：语言ID
 * @return 字符串指针（永不返回NULL）
 */
static const char *lang_xls_get_safe(int str_id, LANGUAGE_ID lang_id)
{
    // 校验参数合法性
    if (str_id < 0 || str_id >= STR_TOTAL)
    {
        printf("[LANG] invalid str_id: %d (max: %d)\n", str_id, STR_TOTAL - 1);
        return LANG_DEFAULT_STR;
    }

    int lang_num = lang_xls_language_num_get();
    if (lang_id < 0 || lang_id >= lang_num)
    {
        printf("[LANG] invalid lang_id: %d (max: %d)\n", lang_id, lang_num - 1);
        return LANG_DEFAULT_STR;
    }

    // 获取XLS字符串，空值则 fallback 到英文
    const char *xls_str = lang_xls_str_get(str_id, lang_id);
    if (xls_str && (strcmp(xls_str, "NULL") != 0) && (strlen(xls_str) > 0))
    {
        return xls_str;
    }

    // 英文也为空时，返回默认字符串
    const char *en_str = lang_xls_str_get(str_id, LANGUAGE_ID_ENGLISH);
    return (en_str && strcmp(en_str, "NULL") != 0 && strlen(en_str) > 0) ? en_str : LANG_DEFAULT_STR;
}

// ========================= 公开API函数 =========================
/**
 * @brief 设置当前语言ID
 * @param id：目标语言ID（LANGUAGE_ID枚举）
 */
void language_id_set(LANGUAGE_ID id)
{
    // 校验语言ID合法性
    if (id >= 0 && id < LANGUAGE_ID_TOTAL)
    {
        s_cur_lang_id = id;
        printf("[LANG] set current language: %d\n", id);
    }
    else
    {
        printf("[LANG] invalid language ID: %d\n", id);
    }
}

/**
 * @brief 获取当前语言ID
 * @return 当前语言ID（默认：LANGUAGE_ID_ENGLISH）
 */
LANGUAGE_ID language_id_get(void)
{
    return s_cur_lang_id;
}

/**
 * @brief 获取当前语言的字符串（核心API）
 * @param lang_str_id：字符串ID（对应XLS文件中的行号）
 * @return 字符串指针（永不返回NULL，失败返回默认值）
 */
const char *lang_str_get(int lang_str_id)
{
    // XLS模块可用则优先从XLS获取，否则返回默认字符串
    if (lang_xls_is_valid())
    {
        return lang_xls_get_safe(lang_str_id, s_cur_lang_id);
    }
    else
    {
        printf("[LANG] XLS language module not available, use default str\n");
        return LANG_DEFAULT_STR;
    }
}

/**
 * @brief 按指定语言获取字符串
 * @param index：字符串ID
 * @param language：指定语言ID
 * @return 字符串指针（永不返回NULL，失败返回默认值）
 */
const char *lang_str_get_by_language(xls_str_id_t index, LANGUAGE_ID language)
{
    if (lang_xls_is_valid())
    {
        return lang_xls_get_safe(index, language);
    }
    else
    {
        printf("[LANG] XLS language module not available, use default str\n");
        return LANG_DEFAULT_STR;
    }
}

/**
 * @brief 获取支持的语言总数
 * @return 语言总数（XLS可用返回实际数量，否则返回1）
 */
int language_total_get(void)
{
    if (lang_xls_is_valid())
    {
        return lang_xls_language_num_get();
    }
    else
    {
        return 1; // 默认仅支持英文
    }
}

/**
 * @brief 输出所有语言文本到文件（用于校验文案）
 * @note 宏控制编译，默认关闭
 */
void write_language_string_to_xls(void)
{
#if 1 // 需要启用时改为1
    FILE *fp = fopen(LANGUAGE_OUTPUT_FILE, "w");
    if (!fp)
    {
        printf("[LANG] open output file failed: %s\n", LANGUAGE_OUTPUT_FILE);
        return;
    }

    // 遍历所有字符串ID，写入文件
    for (int i = 0; i < STR_TOTAL; i++)
    {
        const char *str = lang_str_get(i);
        fprintf(fp, "%s\n", str ? str : LANG_DEFAULT_STR);
    }

    fclose(fp);
    printf("[LANG] language strings written to: %s\n", LANGUAGE_OUTPUT_FILE);
#endif
}
