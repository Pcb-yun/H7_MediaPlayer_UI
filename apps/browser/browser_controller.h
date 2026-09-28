#ifndef H7_BROWSER_CONTROLLER_H
#define H7_BROWSER_CONTROLLER_H

#include "browser_backend.h"
#include "../../ui/ui_router.h"

typedef void (*browser_controller_select_cb_t)(const char *full_path, void *user_data);

typedef struct {
    const char *root_path;
    const char *extensions;
    bool allow_directories;
    bool allow_delete;
    browser_backend_filter_cb_t filter_cb;
    browser_controller_select_cb_t select_cb;
    void *user_data;
    ui_route_id_t return_route;
} browser_controller_request_t;

bool browser_controller_open(const browser_controller_request_t *request);
void browser_controller_bind(lv_obj_t *screen);

#endif
