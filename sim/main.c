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
#include UI_HEADER   /* exported UI library: header, UI_INIT and size come from
                        project.xml via sim/CMakeLists.txt */

int main(void)
{
    lv_init();

    /* Hold the LVGL mutex while building the UI: a no-op without an OS, but
     * required to stay off the Windows driver's render thread. */
    lv_lock();

    hal_init(UI_WIDTH, UI_HEIGHT);
    UI_INIT("A:");      /* "A:" = file-system drive for file-based assets */

    /* 从与固件相同的主页面启动，并绑定应用级交互。
     * Windows: 鼠标滚轮=旋钮旋转，中键按下/长按=旋钮按钮。
     * 键盘: Tab/PageDown=下一项，PageUp=上一项，Enter=按下/长按。 */
    lv_obj_t * home = home_create();
    if(home != NULL) {
        H7_MediaPlayer_UI_bind(home);
        lv_screen_load(home);
    }

    lv_unlock();

    while(1) {
        uint32_t idle_ms = lv_timer_handler();
        if(idle_ms == LV_NO_TIMER_READY) idle_ms = LV_DEF_REFR_PERIOD;
        lv_sleep_ms(idle_ms);
    }

    return 0;
}

#endif /*LVGL_PRO_SIMULATOR_BUILD*/
