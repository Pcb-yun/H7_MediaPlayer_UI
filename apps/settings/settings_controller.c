#include "settings_controller.h"
#include "../../H7_MediaPlayer_UI.h"
#include "../../ui/ui_focus.h"
#include "tim.h"
#include "stm32h7xx_hal.h"
#include "boot.h"

#ifndef H7_MEDIAPLAYER_FIRMWARE_VERSION
#define H7_MEDIAPLAYER_FIRMWARE_VERSION "v0.1.0"
#endif

#define BRIGHTNESS_DEFAULT   100u    // 上电默认亮度（%）
#define BRIGHTNESS_STEP      1u      // 每格旋钮的亮度步进（%）
#define BRIGHTNESS_MAX       100u
#define BRIGHTNESS_MIN       2u

#define ORIENTATION_COUNT    4u      // 0/90/180/270

static settings_navigate_cb_t navigate;

/* 跨屏保留的状态：亮度百分比与方向索引。亮度编辑态只在设置屏内有效，离屏自动复位 */
static uint8_t brightness_pct = BRIGHTNESS_DEFAULT;
static bool brightness_editing;
static uint8_t orientation_idx;

static const lv_display_rotation_t orientation_map[ORIENTATION_COUNT] = {
    LV_DISPLAY_ROTATION_0,
    LV_DISPLAY_ROTATION_90,
    LV_DISPLAY_ROTATION_180,
    LV_DISPLAY_ROTATION_270,
};

/* 行对象在 bind 时取得，editing 期间用于查找 group 与值标签 */
static lv_obj_t * brightness_row;
static lv_obj_t * orientation_row;
static lv_obj_t * brightness_value_lbl;
static lv_obj_t * orientation_value_lbl;

static lv_obj_t * row_value_label(lv_obj_t * row);
static void brightness_apply(void);
static void brightness_refresh_label(void);
static void orientation_refresh_label(void);
static void brightness_cb(lv_obj_t * obj, void * user_data);
static void brightness_key_cb(lv_event_t * e);
static void orientation_cb(lv_obj_t * obj, void * user_data);
static void reboot_cb(lv_obj_t * obj, void * user_data);
static void local_update_cb(lv_obj_t * obj, void * user_data);

static void settings_back_cb(lv_obj_t * obj, void * user_data)
{
    LV_UNUSED(obj);
    LV_UNUSED(user_data);
    /* 亮度处于编辑态时双击先退出编辑而非离开设置屏，避免遗留 editing 状态 */
    if(brightness_editing) {
        brightness_editing = false;
        lv_group_t * group = (brightness_row != NULL) ? lv_obj_get_group(brightness_row) : NULL;
        if(group != NULL) lv_group_set_editing(group, false);
        brightness_refresh_label();
        return;
    }
    if(navigate != NULL) navigate(SETTINGS_NAV_LAUNCHER);
}

static void settings_about_cb(lv_obj_t * obj, void * user_data)
{
    LV_UNUSED(obj);
    LV_UNUSED(user_data);
    if(navigate != NULL) navigate(SETTINGS_NAV_ABOUT);
}

static void about_back_cb(lv_obj_t * obj, void * user_data)
{
    LV_UNUSED(obj);
    LV_UNUSED(user_data);
    if(navigate != NULL) navigate(SETTINGS_NAV_SETTINGS);
}

/* setting_row 组件固定为 图标(0) + 标题(1) + 值(2)，值标签即第 3 个子项 */
static lv_obj_t * row_value_label(lv_obj_t * row)
{
    return (row != NULL) ? lv_obj_get_child(row, 2) : NULL;
}

/* 把当前亮度写入 TIM23_CH1 的比较寄存器：占空比 = pct * ARR / 100，ARR=99 */
static void brightness_apply(void)
{
    uint32_t compare = ((uint32_t)brightness_pct * 99u) / 100u;
    __HAL_TIM_SET_COMPARE(&htim23, TIM_CHANNEL_1, compare);
}

/* 把亮度数值写进行值标签；编辑态省略箭头提示用户正在调 */
static void brightness_refresh_label(void)
{
    if(brightness_value_lbl == NULL) return;
    if(brightness_editing) {
        lv_label_set_text_fmt(brightness_value_lbl, "%u%%", (unsigned)brightness_pct);
    }
    else {
        lv_label_set_text_fmt(brightness_value_lbl, "%u%%  ›", (unsigned)brightness_pct);
    }
}

/* 把当前方向写进行值标签：0° / 90° / 180° / 270° */
static void orientation_refresh_label(void)
{
    if(orientation_value_lbl == NULL) return;
    lv_label_set_text_fmt(orientation_value_lbl, "%u°  ›",
        (unsigned)(orientation_idx * 90u));
}

