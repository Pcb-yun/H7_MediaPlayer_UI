#include "browser_controller.h"
#include "../../H7_MediaPlayer_UI_gen.h"
#include "../../ui/ui_focus.h"
#include "../../../lvgl_fatfs.h"
#include <stdint.h>
#include <string.h>

#define BROWSER_PRELOAD_MARGIN  3u
#define BROWSER_VISIBLE_ROWS    6u
#define BROWSER_WINDOW_SHIFT    (BROWSER_BACKEND_WINDOW_SIZE / 2u)
#define BROWSER_FOCUS_NONE      (-3)
#define BROWSER_ROW_EMPTY       (-2)
#define BROWSER_ROW_PARENT      (-1)

typedef struct {
    browser_backend_t backend;
    browser_controller_request_t request;
    lv_obj_t *screen;
    lv_obj_t *list_obj;
    lv_obj_t *path_obj;
    lv_obj_t *scroll_track_obj;
    lv_obj_t *scroll_thumb_obj;
    lv_obj_t *overlay_obj;
    lv_obj_t *menu_name_obj;
    lv_obj_t *cancel_obj;
    lv_obj_t *delete_obj;
    lv_obj_t *menu_source;
    char menu_path[LV_FS_MAX_PATH_LENGTH];
    char selected_path[LV_FS_MAX_PATH_LENGTH];
    int32_t focused_global;
    uint32_t menu_global;
    uint32_t pending_offset;
    uint32_t pending_global;
    int32_t pending_visual_y;
    bool preserve_visual_y;
    bool shifting;
    bool rebuilding;
    bool release_pending;
} browser_controller_t;

static browser_controller_t *controller;

static void render(void);
static void scroll_indicator_update(int32_t global_index);
static void focus_first(void);
static lv_obj_t *focus_global(uint32_t global_index);
static void row_click(lv_obj_t *obj, void *data);
static void row_back(lv_obj_t *obj, void *data);
static void row_menu(lv_obj_t *obj, void *data);
static void row_focused(lv_event_t *e);
static void browser_edge_cb(lv_group_t *group, bool forward);
static void shift_async(void *data);
static void screen_delete_cb(lv_event_t *e);

static void hidden(lv_obj_t *obj, bool value)
{
    if(obj != NULL) lv_obj_set_hidden(obj, value);
}

static char ascii_lower(char ch)
{
    return (ch >= 'A' && ch <= 'Z') ? (char)(ch + ('a' - 'A')) : ch;
}

static bool extension_is(const char *extension, const char *expected)
{
    if(extension == NULL || expected == NULL) return false;
    while(*extension != '\0' && *expected != '\0') {
        if(ascii_lower(*extension) != ascii_lower(*expected)) return false;
        extension++;
        expected++;
    }
    return *extension == '\0' && *expected == '\0';
}

static const char *file_extension(const char *name)
{
    const char *dot = NULL;
    const char *cursor;

    if(name == NULL) return NULL;
    for(cursor = name; *cursor != '\0'; cursor++) {
        if(*cursor == '.') dot = cursor;
    }
    return dot != NULL && dot[1] != '\0' ? dot + 1 : NULL;
}

