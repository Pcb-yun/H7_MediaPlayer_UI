#ifndef H7_SYSTEM_MONITOR_H
#define H7_SYSTEM_MONITOR_H

#include "lvgl.h"
#include "../../ui/ui_focus.h"

/**
 * 绑定系统监视页
 * @param screen 已创建的屏幕
 * @param back_cb 双击返回上一级的动作
 */
void system_monitor_bind(lv_obj_t * screen, ui_focus_action_cb_t back_cb);

#endif
