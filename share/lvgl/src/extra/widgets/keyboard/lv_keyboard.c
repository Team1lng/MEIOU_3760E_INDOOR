

/**
 * @file lv_keyboard.c
 *
 */

/*********************
 *		INCLUDES
 *********************/
#include "lv_keyboard.h"
#if LV_USE_KEYBOARD
#include "../../../widgets/lv_textarea.h"
#include "../../../misc/lv_assert.h"

#include <stdlib.h>

/*********************
 *		DEFINES
 *********************/
#define MY_CLASS &lv_keyboard_class
#define LV_KB_BTN(width) LV_BTNMATRIX_CTRL_POPOVER | width
#define LV_KB_PAD_BTN(x, y) \
    {                       \
        x, y}

/**********************
 *		TYPEDEFS
 **********************/

/**********************
 *	STATIC PROTOTYPES
 **********************/
static void lv_keyboard_constructor(const lv_obj_class_t *class_p, lv_obj_t *obj);

static void lv_keyboard_update_map(lv_obj_t *obj);

static void lv_keyboard_update_ctrl_map(lv_obj_t *obj);

#if SAT_LV_ADD_CUSTOM
static void lv_keyboard_update_pad_map(lv_obj_t *obj);

#endif

/**********************
 *	STATIC VARIABLES
 **********************/
const lv_obj_class_t lv_keyboard_class =
    {
        .constructor_cb = lv_keyboard_constructor,
        .width_def = LV_PCT(100),
        .height_def = LV_PCT(50),
        .instance_size = sizeof(lv_keyboard_t),
        .editable = 1,
        .base_class = &lv_btnmatrix_class};

#if SAT_LV_ADD_CUSTOM
static const void *p_deafult_kb_img_map[3][44];

#endif

static const char *const default_kb_map_lc[] =
    {
        "1#", "q", "w", "e", "f", "r", "t", "y", "u", "i", "o", "p", " ", "\n",
        "ABC", "a", "s", "d", "f", "g", "h", "j", "k", "l", " ", "\n",
        "_", "z", "x", "c", "v", "b", "n", "m", ".", ",", ":", "\n",
        " ", " ", " ", " ", " ", ""};

static const lv_btnmatrix_ctrl_t default_kb_ctrl_lc_map[] =
    {
        LV_KB_BTN(2),
        LV_KB_BTN(2),
        LV_KB_BTN(2),
        LV_KB_BTN(2),
        LV_KB_BTN(2),
        LV_KB_BTN(2),
        LV_KB_BTN(2),
        LV_KB_BTN(2),
        LV_KB_BTN(2),
        LV_KB_BTN(2),
        LV_KB_BTN(2),
        LV_KB_BTN(2),
        LV_KB_BTN(6),
        LV_KB_BTN(4),
        LV_KB_BTN(2),
        LV_KB_BTN(2),
        LV_KB_BTN(2),
        LV_KB_BTN(2),
        LV_KB_BTN(2),
        LV_KB_BTN(2),
        LV_KB_BTN(2),
        LV_KB_BTN(2),
        LV_KB_BTN(2),
        LV_KB_BTN(5),
        LV_KB_BTN(2),
        LV_KB_BTN(2),
        LV_KB_BTN(2),
        LV_KB_BTN(2),
        LV_KB_BTN(2),
        LV_KB_BTN(2),
        LV_KB_BTN(2),
        LV_KB_BTN(2),
        LV_KB_BTN(2),
        LV_KB_BTN(2),
        LV_KB_BTN(2),
        LV_KB_BTN(2),
        LV_KB_BTN(2),
        LV_KB_BTN(6),
        LV_KB_BTN(2),
        LV_KB_BTN(2),
};

#if SAT_LV_ADD_CUSTOM
static const lv_btnmatrix_pad_info_t default_kb_pad_lc_map[] =
    {
        LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0),
        LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0),
        LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0),
        LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0)};

#endif

static const char *const default_kb_map_uc[] =
    {
        "1#", "Q", "W", "E", "F", "R", "T", "Y", "U", "I", "O", "P", " ", "\n",
        "abc", "A", "S", "D", "F", "G", "H", "J", "K", "L", " ", "\n",
        "_", "Z", "X", "C", "V", "B", "N", "M", ".", ",", ":", "\n",
        " ", " ", " ", " ", " ", ""};