/* 已知类型使用对应 LVGL 内建符号；所有图标保持统一颜色。 */
static const char *file_icon(const browser_backend_entry_t *entry)
{
    const char *ext;

    if(entry == NULL) return LV_SYMBOL_FILE;
    if(entry->is_directory) return LV_SYMBOL_DIRECTORY;

    ext = file_extension(entry->name);
    if(extension_is(ext, "mp3") || extension_is(ext, "wav") ||
       extension_is(ext, "flac") || extension_is(ext, "aac") ||
       extension_is(ext, "m4a") || extension_is(ext, "ogg") ||
       extension_is(ext, "opus")) return LV_SYMBOL_AUDIO;

    if(extension_is(ext, "jpg") || extension_is(ext, "jpeg") ||
       extension_is(ext, "png") || extension_is(ext, "bmp") ||
       extension_is(ext, "gif") || extension_is(ext, "webp")) return LV_SYMBOL_IMAGE;

    if(extension_is(ext, "mp4") || extension_is(ext, "avi") ||
       extension_is(ext, "mkv") || extension_is(ext, "mov") ||
       extension_is(ext, "m4v") || extension_is(ext, "ts")) return LV_SYMBOL_VIDEO;

    if(extension_is(ext, "txt") || extension_is(ext, "lrc") ||
       extension_is(ext, "log") || extension_is(ext, "ini") ||
       extension_is(ext, "cfg") || extension_is(ext, "json") ||
       extension_is(ext, "xml") || extension_is(ext, "c") ||
       extension_is(ext, "h") || extension_is(ext, "cpp")) return LV_SYMBOL_EDIT;

    if(extension_is(ext, "hex") || extension_is(ext, "bin") ||
       extension_is(ext, "elf") || extension_is(ext, "uf2")) return LV_SYMBOL_SAVE;

    if(extension_is(ext, "m3u") || extension_is(ext, "m3u8") ||
       extension_is(ext, "pls") || extension_is(ext, "zip") ||
       extension_is(ext, "7z") || extension_is(ext, "rar") ||
       extension_is(ext, "tar") || extension_is(ext, "gz")) return LV_SYMBOL_LIST;

    return LV_SYMBOL_FILE;
}
/* 行只对应当前缓存窗口内的相对索引；窗口平移后统一重建并重新绑定。 */
static lv_obj_t *add_row(const char *icon, const char *name,
                         int32_t entry_index)
{
    lv_obj_t *button;
    lv_obj_t *label;

    if(controller == NULL || controller->list_obj == NULL) return NULL;
    button = lv_button_create(controller->list_obj);
    lv_obj_set_width(button, lv_pct(100));
    lv_obj_set_flex_flow(button, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_all(button, 5, 0);
    lv_obj_set_style_pad_column(button, 7, 0);
    lv_obj_set_style_radius(button, 5, 0);
    lv_obj_set_style_bg_opa(button, LV_OPA_TRANSP, 0);
    lv_obj_set_style_bg_color(button, lv_color_hex(0x171717), LV_STATE_FOCUSED);
    lv_obj_set_style_bg_opa(button, LV_OPA_COVER, LV_STATE_FOCUSED);
    lv_obj_set_style_border_width(button, 2, LV_STATE_FOCUSED);
    lv_obj_set_style_border_color(button, lv_color_hex(0xff72b6), LV_STATE_FOCUSED);

    label = lv_label_create(button);
    lv_label_set_text(label, icon);
    if(cjk_sc_14 != NULL) lv_obj_set_style_text_font(label, cjk_sc_14, 0);

    label = lv_label_create(button);
    lv_obj_set_flex_grow(label, 1);
    lv_label_set_long_mode(label, LV_LABEL_LONG_MODE_DOTS);
    lv_label_set_text(label, name);

    /* 单击由 row_click 按条目类型分发；双击始终返回上一级。 */
    ui_focus_bind_row(button, row_click, row_back,
                      (void *)(intptr_t)entry_index);
    lv_obj_add_event_cb(button, row_focused, LV_EVENT_FOCUSED,
                        (void *)(intptr_t)entry_index);
    return button;
}

/* 只渲染有界缓存窗口。父目录入口仅出现在逻辑起点，避免随窗口重复出现。 */
static void render(void)
{
    uint32_t i;

    if(controller == NULL || controller->list_obj == NULL) return;

    /*
     * lv_obj_clean() removes the focused row before the other old rows.  LVGL
     * then temporarily refocuses the next row and emits LV_EVENT_FOCUSED for
     * it.  Those transient events must not drive the window preloader: their
     * local indexes belong to the directory/window that is being destroyed.
     * Otherwise a large unfiltered directory can queue shift_async() with an
     * obsolete global index and leave the newly rendered directory unfocused.
     */
    controller->rebuilding = true;
    lv_obj_clean(controller->list_obj);
    if(controller->path_obj != NULL)
        lv_label_set_text(controller->path_obj,
                          browser_backend_current_path(&controller->backend));

    if(browser_backend_window_offset(&controller->backend) == 0u &&
       browser_backend_can_go_up(&controller->backend)) {
        add_row(LV_SYMBOL_UP, "..", BROWSER_ROW_PARENT);
    }

    for(i = 0; i < browser_backend_window_count(&controller->backend); i++) {
        const browser_backend_entry_t *entry = browser_backend_entry(&controller->backend, i);
        if(entry != NULL)
            add_row(file_icon(entry), entry->name, (int32_t)i);
    }

    if(lv_obj_get_child_count(controller->list_obj) == 0u)
        add_row(LV_SYMBOL_LIST, "空目录", BROWSER_ROW_EMPTY);
    controller->rebuilding = false;
}

/* 自定义进度条按全目录位置计算，不受动态缓存窗口重建影响。 */
static void scroll_indicator_update(int32_t global_index)
{
    uint32_t total;
    int32_t track_height;
    int32_t thumb_height;
    int32_t thumb_y;

    if(controller == NULL || controller->scroll_track_obj == NULL ||
       controller->scroll_thumb_obj == NULL) return;

    total = browser_backend_total_count(&controller->backend);
    if(total <= BROWSER_VISIBLE_ROWS) {
        hidden(controller->scroll_track_obj, true);
        return;
    }
    hidden(controller->scroll_track_obj, false);
    lv_obj_update_layout(controller->scroll_track_obj);
    track_height = lv_obj_get_content_height(controller->scroll_track_obj);
    if(track_height <= 0) return;

    thumb_height = (int32_t)((uint32_t)track_height * BROWSER_VISIBLE_ROWS / total);
    if(thumb_height < 12) thumb_height = 12;
    if(thumb_height > track_height) thumb_height = track_height;
    if(global_index < 0) global_index = 0;
    if((uint32_t)global_index >= total) global_index = (int32_t)(total - 1u);
    thumb_y = (int32_t)((uint64_t)(track_height - thumb_height) *
              (uint32_t)global_index / (total - 1u));
    lv_obj_set_height(controller->scroll_thumb_obj, thumb_height);
    lv_obj_set_y(controller->scroll_thumb_obj, thumb_y);
}

static void focus_first(void)
{
    lv_obj_t *first;

    if(controller == NULL || controller->list_obj == NULL) return;
    first = lv_obj_get_child(controller->list_obj, 0);
    controller->focused_global = BROWSER_FOCUS_NONE;
    if(first != NULL) lv_group_focus_obj(first);
}

/* 在当前缓存窗口中恢复某个全局文件索引的焦点。 */
static lv_obj_t *focus_global(uint32_t global_index)
{
    uint32_t offset;
    uint32_t local;
    int32_t child_index;
    lv_obj_t *target;

    if(controller == NULL || controller->list_obj == NULL) return NULL;
    offset = browser_backend_window_offset(&controller->backend);
    if(global_index < offset ||
       global_index >= offset + browser_backend_window_count(&controller->backend)) return NULL;

    local = global_index - offset;
    child_index = (int32_t)local;
    if(offset == 0u && browser_backend_can_go_up(&controller->backend)) child_index++;
    target = lv_obj_get_child(controller->list_obj, child_index);
    if(target != NULL) lv_group_focus_obj(target);
    return target;
}

static uint32_t last_window_offset(void)
{
    uint32_t total;

    if(controller == NULL) return 0u;
    total = browser_backend_total_count(&controller->backend);
    return total > BROWSER_BACKEND_WINDOW_SIZE ?
           total - BROWSER_BACKEND_WINDOW_SIZE : 0u;
}

/* 合并同一 LVGL 周期内的平移请求，避免在焦点事件里删除当前对象。 */
static void schedule_shift(uint32_t offset, uint32_t focused_global,
                           lv_obj_t *focused_row, bool preserve_position)
{
    lv_area_t row_area;
    lv_area_t list_area;

    if(controller == NULL || controller->shifting) return;
    controller->pending_offset = offset;
    controller->pending_global = focused_global;
    controller->preserve_visual_y = preserve_position && focused_row != NULL &&
                                    controller->list_obj != NULL;
    if(controller->preserve_visual_y) {
        lv_obj_get_coords(focused_row, &row_area);
        lv_obj_get_coords(controller->list_obj, &list_area);
        controller->pending_visual_y = row_area.y1 - list_area.y1;
    }
    controller->shifting = true;
    lv_async_call(shift_async, NULL);
}

static void shift_async(void *data)
{
    browser_backend_result_t result;

    LV_UNUSED(data);
    if(controller == NULL) return;
    if(controller->release_pending) {
        lv_free(controller);
        controller = NULL;
        return;
    }

    result = browser_backend_load_window(&controller->backend,
                                         controller->pending_offset);
    if(result != BROWSER_BACKEND_OK) {
        LV_LOG_ERROR("browser window preload failed: %d", (int)result);
        controller->shifting = false;
        return;
    }

    render();
    {
        lv_obj_t *target = focus_global(controller->pending_global);
        if(target != NULL && controller->preserve_visual_y && controller->list_obj != NULL) {
            lv_area_t row_area;
            lv_area_t list_area;
            int32_t scroll_y;

            lv_obj_update_layout(controller->list_obj);
            lv_obj_get_coords(target, &row_area);
            lv_obj_get_coords(controller->list_obj, &list_area);
            scroll_y = lv_obj_get_scroll_y(controller->list_obj) +
                       (row_area.y1 - list_area.y1 - controller->pending_visual_y);
            lv_obj_scroll_to_y(controller->list_obj, scroll_y, LV_ANIM_OFF);
        }
        else if(target != NULL) {
            lv_obj_scroll_to_view(target, LV_ANIM_OFF);
        }
    }
    controller->preserve_visual_y = false;
    controller->shifting = false;
}

/* 禁止 LVGL 在缓存窗口内部自行回绕；到达边界后按全目录索引移动。 */
static void browser_edge_cb(lv_group_t *group, bool forward)
{
    uint32_t total;
    uint32_t target;
    uint32_t offset;
    uint32_t last_offset;

    LV_UNUSED(group);
    if(controller == NULL || controller->shifting || controller->rebuilding) return;
    total = browser_backend_total_count(&controller->backend);
    if(total == 0u) return;

    if(forward) {
        target = controller->focused_global >= 0 &&
                 (uint32_t)controller->focused_global + 1u < total ?
                 (uint32_t)controller->focused_global + 1u : 0u;
    }
    else {
        target = controller->focused_global > 0 ?
                 (uint32_t)controller->focused_global - 1u : total - 1u;
    }

    last_offset = last_window_offset();
    if(target == 0u) offset = 0u;
    else if(target == total - 1u) offset = last_offset;
    else {
        offset = target > BROWSER_WINDOW_SHIFT ?
                 target - BROWSER_WINDOW_SHIFT : 0u;
        if(offset > last_offset) offset = last_offset;
    }
    schedule_shift(offset, target, NULL, false);
}
/* 焦点靠近缓存边缘时提前平移半个窗口，并按全局索引恢复同一文件。 */
static void row_focused(lv_event_t *e)
{
    int32_t local = (int32_t)(intptr_t)lv_event_get_user_data(e);
    int32_t previous;
    uint32_t global;
    uint32_t offset;
    uint32_t count;
    uint32_t total;
    uint32_t last_offset;
    uint32_t next_offset;

    if(controller == NULL || controller->rebuilding) return;
    previous = controller->focused_global;

    if(local == BROWSER_ROW_PARENT) {
        controller->focused_global = BROWSER_ROW_PARENT;
        scroll_indicator_update(0);
        return;
    }
    if(local < 0) {
        controller->focused_global = BROWSER_FOCUS_NONE;
        scroll_indicator_update(0);
        return;
    }

    offset = browser_backend_window_offset(&controller->backend);
    count = browser_backend_window_count(&controller->backend);
    total = browser_backend_total_count(&controller->backend);
    global = offset + (uint32_t)local;
    controller->focused_global = (int32_t)global;
    scroll_indicator_update((int32_t)global);
    if(controller->shifting || count == 0u || total <= count) return;

    last_offset = last_window_offset();


    if(previous >= 0 && global > (uint32_t)previous &&
       (uint32_t)local + BROWSER_PRELOAD_MARGIN >= count &&
       offset < last_offset) {
        next_offset = offset + BROWSER_WINDOW_SHIFT;
        if(next_offset > last_offset) next_offset = last_offset;
        schedule_shift(next_offset, global, lv_event_get_current_target_obj(e), true);
    }
    else if(previous >= 0 && global < (uint32_t)previous &&
            (uint32_t)local < BROWSER_PRELOAD_MARGIN && offset > 0u) {
        next_offset = offset > BROWSER_WINDOW_SHIFT ?
                      offset - BROWSER_WINDOW_SHIFT : 0u;
        schedule_shift(next_offset, global, lv_event_get_current_target_obj(e), true);
    }
}

static void reload_at_start(void)
{
    if(controller == NULL) return;
    if(browser_backend_load_window(&controller->backend, 0u) != BROWSER_BACKEND_OK) {
        LV_LOG_ERROR("browser directory reload failed");
        return;
    }
    render();
    focus_first();
}

static void row_click(lv_obj_t *obj, void *data)
{
    intptr_t index = (intptr_t)data;
    const browser_backend_entry_t *entry;

    LV_UNUSED(obj);
    if(controller == NULL) return;
    if(index == BROWSER_ROW_PARENT) {
        if(browser_backend_up(&controller->backend) == BROWSER_BACKEND_OK) reload_at_start();
        else LV_LOG_ERROR("browser parent directory read failed");
        return;
    }
    if(index < 0) return;

    entry = browser_backend_entry(&controller->backend, (uint32_t)index);
    if(entry == NULL) return;
    if(entry->is_directory) {
        if(browser_backend_enter(&controller->backend, entry->name) == BROWSER_BACKEND_OK)
            reload_at_start();
        else
            LV_LOG_ERROR("browser directory open failed: %s", entry->name);
        return;
    }

    /* 普通文件浏览模式先打开可扩展操作菜单；选择器模式则直接回调文件。 */
    if(controller->request.allow_delete) {
        row_menu(obj, data);
        return;
    }
    if(controller->request.select_cb != NULL &&
       browser_backend_full_path(&controller->backend, (uint32_t)index,
                                 controller->selected_path,
                                 sizeof(controller->selected_path)) == BROWSER_BACKEND_OK) {
        controller->request.select_cb(controller->selected_path,
                                      controller->request.user_data);
    }
}

static void row_back(lv_obj_t *obj, void *data)
{
    LV_UNUSED(obj);
    LV_UNUSED(data);
    if(controller == NULL) return;

    if(browser_backend_can_go_up(&controller->backend)) {
        if(browser_backend_up(&controller->backend) == BROWSER_BACKEND_OK) reload_at_start();
        else LV_LOG_ERROR("browser parent directory read failed");
    }
    else {
        ui_router_open(controller->request.return_route, false);
    }
}

static void menu_scope(bool menu_only)
{
    lv_group_t *group;
    uint32_t i;
    uint32_t count;

    if(controller == NULL || controller->list_obj == NULL) return;
    group = lv_group_get_default();
    if(group == NULL) return;

    count = lv_obj_get_child_count(controller->list_obj);
    for(i = 0; i < count; i++) {
        lv_obj_t *row = lv_obj_get_child(controller->list_obj, i);
        if(menu_only) lv_group_remove_obj(row);
        else lv_group_add_obj(group, row);
    }
    /* 菜单内只在取消/删除之间回绕；关闭后恢复目录级边界切窗。 */
    lv_group_set_edge_cb(group, menu_only ? NULL : browser_edge_cb);
    lv_group_set_wrap(group, menu_only);
}

static void row_menu(lv_obj_t *obj, void *data)
{
    intptr_t index = (intptr_t)data;
    const browser_backend_entry_t *entry;

    if(controller == NULL || index < 0 || controller->overlay_obj == NULL) return;
    entry = browser_backend_entry(&controller->backend, (uint32_t)index);
    if(entry == NULL || entry->is_directory) return;
    if(browser_backend_full_path(&controller->backend, (uint32_t)index,
       controller->menu_path, sizeof(controller->menu_path)) != BROWSER_BACKEND_OK) return;

    controller->menu_global = browser_backend_window_offset(&controller->backend) +
                              (uint32_t)index;
    controller->menu_source = obj;
    if(controller->menu_name_obj != NULL)
        lv_label_set_text(controller->menu_name_obj, entry->name);
    hidden(controller->overlay_obj, false);
    lv_obj_move_foreground(controller->overlay_obj);
    menu_scope(true);
    if(controller->cancel_obj != NULL) lv_group_focus_obj(controller->cancel_obj);
    else if(controller->delete_obj != NULL) lv_group_focus_obj(controller->delete_obj);
}

static void close_menu(bool restore)
{
    if(controller == NULL) return;
    hidden(controller->overlay_obj, true);
    menu_scope(false);
    if(restore && controller->menu_source != NULL)
        lv_group_focus_obj(controller->menu_source);
    controller->menu_source = NULL;
    controller->menu_path[0] = '\0';
}

static void menu_close_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    close_menu(true);
}

