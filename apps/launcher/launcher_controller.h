#ifndef H7_LAUNCHER_CONTROLLER_H
#define H7_LAUNCHER_CONTROLLER_H

#include "lvgl.h"

typedef enum {
    LAUNCHER_NAV_HOME,
    LAUNCHER_NAV_SETTINGS,
    LAUNCHER_NAV_MUSIC,
    LAUNCHER_NAV_FILES,
    LAUNCHER_NAV_SYSTEM_MONITOR,
    LAUNCHER_NAV_PLACEHOLDER,
} launcher_nav_target_t;

typedef void (*launcher_navigate_cb_t)(launcher_nav_target_t target);

void launcher_controller_init(launcher_navigate_cb_t navigate_cb);
void launcher_controller_reset(void);
void launcher_controller_bind(lv_obj_t * screen);
uint8_t launcher_controller_selected(void);
const char * launcher_controller_selected_name(void);

#endif