static const lv_btnmatrix_ctrl_t default_kb_ctrl_uc_map[] =
    {
        LV_KB_BTN(2),
        LV_KB_BTN(2),
        LV_KB_BTN(2),
        LV_KB_BTN(2),
        LV_KB_BTN(2),
        LV_KB_BTN(2),
        LV_KB_BTN(2),
        LV_KB_BTN(2),
        LV_KB_BTN(2),
        LV_KB_BTN(2),
        LV_KB_BTN(2),
        LV_KB_BTN(2),
        LV_KB_BTN(6),
        LV_KB_BTN(4),
        LV_KB_BTN(2),
        LV_KB_BTN(2),
        LV_KB_BTN(2),
        LV_KB_BTN(2),
        LV_KB_BTN(2),
        LV_KB_BTN(2),
        LV_KB_BTN(2),
        LV_KB_BTN(2),
        LV_KB_BTN(2),
        LV_KB_BTN(5),
        LV_KB_BTN(2),
        LV_KB_BTN(2),
        LV_KB_BTN(2),
        LV_KB_BTN(2),
        LV_KB_BTN(2),
        LV_KB_BTN(2),
        LV_KB_BTN(2),
        LV_KB_BTN(2),
        LV_KB_BTN(2),
        LV_KB_BTN(2),
        LV_KB_BTN(2),
        LV_KB_BTN(2),
        LV_KB_BTN(2),
        LV_KB_BTN(6),
        LV_KB_BTN(2),
        LV_KB_BTN(2),
};

#if SAT_LV_ADD_CUSTOM
static const lv_btnmatrix_pad_info_t default_kb_pad_uc_map[] =
    {
        LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0),
        LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0),
        LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0),
        LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0)};

#endif

static const char *const default_kb_map_spec[] =
    {
        "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", " ", "\n",
        "abc", "+", "-", "/", "*", "=", "%", "!", "?", "#", "<", ">", "\n",
        "\\", "@", "$", "(", ")", "{", "}", "[", "]", ";", "\"", "'", "\n",
        " ", " ", " ", " ", " ", ""};

static const lv_btnmatrix_ctrl_t default_kb_ctrl_spec_map[] =
    {
        LV_KB_BTN(2), LV_KB_BTN(2), LV_KB_BTN(2), LV_KB_BTN(2), LV_KB_BTN(2), LV_KB_BTN(2), LV_KB_BTN(2), LV_KB_BTN(2), LV_KB_BTN(2), LV_KB_BTN(2), LV_KB_BTN(5),
        LV_KB_BTN(2), LV_KB_BTN(2), LV_KB_BTN(2), LV_KB_BTN(2), LV_KB_BTN(2), LV_KB_BTN(2), LV_KB_BTN(2), LV_KB_BTN(2), LV_KB_BTN(2), LV_KB_BTN(2), LV_KB_BTN(2), LV_KB_BTN(2),
        LV_KB_BTN(2), LV_KB_BTN(2), LV_KB_BTN(2), LV_KB_BTN(2), LV_KB_BTN(2), LV_KB_BTN(2), LV_KB_BTN(2), LV_KB_BTN(2), LV_KB_BTN(2), LV_KB_BTN(2), LV_KB_BTN(2), LV_KB_BTN(2),
        LV_KB_BTN(1), LV_KB_BTN(1), LV_KB_BTN(3), LV_KB_BTN(1), LV_KB_BTN(1)};

#if SAT_LV_ADD_CUSTOM
static const lv_btnmatrix_pad_info_t default_kb_pad_spec_map[] =
    {
        LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0),
        LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0),
        LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0),
        LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0), LV_KB_PAD_BTN(0, 0)};

#endif

static const char *const default_kb_map_num[] =
    {
        "1", "2", "3", LV_SYMBOL_KEYBOARD, "\n",
        "4", "5", "6", LV_SYMBOL_OK, "\n",
        "7", "8", "9", LV_SYMBOL_BACKSPACE, "\n",
        "+/-", "0", ".", LV_SYMBOL_LEFT, LV_SYMBOL_RIGHT, ""};

static const lv_btnmatrix_ctrl_t default_kb_ctrl_num_map[] =
    {
        1, 1, 1, LV_KEYBOARD_CTRL_BTN_FLAGS | 2,
        1, 1, 1, LV_KEYBOARD_CTRL_BTN_FLAGS | 2,
        1, 1, 1, 2,
        1, 1, 1, 1, 1};