static void delete_cb(lv_event_t *e)
{
    uint32_t total;
    uint32_t target;
    uint32_t offset;

    LV_UNUSED(e);
    if(controller == NULL) return;
    if(controller->menu_path[0] == '\0' ||
       !lvgl_fatfs_remove(controller->menu_path)) {
        if(controller->menu_name_obj != NULL)
            lv_label_set_text(controller->menu_name_obj, "删除失败");
        return;
    }

    target = controller->menu_global;
    close_menu(false);
    if(browser_backend_reload(&controller->backend) != BROWSER_BACKEND_OK) {
        LV_LOG_ERROR("browser reload after delete failed");
        return;
    }

    total = browser_backend_total_count(&controller->backend);
    if(total == 0u) {
        render();
        focus_first();
        return;
    }
    if(target >= total) target = total - 1u;
    offset = browser_backend_window_offset(&controller->backend);
    if(target < offset || target >= offset + browser_backend_window_count(&controller->backend)) {
        offset = target > BROWSER_PRELOAD_MARGIN ? target - BROWSER_PRELOAD_MARGIN : 0u;
        if(browser_backend_load_window(&controller->backend, offset) != BROWSER_BACKEND_OK) {
            LV_LOG_ERROR("browser reload target window failed");
            return;
        }
    }
    render();
    controller->focused_global = BROWSER_FOCUS_NONE;
    focus_global(target);
}

