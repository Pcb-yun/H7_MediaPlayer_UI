#include "launcher_controller.h"
#include "../../ui/ui_focus.h"
#include <stdint.h>

#define LAUNCHER_PAGE_SIZE 4u

typedef struct {
    const char * name;
} launcher_app_t;

static const launcher_app_t apps[] = {
    { "设置" },
    { "音乐" },
    { "文件" },
    { "图片" },
    { "视频" },
    { "相机" },
    { "系统监视器" },
};

#define APP_COUNT ((uint8_t)(sizeof(apps) / sizeof(apps[0])))

static launcher_navigate_cb_t navigate;
static uint8_t selected;
static bool focus_guard;

static void set_hidden(lv_obj_t * obj, bool hidden)
{
    if(obj != NULL) lv_obj_set_hidden(obj, hidden);
}

static void update_page(lv_obj_t * screen)
{
    uint8_t page = selected / LAUNCHER_PAGE_SIZE;
    set_hidden(lv_obj_find_by_name(screen, "launcher_page_0"), page != 0u);
    set_hidden(lv_obj_find_by_name(screen, "launcher_page_1"), page != 1u);
    set_hidden(lv_obj_find_by_name(screen, "launcher_dots_page_0"), page != 0u);
    set_hidden(lv_obj_find_by_name(screen, "launcher_dots_page_1"), page != 1u);
}

static void app_cb(lv_obj_t * obj, void * user_data)
{
    LV_UNUSED(obj);
    selected = (uint8_t)(uintptr_t)user_data;
    if(navigate == NULL) return;
    if(selected == 0u) navigate(LAUNCHER_NAV_SETTINGS);
    else if(selected == 1u) navigate(LAUNCHER_NAV_MUSIC);
    else if(selected == 2u) navigate(LAUNCHER_NAV_FILES);
    else if(selected == 6u) navigate(LAUNCHER_NAV_SYSTEM_MONITOR);
    else navigate(LAUNCHER_NAV_PLACEHOLDER);
}

static void back_cb(lv_obj_t * obj, void * user_data)
{
    LV_UNUSED(obj);
    LV_UNUSED(user_data);
    if(navigate != NULL) navigate(LAUNCHER_NAV_HOME);
}

static void focus_cb(lv_event_t * e)
{
    uint8_t focused = (uint8_t)(uintptr_t)lv_event_get_user_data(e);
    uint8_t page = selected / LAUNCHER_PAGE_SIZE;
    uint8_t first = page * LAUNCHER_PAGE_SIZE;
    uint8_t last = first + LAUNCHER_PAGE_SIZE - 1u;
    lv_obj_t * screen;
    lv_obj_t * target;
    char name[24];

    if(focus_guard) return;
    if(last >= APP_COUNT) last = APP_COUNT - 1u;

    if(selected == last && focused == first) selected = (uint8_t)((last + 1u) % APP_COUNT);
    else if(selected == first && focused == last) selected = (uint8_t)((first + APP_COUNT - 1u) % APP_COUNT);
    else {
        selected = focused;
        return;
    }

    screen = lv_obj_get_screen(lv_event_get_target(e));
    update_page(screen);
    lv_snprintf(name, sizeof(name), "launcher_tile_%u", (unsigned)selected);
    target = lv_obj_find_by_name(screen, name);
    if(target != NULL) {
        focus_guard = true;
        lv_group_focus_obj(target);
        focus_guard = false;
    }
}

void launcher_controller_init(launcher_navigate_cb_t navigate_cb)
{
    navigate = navigate_cb;
}

void launcher_controller_reset(void)
{
    selected = 0u;
}

uint8_t launcher_controller_selected(void)
{
    return selected;
}

const char * launcher_controller_selected_name(void)
{
    return apps[selected % APP_COUNT].name;
}

void launcher_controller_bind(lv_obj_t * screen)
{
    char name[24];
    lv_obj_t * selected_obj = NULL;

    update_page(screen);
    for(uint8_t i = 0; i < APP_COUNT; i++) {
        lv_obj_t * tile;
        lv_snprintf(name, sizeof(name), "launcher_tile_%u", (unsigned)i);
        tile = lv_obj_find_by_name(screen, name);
        if(tile == NULL) continue;
        /* 单击进入（延迟到双击窗口结束），双击回主页，长按走全局返回主页 */
        ui_focus_bind_row(tile, app_cb, back_cb, (void *)(uintptr_t)i);
        lv_obj_add_event_cb(tile, focus_cb, LV_EVENT_FOCUSED, (void *)(uintptr_t)i);
        if(i == selected) selected_obj = tile;
    }

    if(selected_obj != NULL) {
        lv_group_t * group = lv_obj_get_group(selected_obj);
        if(group != NULL) lv_group_set_wrap(group, true);
        focus_guard = true;
        lv_group_focus_obj(selected_obj);
        focus_guard = false;
    }
}