static const char **kb_map[9] =
    {
        (const char **)default_kb_map_lc,
        (const char **)default_kb_map_uc,
        (const char **)default_kb_map_spec,
        (const char **)default_kb_map_num,
        (const char **)default_kb_map_lc,
        (const char **)default_kb_map_lc,
        (const char **)default_kb_map_lc,
        (const char **)default_kb_map_lc,
        (const char **)NULL,
};

static const lv_btnmatrix_ctrl_t *kb_ctrl[9] =
    {
        default_kb_ctrl_lc_map,
        default_kb_ctrl_uc_map,
        default_kb_ctrl_spec_map,
        default_kb_ctrl_num_map,
        default_kb_ctrl_lc_map,
        default_kb_ctrl_lc_map,
        default_kb_ctrl_lc_map,
        default_kb_ctrl_lc_map,
        NULL,
};

#if SAT_LV_ADD_CUSTOM
static const lv_btnmatrix_pad_info_t *kb_pad_ctrl[] =
    {
        default_kb_pad_lc_map,
        default_kb_pad_uc_map,
        default_kb_pad_spec_map,
        NULL,
};

#endif

/**********************
 *		MACROS
 **********************/

/**********************
 *	 GLOBAL FUNCTIONS
 **********************/

/**
 * Create a Keyboard object
 * @param parent pointer to an object, it will be the parent of the new keyboard
 * @return pointer to the created keyboard
 */
lv_obj_t *lv_keyboard_create(lv_obj_t *parent)
{
    LV_LOG_INFO("begin");
    lv_obj_t *obj = lv_obj_class_create_obj(&lv_keyboard_class, parent);

    lv_obj_class_init_obj(obj);
    return obj;
}

/*=====================
 * Setter functions
 *====================*/

/**
 * Assign a Text Area to the Keyboard. The pressed characters will be put there.
 * @param kb pointer to a Keyboard object
 * @param ta pointer to a Text Area object to write there
 */
void lv_keyboard_set_textarea(lv_obj_t *obj, lv_obj_t *ta)
{
    if (ta)
    {
        LV_ASSERT_OBJ(ta, &lv_textarea_class);
    }

    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_keyboard_t *keyboard = (lv_keyboard_t *)obj;

    /*Hide the cursor of the old Text area if cursor management is enabled*/
    if (keyboard->ta)
    {
        lv_obj_clear_state(obj, LV_STATE_FOCUSED);
    }

    keyboard->ta = ta;

    /*Show the cursor of the new Text area if cursor management is enabled*/
    if (keyboard->ta)
    {
        lv_obj_add_flag(obj, LV_STATE_FOCUSED);
    }
}

/**
 * Set a new a mode (text or number map)
 * @param kb pointer to a Keyboard object
 * @param mode the mode from 'lv_keyboard_mode_t'
 */
void lv_keyboard_set_mode(lv_obj_t *obj, lv_keyboard_mode_t mode)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_keyboard_t *keyboard = (lv_keyboard_t *)obj;

    if (keyboard->mode == mode)
        return;

    keyboard->mode = mode;
    lv_keyboard_update_map(obj);
}

/**
 * Show the button title in a popover when pressed.
 * @param kb pointer to a Keyboard object
 * @param en whether "popovers" mode is enabled
 */
void lv_keyboard_set_popovers(lv_obj_t *obj, bool en)
{
    lv_keyboard_t *keyboard = (lv_keyboard_t *)obj;

    if (keyboard->popovers == en)
    {
        return;
    }

    keyboard->popovers = en;
    lv_keyboard_update_ctrl_map(obj);
}

/**
 * Set a new map for the keyboard
 * @param kb pointer to a Keyboard object
 * @param mode keyboard map to alter 'lv_keyboard_mode_t'
 * @param map pointer to a string array to describe the map.
 *			  See 'lv_btnmatrix_set_map()' for more info.
 */
void lv_keyboard_set_map(lv_obj_t *obj, lv_keyboard_mode_t mode, const char *map[],
                         const lv_btnmatrix_ctrl_t ctrl_map[])
{
    kb_map[mode] = map;
    kb_ctrl[mode] = ctrl_map;
    lv_keyboard_update_map(obj);
}

#if SAT_LV_ADD_CUSTOM

