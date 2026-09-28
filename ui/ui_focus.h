#ifndef H7_UI_FOCUS_H
#define H7_UI_FOCUS_H

#include "lvgl.h"

/**
 * 行动作回调
 * @param obj 触发动作的对象（对象已删除或来自全局动作时为 NULL）
 * @param user_data 绑定该行时传入的数据
 */
typedef void (*ui_focus_action_cb_t)(lv_obj_t * obj, void * user_data);

/**
 * 设置全局长按动作（返回主页），UI 初始化时调用一次：
 * 长按任一绑定行都会走这里，各页不再各自处理长按。
 * @param home_cb 长按动作，NULL 表示长按不做任何事
 */
void ui_focus_set_home_cb(ui_focus_action_cb_t home_cb);

/**
 * 绑定一行按键行为（单击/双击/长按都由本模块统一调度）：
 * 单击不会立即回调，而是等双击判定窗口过去后才回调 click_cb，
 * 这样双击就不会先触发一次进入再返回。
 * @param obj 可聚焦行对象
 * @param click_cb 单击确认动作，NULL 表示保留原有设置
 * @param back_cb 双击返回上一级动作，NULL 表示保留原有设置
 * @param user_data 回调附带数据，NULL 表示保留原有设置
 */
void ui_focus_bind_row(lv_obj_t * obj,
                       ui_focus_action_cb_t click_cb,
                       ui_focus_action_cb_t back_cb,
                       void * user_data);

/**
 * 覆盖某一行的长按动作（默认是全局返回主页）
 * @param obj 可聚焦行对象
 * @param long_cb 长按动作，NULL 表示回到全局长按动作
 */
void ui_focus_set_row_long_cb(lv_obj_t * obj, ui_focus_action_cb_t long_cb);

/**
 * 按名字批量绑定：双击 back_cb 返回上一级，长按走全局动作；
 * 各行自己的单击动作由调用方随后用 ui_focus_bind_row() 补充。
 * @param screen 屏幕对象
 * @param names 行对象名数组
 * @param count 名字数量
 * @param wrap 焦点是否循环
 * @param back_cb 双击返回动作
 * @return 第一个找到的行对象（无则 NULL）
 */
lv_obj_t * ui_focus_bind_names(lv_obj_t * screen,
                               const char * const * names,
                               uint32_t count,
                               bool wrap,
                               ui_focus_action_cb_t back_cb);

#endif
