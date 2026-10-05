/**
 * @file main.c
 *
 * PC simulator entry point for a UI exported from LVGL Pro: opens a window,
 * initializes the exported UI library and starts the LVGL loop.
 */

/* Built only by sim/CMakeLists.txt; lets a glob-based embedded build skip it. */
#ifdef LVGL_PRO_SIMULATOR_BUILD

#include "lvgl.h"
#include "hal.h"
#include "../../UI_next/app/ui_app.h"
#include UI_HEADER   /* exported UI library: header, UI_INIT and size come from
                        project.xml via sim/CMakeLists.txt */

int main(void)
{
    lv_init();

    /* 显示/输入后端必须在未持锁状态创建: Windows 驱动在内部线程处理
     * WM_CREATE 时创建 display, 主线程若先持有 LVGL 锁会与其死锁 */
    hal_init(UI_WIDTH, UI_HEIGHT);

    /* Hold the LVGL mutex while building the UI to stay off the Windows
     * driver's render thread. */
    lv_lock();
    ui_app_init();
    lv_unlock();

    while(1) {
        uint32_t idle_ms = lv_timer_handler();
        if(idle_ms == LV_NO_TIMER_READY) idle_ms = LV_DEF_REFR_PERIOD;
        lv_sleep_ms(idle_ms);
    }

    return 0;
}

#endif /*LVGL_PRO_SIMULATOR_BUILD*/