void lv_keyboard_set_pad_map(lv_obj_t *obj, lv_keyboard_mode_t mode, const lv_btnmatrix_pad_info_t ctrl_map[])
{
    kb_pad_ctrl[mode] = ctrl_map;
    lv_keyboard_update_pad_map(obj);
}

#endif

/*=====================
 * Getter functions
 *====================*/

/**
 * Assign a Text Area to the Keyboard. The pressed characters will be put there.
 * @param kb pointer to a Keyboard object
 * @return pointer to the assigned Text Area object
 */
lv_obj_t *lv_keyboard_get_textarea(const lv_obj_t *obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_keyboard_t *keyboard = (lv_keyboard_t *)obj;

    return keyboard->ta;
}

/**
 * Set a new a mode (text or number map)
 * @param kb pointer to a Keyboard object
 * @return the current mode from 'lv_keyboard_mode_t'
 */
lv_keyboard_mode_t lv_keyboard_get_mode(const lv_obj_t *obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_keyboard_t *keyboard = (lv_keyboard_t *)obj;

    return keyboard->mode;
}

/**
 * Tell whether "popovers" mode is enabled or not.
 * @param kb pointer to a Keyboard object
 * @return true: "popovers" mode is enabled; false: disabled
 */
bool lv_btnmatrix_get_popovers(const lv_obj_t *obj)
{
    lv_keyboard_t *keyboard = (lv_keyboard_t *)obj;

    return keyboard->popovers;
}

/*=====================
 * Other functions
 *====================*/

/**
 * Default keyboard event to add characters to the Text area and change the map.
 * If a custom `event_cb` is added to the keyboard this function can be called from it to handle the
 * button clicks
 * @param kb pointer to a keyboard
 * @param event the triggering event
 */
void lv_keyboard_def_event_cb(lv_event_t *e)
{
    lv_obj_t *obj = lv_event_get_target(e);

    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_keyboard_t *keyboard = (lv_keyboard_t *)obj;
    uint16_t btn_id = lv_btnmatrix_get_selected_btn(obj);
    if (btn_id == LV_BTNMATRIX_BTN_NONE)
        return;

    const char *txt = lv_btnmatrix_get_btn_text(obj, lv_btnmatrix_get_selected_btn(obj));

    if (txt == NULL)
        return;

    if (keyboard->mode != LV_KEYBOARD_MODE_SPECIAL && keyboard->mode != LV_KEYBOARD_MODE_TEXT_UPPER && keyboard->mode != LV_KEYBOARD_MODE_TEXT_LOWER)
        return;
    if (keyboard->mode == LV_KEYBOARD_MODE_SPECIAL && btn_id == 10)
    {
        lv_textarea_del_char(keyboard->ta);
    }
    else if (keyboard->mode == LV_KEYBOARD_MODE_TEXT_UPPER && btn_id == 12)
    {
        lv_textarea_del_char(keyboard->ta);
    }
    else if (keyboard->mode == LV_KEYBOARD_MODE_TEXT_LOWER && btn_id == 12)
    {
        lv_textarea_del_char(keyboard->ta);
    }
    else if (strcmp(txt, "ABC") == 0)
    {
        keyboard->mode = LV_KEYBOARD_MODE_TEXT_UPPER;
        lv_btnmatrix_set_map(obj, kb_map[keyboard->mode]);
        lv_keyboard_update_ctrl_map(obj);
        lv_keyboard_update_pad_map(obj);
    }
    else if (strcmp(txt, "1#") == 0)
    {
        keyboard->mode = LV_KEYBOARD_MODE_SPECIAL;
        lv_btnmatrix_set_map(obj, kb_map[keyboard->mode]);
        lv_keyboard_update_pad_map(obj);
        lv_keyboard_update_ctrl_map(obj);
    }
    else if (strcmp(txt, "abc") == 0)
    {
        keyboard->mode = LV_KEYBOARD_MODE_TEXT_LOWER;
        lv_btnmatrix_set_map(obj, kb_map[keyboard->mode]);
        lv_keyboard_update_ctrl_map(obj);
        lv_keyboard_update_pad_map(obj);
    }
    // // ========== 新增/修改 < > 按钮处理逻辑 ==========
    else if (btn_id == 36) // 左箭头：光标左移
    {
        if (keyboard->ta)
            lv_textarea_cursor_left(keyboard->ta);
    }
    else if (btn_id == 38) // 右箭头：光标右移
    {
        if (keyboard->ta)
            lv_textarea_cursor_right(keyboard->ta);
    }
    else if (btn_id == 39) // 确认，留给应用实现回调
    {
    }
    else if ((btn_id == 23) && (keyboard->mode == LV_KEYBOARD_MODE_TEXT_LOWER || keyboard->mode == LV_KEYBOARD_MODE_TEXT_UPPER))
    {
        lv_textarea_add_char(keyboard->ta, '\n');
    }
    else
    {
        lv_textarea_add_text(keyboard->ta, txt);
    }
}

