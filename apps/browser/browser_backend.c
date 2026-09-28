#include "browser_backend.h"
#include <string.h>

#define BROWSER_DEFAULT_ROOT "C:/"

static char ascii_lower(char ch)
{
    return (ch >= 'A' && ch <= 'Z') ? (char)(ch + ('a' - 'A')) : ch;
}

static bool extension_matches(const char *list, const char *name)
{
    const char *dot = NULL;
    const char *p;

    if(list == NULL || list[0] == '\0') return true;
    for(p = name; *p != '\0'; p++) if(*p == '.') dot = p + 1;
    if(dot == NULL || *dot == '\0') return false;

    p = list;
    while(*p != '\0') {
        const char *ext = dot;
        while(*p == ' ' || *p == '.' || *p == ';' || *p == ',') p++;
        while(*p != '\0' && *p != ';' && *p != ',' && *p != ' ') {
            if(*ext == '\0' || ascii_lower(*p) != ascii_lower(*ext)) break;
            p++;
            ext++;
        }
        if(*ext == '\0' && (*p == '\0' || *p == ';' || *p == ',' || *p == ' ')) return true;
        while(*p != '\0' && *p != ';' && *p != ',') p++;
    }
    return false;
}

static bool entry_visible(const browser_backend_t *backend, const char *name, bool is_directory)
{
    if(is_directory && !backend->allow_directories) return false;
    if(!is_directory && !extension_matches(backend->extensions, name)) return false;
    return backend->filter_cb == NULL || backend->filter_cb(name, is_directory, backend->user_data);
}

static browser_backend_result_t copy_root(browser_backend_t *backend, const char *root)
{
    uint32_t length;
    if(root == NULL || root[0] == '\0') root = BROWSER_DEFAULT_ROOT;
    length = (uint32_t)lv_strlen(root);
    if(length + 2u > sizeof(backend->root_path)) return BROWSER_BACKEND_PATH_TOO_LONG;
    lv_snprintf(backend->root_path, sizeof(backend->root_path), "%s", root);
    if(length == 0u || backend->root_path[length - 1u] != '/') {
        backend->root_path[length] = '/';
        backend->root_path[length + 1u] = '\0';
    }
    lv_snprintf(backend->current_path, sizeof(backend->current_path), "%s", backend->root_path);
    return BROWSER_BACKEND_OK;
}

browser_backend_result_t browser_backend_init(browser_backend_t *backend,
                                               const browser_backend_config_t *config)
{
    browser_backend_result_t result;
    if(backend == NULL || config == NULL) return BROWSER_BACKEND_INVALID_ARG;
    memset(backend, 0, sizeof(*backend));
    result = copy_root(backend, config->root_path);
    if(result != BROWSER_BACKEND_OK) return result;
    if(config->extensions != NULL)
        lv_snprintf(backend->extensions, sizeof(backend->extensions), "%s", config->extensions);
    backend->allow_directories = config->allow_directories;
    backend->filter_cb = config->filter_cb;
    backend->user_data = config->user_data;
    return browser_backend_reload(backend);
}

browser_backend_result_t browser_backend_reload(browser_backend_t *backend)
{
    lv_fs_dir_t directory;
    lv_fs_res_t read_result;

    if(backend == NULL) return BROWSER_BACKEND_INVALID_ARG;
    backend->window_count = 0;
    backend->total_count = 0;
    if(lv_fs_dir_open(&directory, backend->current_path) != LV_FS_RES_OK)
        return BROWSER_BACKEND_OPEN_FAILED;

    while(true) {
        read_result = lv_fs_dir_read(&directory, backend->scratch, sizeof(backend->scratch));
        if(read_result != LV_FS_RES_OK) {
            lv_fs_dir_close(&directory);
            backend->window_count = 0u;
            backend->total_count = 0u;
            return BROWSER_BACKEND_READ_FAILED;
        }
        if(backend->scratch[0] == '\0') break;
        {
        bool is_directory = backend->scratch[0] == '/';
        const char *name = is_directory ? backend->scratch + 1 : backend->scratch;
        uint32_t logical_index;
        browser_backend_entry_t *entry;

        if(!entry_visible(backend, name, is_directory)) continue;
        logical_index = backend->total_count++;
        if(logical_index < backend->window_offset ||
           backend->window_count >= BROWSER_BACKEND_WINDOW_SIZE) continue;
        entry = &backend->entries[backend->window_count++];
        lv_snprintf(entry->name, sizeof(entry->name), "%s", name);
        entry->is_directory = is_directory;
        }
    }
    lv_fs_dir_close(&directory);

    /* 目录缩短或窗口靠近末尾时向前回填，让末端窗口尽量保持完整。 */
    if(backend->total_count == 0u) {
        backend->window_offset = 0u;
    }
    else {
        uint32_t last_offset = backend->total_count > BROWSER_BACKEND_WINDOW_SIZE ?
                               backend->total_count - BROWSER_BACKEND_WINDOW_SIZE : 0u;
        if(backend->window_offset > last_offset) {
            backend->window_offset = last_offset;
            return browser_backend_reload(backend);
        }
    }
    return BROWSER_BACKEND_OK;
}

