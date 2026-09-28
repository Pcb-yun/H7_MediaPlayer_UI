#ifndef H7_BROWSER_BACKEND_H
#define H7_BROWSER_BACKEND_H

#include "lvgl.h"
#include <stdbool.h>
#include <stdint.h>

#ifndef BROWSER_BACKEND_WINDOW_SIZE
#define BROWSER_BACKEND_WINDOW_SIZE 16u
#endif

#ifndef BROWSER_BACKEND_EXTENSIONS_SIZE
#define BROWSER_BACKEND_EXTENSIONS_SIZE 96u
#endif

typedef enum {
    BROWSER_BACKEND_OK = 0,
    BROWSER_BACKEND_INVALID_ARG,
    BROWSER_BACKEND_OPEN_FAILED,
    BROWSER_BACKEND_READ_FAILED,
    BROWSER_BACKEND_PATH_TOO_LONG,
    BROWSER_BACKEND_NOT_DIRECTORY,
} browser_backend_result_t;

typedef struct {
    char name[LV_FS_MAX_PATH_LENGTH];
    bool is_directory;
} browser_backend_entry_t;

/* 返回 false 可隐藏该条目。扩展名和目录权限过滤会先执行。 */
typedef bool (*browser_backend_filter_cb_t)(const char *name, bool is_directory,
                                             void *user_data);

typedef struct {
    const char *root_path;       /* 例如 "C:/"；NULL 时使用 "C:/" */
    const char *extensions;      /* "mp3;wav;flac"；NULL/空串表示全部文件 */
    bool allow_directories;
    browser_backend_filter_cb_t filter_cb;
    void *user_data;
} browser_backend_config_t;

typedef struct {
    char root_path[LV_FS_MAX_PATH_LENGTH];
    char current_path[LV_FS_MAX_PATH_LENGTH];
    char extensions[BROWSER_BACKEND_EXTENSIONS_SIZE];
    browser_backend_filter_cb_t filter_cb;
    void *user_data;
    bool allow_directories;

    browser_backend_entry_t entries[BROWSER_BACKEND_WINDOW_SIZE];
    char scratch[LV_FS_MAX_PATH_LENGTH];
    uint32_t window_offset;
    uint32_t window_count;
    uint32_t total_count;
} browser_backend_t;

browser_backend_result_t browser_backend_init(browser_backend_t *backend,
                                               const browser_backend_config_t *config);
browser_backend_result_t browser_backend_reload(browser_backend_t *backend);
/* 从任意逻辑条目开始加载有界窗口；控制器用它实现连续滚动预加载。 */
browser_backend_result_t browser_backend_load_window(browser_backend_t *backend,
                                                      uint32_t offset);

browser_backend_result_t browser_backend_enter(browser_backend_t *backend,
                                                const char *directory_name);
browser_backend_result_t browser_backend_up(browser_backend_t *backend);
bool browser_backend_can_go_up(const browser_backend_t *backend);

const browser_backend_entry_t *browser_backend_entry(const browser_backend_t *backend,
                                                      uint32_t index);
const char *browser_backend_current_path(const browser_backend_t *backend);
uint32_t browser_backend_window_count(const browser_backend_t *backend);
uint32_t browser_backend_window_offset(const browser_backend_t *backend);
uint32_t browser_backend_total_count(const browser_backend_t *backend);

browser_backend_result_t browser_backend_full_path(const browser_backend_t *backend,
                                                   uint32_t index,
                                                   char *buffer,
                                                   uint32_t buffer_size);

#endif
