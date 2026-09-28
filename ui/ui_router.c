#include "ui_router.h"
#include "../H7_MediaPlayer_UI_gen.h"

static ui_route_bind_cb_t binder;
static ui_route_id_t current_route;
static bool switching;
static bool forward_direction = true;

static lv_obj_t * create_route(ui_route_id_t route)
{
    switch(route) {
        case UI_ROUTE_LAUNCHER: return launcher_create();
        case UI_ROUTE_SETTINGS: return settings_create();
        case UI_ROUTE_ABOUT: return about_create();
        case UI_ROUTE_APP_PLACEHOLDER: return app_placeholder_create();
        case UI_ROUTE_PLAYER: return player_create();
        case UI_ROUTE_BROWSER: return browser_create();
        case UI_ROUTE_SYSTEM_MONITOR: return system_monitor_create();
        default: return home_create();
    }
}

static void transition_done_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    switching = false;
}

void ui_router_init(ui_route_bind_cb_t bind_cb, ui_route_id_t initial_route)
{
    binder = bind_cb;
    current_route = initial_route;
    switching = false;
    forward_direction = true;
}

void ui_router_open(ui_route_id_t route, bool forward)
{
    lv_obj_t * next;

    if(switching || route == current_route) return;
    next = create_route(route);
    if(next == NULL) return;

    current_route = route;
    forward_direction = forward;
    switching = true;
    if(binder != NULL) binder(next);
    lv_obj_add_event_cb(lv_screen_active(), transition_done_cb, LV_EVENT_SCREEN_UNLOADED, NULL);
    lv_screen_load_anim(next, LV_SCR_LOAD_ANIM_FADE_ON, 300, 0, true);
}

ui_route_id_t ui_router_current(void)
{
    return current_route;
}

bool ui_router_forward(void)
{
    return forward_direction;
}