static void screen_delete_cb(lv_event_t *e)
{
    lv_obj_t *screen = lv_event_get_current_target_obj(e);

    if(controller == NULL || controller->screen != screen) return;
    {
        lv_group_t *group = lv_group_get_default();
        if(group != NULL) {
            lv_group_set_edge_cb(group, NULL);
            lv_group_set_wrap(group, true);
        }
    }
    controller->screen = NULL;
    controller->list_obj = NULL;
    controller->path_obj = NULL;
    controller->scroll_track_obj = NULL;
    controller->scroll_thumb_obj = NULL;
    controller->overlay_obj = NULL;
    controller->menu_name_obj = NULL;
    controller->cancel_obj = NULL;
    controller->delete_obj = NULL;
    controller->menu_source = NULL;
    if(controller->shifting) {
        /* 已提交的异步预加载无法取消，由回调在下一轮释放上下文。 */
        controller->release_pending = true;
        return;
    }
    lv_free(controller);
    controller = NULL;
}

bool browser_controller_open(const browser_controller_request_t *value)
{
    browser_backend_config_t config;
    browser_controller_t *created;

    if(value == NULL) return false;
    if(controller != NULL) {
        LV_LOG_ERROR("browser is already active");
        return false;
    }

    created = lv_malloc(sizeof(*created));
    if(created == NULL) {
        LV_LOG_ERROR("browser context allocation failed (%u bytes)",
                     (unsigned)sizeof(*created));
        return false;
    }
    memset(created, 0, sizeof(*created));
    created->focused_global = BROWSER_FOCUS_NONE;
    created->request = *value;

    config.root_path = value->root_path;
    config.extensions = value->extensions;
    config.allow_directories = value->allow_directories;
    config.filter_cb = value->filter_cb;
    config.user_data = value->user_data;
    if(browser_backend_init(&created->backend, &config) != BROWSER_BACKEND_OK) {
        LV_LOG_ERROR("browser root open failed: %s",
                     value->root_path != NULL ? value->root_path : "C:/");
        lv_free(created);
        return false;
    }

    controller = created;
    ui_router_open(UI_ROUTE_BROWSER, true);
    return true;
}