/**********************
 *	 STATIC FUNCTIONS
 **********************/
static void lv_keyboard_constructor(const lv_obj_class_t *class_p, lv_obj_t *obj)
{
    LV_UNUSED(class_p);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE);

    lv_keyboard_t *keyboard = (lv_keyboard_t *)obj;

    keyboard->ta = NULL;
    keyboard->mode = LV_KEYBOARD_MODE_TEXT_LOWER;
    keyboard->popovers = 0;

    lv_obj_align(obj, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_event_cb(obj, lv_keyboard_def_event_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_set_style_base_dir(obj, LV_BASE_DIR_LTR, 0);

    lv_keyboard_update_map(obj);

#if SAT_LV_ADD_CUSTOM
    lv_keyboard_update_pad_map(obj);
#endif
}

/**
 * Update the key and control map for the current mode
 * @param obj pointer to a keyboard object
 */
static void lv_keyboard_update_map(lv_obj_t *obj)
{
    lv_keyboard_t *keyboard = (lv_keyboard_t *)obj;

    lv_btnmatrix_set_map(obj, kb_map[keyboard->mode]);
    lv_keyboard_update_ctrl_map(obj);
}

#if SAT_LV_ADD_CUSTOM

static void lv_keyboard_update_pad_map(lv_obj_t *obj)
{
    lv_keyboard_t *keyboard = (lv_keyboard_t *)obj;

    lv_btnmatrix_set_map(obj, kb_map[keyboard->mode]);
    lv_btnmatrix_set_pad_map(obj, kb_pad_ctrl[keyboard->mode]);

    if (p_deafult_kb_img_map[keyboard->mode] != NULL)
    {

        lv_btnmatrix_set_btn_bg_map(obj, p_deafult_kb_img_map[keyboard->mode]);
    }
}

void lv_keyboard_update_img_map(lv_obj_t *obj, const void *arg[3][44])
{
    for (int i = 0; i < 44; i++)
    {
        p_deafult_kb_img_map[0][i] = arg[0][i];
        p_deafult_kb_img_map[1][i] = arg[1][i];
        p_deafult_kb_img_map[2][i] = arg[2][i];
    }
}

#endif

/**
 * Update the control map for the current mode
 * @param obj pointer to a keyboard object
 */
static void lv_keyboard_update_ctrl_map(lv_obj_t *obj)
{
    lv_keyboard_t *keyboard = (lv_keyboard_t *)obj;

    if (keyboard->popovers)
    {
        /*Apply the current control map (already includes LV_BTNMATRIX_CTRL_POPOVER flags)*/
        lv_btnmatrix_set_ctrl_map(obj, kb_ctrl[keyboard->mode]);
    }
    else
    {
        /*Make a copy of the current control map*/
        lv_btnmatrix_t *btnm = (lv_btnmatrix_t *)obj;
        lv_btnmatrix_ctrl_t *ctrl_map = lv_mem_alloc(btnm->btn_cnt * sizeof(lv_btnmatrix_ctrl_t));

        lv_memcpy(ctrl_map, kb_ctrl[keyboard->mode], sizeof(lv_btnmatrix_ctrl_t) * btnm->btn_cnt);

        /*Remove all LV_BTNMATRIX_CTRL_POPOVER flags*/
        for (uint16_t i = 0; i < btnm->btn_cnt; i++)
        {
            ctrl_map[i] &= (~LV_BTNMATRIX_CTRL_POPOVER);
        }

        /*Apply new control map and clean up*/
        lv_btnmatrix_set_ctrl_map(obj, ctrl_map);
        lv_mem_free(ctrl_map);
    }
}

#endif /*LV_USE_KEYBOARD*/
