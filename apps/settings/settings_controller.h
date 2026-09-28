#ifndef H7_SETTINGS_CONTROLLER_H
#define H7_SETTINGS_CONTROLLER_H

#include "lvgl.h"

typedef enum {
    SETTINGS_NAV_LAUNCHER,
    SETTINGS_NAV_ABOUT,
    SETTINGS_NAV_SETTINGS,
    SETTINGS_NAV_FIRMWARE_BROWSER,   // 本地更新：跳转到固件文件浏览屏
} settings_nav_target_t;

typedef void (*settings_navigate_cb_t)(settings_nav_target_t target);

void settings_controller_init(settings_navigate_cb_t navigate_cb);
void settings_controller_bind(lv_obj_t * screen);
void settings_about_controller_bind(lv_obj_t * screen);

#endif