void browser_controller_bind(lv_obj_t *screen)
{
    lv_group_t *group;
    lv_obj_t *first;

    if(controller == NULL || screen == NULL) return;
    controller->screen = screen;
    controller->list_obj = lv_obj_find_by_name(screen, "browser_list");
    controller->path_obj = lv_obj_find_by_name(screen, "browser_path");
    controller->scroll_track_obj = lv_obj_find_by_name(screen, "browser_scroll_track");
    controller->scroll_thumb_obj = lv_obj_find_by_name(screen, "browser_scroll_thumb");
    controller->overlay_obj = lv_obj_find_by_name(screen, "browser_menu_overlay");
    controller->menu_name_obj = lv_obj_find_by_name(screen, "browser_menu_name");
    controller->cancel_obj = lv_obj_find_by_name(screen, "browser_menu_cancel");
    controller->delete_obj = lv_obj_find_by_name(screen, "browser_menu_delete");
    controller->menu_source = NULL;
    controller->menu_path[0] = '\0';
    controller->focused_global = BROWSER_FOCUS_NONE;
    controller->shifting = false;

    hidden(controller->overlay_obj, true);
    if(controller->list_obj != NULL)
        lv_obj_set_scrollbar_mode(controller->list_obj, LV_SCROLLBAR_MODE_OFF);
    if(controller->cancel_obj != NULL)
        lv_obj_add_event_cb(controller->cancel_obj, menu_close_cb,
                            LV_EVENT_SHORT_CLICKED, NULL);
    if(controller->delete_obj != NULL) {
        lv_obj_add_event_cb(controller->delete_obj, delete_cb,
                            LV_EVENT_SHORT_CLICKED, NULL);
        lv_obj_add_event_cb(controller->delete_obj, menu_close_cb,
                            LV_EVENT_LONG_PRESSED, NULL);
    }
    lv_obj_add_event_cb(screen, screen_delete_cb, LV_EVENT_DELETE, NULL);

    render();
    focus_first();
    first = controller->list_obj != NULL ?
            lv_obj_get_child(controller->list_obj, 0) : NULL;
    group = first != NULL ? lv_obj_get_group(first) : NULL;
    /* 缓存窗口不自行回绕；边界回调负责跨窗口和全目录首尾循环。 */
    if(group != NULL) {
        lv_group_set_wrap(group, false);
        lv_group_set_edge_cb(group, browser_edge_cb);
    }
}