/* 亮度行单击：进出编辑态，编辑态下旋钮转动以 KEY 事件送达本对象 */
static void brightness_cb(lv_obj_t * obj, void * user_data)
{
    LV_UNUSED(obj);
    LV_UNUSED(user_data);
    lv_group_t * group = (brightness_row != NULL) ? lv_obj_get_group(brightness_row) : NULL;
    if(group == NULL) return;

    brightness_editing = !brightness_editing;
    lv_group_set_editing(group, brightness_editing);
    brightness_refresh_label();
}

/* 亮度行 KEY 事件：编辑态下捕获 LV_KEY_LEFT/RIGHT 调整百分比 */
static void brightness_key_cb(lv_event_t * e)
{
    if(!brightness_editing) return;

    uint32_t key = *(uint32_t *)lv_event_get_param(e);
    int32_t pct = (int32_t)brightness_pct;

    if(key == LV_KEY_RIGHT) pct += (int32_t)BRIGHTNESS_STEP;
    else if(key == LV_KEY_LEFT) pct -= (int32_t)BRIGHTNESS_STEP;
    else return;

    if(pct < (int32_t)BRIGHTNESS_MIN) pct = (int32_t)BRIGHTNESS_MIN;
    if(pct > (int32_t)BRIGHTNESS_MAX) pct = (int32_t)BRIGHTNESS_MAX;

    brightness_pct = (uint8_t)pct;
    brightness_apply();
    brightness_refresh_label();
}

/* 方向行单击：循环切换 0/90/180/270 并立即应用到默认显示 */
static void orientation_cb(lv_obj_t * obj, void * user_data)
{
    LV_UNUSED(obj);
    LV_UNUSED(user_data);
    orientation_idx = (uint8_t)((orientation_idx + 1u) % ORIENTATION_COUNT);
    lv_display_t * disp = lv_display_get_default();
    if(disp != NULL) lv_display_set_rotation(disp, orientation_map[orientation_idx]);
    orientation_refresh_label();
}

/* 重启行单击：弹出 msgbox 二次确认，自带关闭按钮可取消 */
static void reboot_cb(lv_obj_t * obj, void * user_data)
{
    LV_UNUSED(obj);
    LV_UNUSED(user_data);
    HAL_NVIC_SystemReset();
}

/* 本地更新行单击：通知主控切到固件文件浏览屏 */
static void local_update_cb(lv_obj_t * obj, void * user_data)
{
    LV_UNUSED(obj);
    LV_UNUSED(user_data);
    if(navigate != NULL) navigate(SETTINGS_NAV_FIRMWARE_BROWSER);
}

void settings_controller_init(settings_navigate_cb_t navigate_cb)
{
    navigate = navigate_cb;
}

void settings_controller_bind(lv_obj_t * screen)
{
    static const char * const rows[] = {
        "settings_brightness",
        "settings_orientation",
        "settings_reboot",
        "settings_local_update",
        "settings_about",
    };
    lv_obj_t * about;

    ui_focus_bind_names(screen, rows, sizeof(rows) / sizeof(rows[0]), true, settings_back_cb);

    brightness_row = lv_obj_find_by_name(screen, "settings_brightness");
    orientation_row = lv_obj_find_by_name(screen, "settings_orientation");
    lv_obj_t * reboot = lv_obj_find_by_name(screen, "settings_reboot");
    lv_obj_t * update = lv_obj_find_by_name(screen, "settings_local_update");
    about = lv_obj_find_by_name(screen, "settings_about");

    brightness_value_lbl = row_value_label(brightness_row);
    orientation_value_lbl = row_value_label(orientation_row);

    if(brightness_row != NULL) {
        ui_focus_bind_row(brightness_row, brightness_cb, NULL, NULL);
        lv_obj_add_event_cb(brightness_row, brightness_key_cb, LV_EVENT_KEY, NULL);
    }
    if(orientation_row != NULL) {
        ui_focus_bind_row(orientation_row, orientation_cb, NULL, NULL);
    }
    if(reboot != NULL) {
        ui_focus_bind_row(reboot, reboot_cb, NULL, NULL);
    }
    if(update != NULL) {
        ui_focus_bind_row(update, local_update_cb, NULL, NULL);
    }
    if(about != NULL) {
        ui_focus_bind_row(about, settings_about_cb, NULL, NULL);
    }

    /* 进屏时复位编辑态并把当前状态同步到硬件与显示 */
    brightness_editing = false;
    brightness_apply();
    brightness_refresh_label();
    orientation_refresh_label();
}

void settings_about_controller_bind(lv_obj_t * screen)
{
    static const char * const rows[] = {
        "about_intro",
        "about_repository",
        "about_version_row",
        "about_build_row",
    };
    lv_obj_t * version = lv_obj_find_by_name(screen, "about_firmware_version");
    lv_obj_t * build_time = lv_obj_find_by_name(screen, "about_build_time");

    if(version != NULL) lv_label_set_text(version, H7_MEDIAPLAYER_FIRMWARE_VERSION);
    if(build_time != NULL) lv_label_set_text(build_time, __DATE__ " " __TIME__);
    ui_focus_bind_names(screen, rows, sizeof(rows) / sizeof(rows[0]), true, about_back_cb);
}
