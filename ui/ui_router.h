#ifndef H7_UI_ROUTER_H
#define H7_UI_ROUTER_H

#include "lvgl.h"

typedef enum {
    UI_ROUTE_HOME,
    UI_ROUTE_LAUNCHER,
    UI_ROUTE_SETTINGS,
    UI_ROUTE_ABOUT,
    UI_ROUTE_APP_PLACEHOLDER,
    UI_ROUTE_PLAYER,
    UI_ROUTE_BROWSER,
    UI_ROUTE_SYSTEM_MONITOR,
} ui_route_id_t;

typedef void (*ui_route_bind_cb_t)(lv_obj_t * screen);

void ui_router_init(ui_route_bind_cb_t bind_cb, ui_route_id_t initial_route);
void ui_router_open(ui_route_id_t route, bool forward);
ui_route_id_t ui_router_current(void);
bool ui_router_forward(void);

#endif