browser_backend_result_t browser_backend_load_window(browser_backend_t *backend,
                                                      uint32_t offset)
{
    uint32_t last_offset;

    if(backend == NULL) return BROWSER_BACKEND_INVALID_ARG;
    last_offset = backend->total_count > BROWSER_BACKEND_WINDOW_SIZE ?
                  backend->total_count - BROWSER_BACKEND_WINDOW_SIZE : 0u;
    backend->window_offset = offset < last_offset ? offset : last_offset;
    return browser_backend_reload(backend);
}
browser_backend_result_t browser_backend_enter(browser_backend_t *backend,
                                                const char *directory_name)
{
    uint32_t length;
    browser_backend_result_t result;
    const char *p;

    if(backend == NULL || directory_name == NULL || directory_name[0] == '\0')
        return BROWSER_BACKEND_INVALID_ARG;
    if(!backend->allow_directories || lv_strcmp(directory_name, ".") == 0 ||
       lv_strcmp(directory_name, "..") == 0) return BROWSER_BACKEND_NOT_DIRECTORY;
    for(p = directory_name; *p != '\0'; p++)
        if(*p == '/' || *p == '\\' || *p == ':') return BROWSER_BACKEND_INVALID_ARG;

    length = (uint32_t)lv_strlen(backend->current_path);
    if(length + lv_strlen(directory_name) + 2u > sizeof(backend->current_path))
        return BROWSER_BACKEND_PATH_TOO_LONG;
    lv_snprintf(backend->current_path + length, sizeof(backend->current_path) - length,
                "%s/", directory_name);
    backend->window_offset = 0u;
    result = browser_backend_reload(backend);
    if(result != BROWSER_BACKEND_OK) {
        backend->current_path[length] = '\0';
        (void)browser_backend_reload(backend);
    }
    return result;
}

bool browser_backend_can_go_up(const browser_backend_t *backend)
{
    return backend != NULL && lv_strcmp(backend->current_path, backend->root_path) != 0;
}

browser_backend_result_t browser_backend_up(browser_backend_t *backend)
{
    uint32_t length;
    if(backend == NULL) return BROWSER_BACKEND_INVALID_ARG;
    if(!browser_backend_can_go_up(backend)) return BROWSER_BACKEND_OK;
    length = (uint32_t)lv_strlen(backend->current_path);
    if(length > 0u) length--;
    while(length > 0u && backend->current_path[length - 1u] != '/') length--;
    if(length < lv_strlen(backend->root_path)) length = (uint32_t)lv_strlen(backend->root_path);
    backend->current_path[length] = '\0';
    backend->window_offset = 0u;
    return browser_backend_reload(backend);
}

const browser_backend_entry_t *browser_backend_entry(const browser_backend_t *backend, uint32_t index)
{
    return backend != NULL && index < backend->window_count ? &backend->entries[index] : NULL;
}

const char *browser_backend_current_path(const browser_backend_t *backend)
{
    return backend != NULL ? backend->current_path : "";
}

uint32_t browser_backend_window_count(const browser_backend_t *backend)
{
    return backend != NULL ? backend->window_count : 0u;
}

uint32_t browser_backend_window_offset(const browser_backend_t *backend)
{
    return backend != NULL ? backend->window_offset : 0u;
}

uint32_t browser_backend_total_count(const browser_backend_t *backend)
{
    return backend != NULL ? backend->total_count : 0u;
}

browser_backend_result_t browser_backend_full_path(const browser_backend_t *backend, uint32_t index,
                                                   char *buffer, uint32_t buffer_size)
{
    const browser_backend_entry_t *entry = browser_backend_entry(backend, index);
    uint32_t required;
    if(entry == NULL || buffer == NULL || buffer_size == 0u) return BROWSER_BACKEND_INVALID_ARG;
    required = (uint32_t)lv_strlen(backend->current_path) + (uint32_t)lv_strlen(entry->name) +
               (entry->is_directory ? 2u : 1u);
    if(required > buffer_size) return BROWSER_BACKEND_PATH_TOO_LONG;
    lv_snprintf(buffer, buffer_size, entry->is_directory ? "%s%s/" : "%s%s",
                backend->current_path, entry->name);
    return BROWSER_BACKEND_OK;
}
