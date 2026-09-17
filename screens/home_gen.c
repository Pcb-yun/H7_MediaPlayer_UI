/**
 * @file home_gen.c
 * @brief Template source file for LVGL objects
 */

/*********************
 *      INCLUDES
 *********************/

#include "home_gen.h"
#include "../H7_MediaPlayer_UI.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/***********************
 *  STATIC VARIABLES
 **********************/

/***********************
 *  STATIC PROTOTYPES
 **********************/

static lv_anim_timeline_t * timeline_timeline_spin_create(lv_obj_t * obj);
static void free_timeline_event_cb(lv_event_t * e);

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t * home_create(void)
{
    LV_TRACE_OBJ_CREATE("begin");

    static lv_style_t style_box;
    static lv_style_t style_dot;

    static bool style_inited = false;

    if (!style_inited) {
        /*Init all styles*/
        lv_style_init(&style_box);
        lv_style_init(&style_dot);

        lv_style_set_width(&style_box, lv_pct(80));
        lv_style_set_height(&style_box, LV_SIZE_CONTENT);
        lv_style_set_flex_flow(&style_box, LV_FLEX_FLOW_ROW);
        lv_style_set_flex_main_place(&style_box, LV_FLEX_ALIGN_CENTER);
        lv_style_set_flex_cross_place(&style_box, LV_FLEX_ALIGN_CENTER);
        lv_style_set_flex_track_place(&style_box, LV_FLEX_ALIGN_CENTER);
        lv_style_set_pad_all(&style_box, 10);
        lv_style_set_radius(&style_box, 10);
        lv_style_set_bg_color(&style_box, lv_color_hex(0xf1f5f9));
        lv_style_set_border_width(&style_box, 2);
        lv_style_set_border_color(&style_box, lv_color_hex(0x3b82f6));
        lv_style_set_radius(&style_dot, 4);
        lv_style_set_bg_color(&style_dot, lv_color_hex(0x9ca3af));
        lv_style_set_bg_opa(&style_dot, (255 * 100 / 100));

        style_inited = true;
    }


    lv_obj_t * the_root = NULL;

    #if H7_MEDIAPLAYER_UI_CHECK_COMPILE_TARGET(H7_MEDIAPLAYER_UI_TARGET_ALL)
    if (H7_MediaPlayer_UI_check_target(H7_MEDIAPLAYER_UI_TARGET_ALL)) {
        lv_obj_t * lv_obj_0 = lv_obj_create(NULL);
        lv_obj_set_name_static(lv_obj_0, "home_#");
        lv_obj_set_flex_flow(lv_obj_0, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_flex_main_place(lv_obj_0, LV_FLEX_ALIGN_CENTER, 0);
        lv_obj_set_style_flex_cross_place(lv_obj_0, LV_FLEX_ALIGN_CENTER, 0);
        lv_obj_set_style_flex_track_place(lv_obj_0, LV_FLEX_ALIGN_CENTER, 0);
        lv_obj_set_style_pad_all(lv_obj_0, 12, 0);
        lv_obj_set_style_pad_row(lv_obj_0, 10, 0);
        lv_obj_set_style_bg_color(lv_obj_0, lv_color_hex(0xffffff), 0);
        lv_obj_set_style_bg_opa(lv_obj_0, (255 * 100 / 100), 0);

        lv_obj_t * lv_obj_1 = lv_obj_create(lv_obj_0);
        lv_obj_add_style(lv_obj_1, &style_box, 0);
        lv_obj_t * lv_label_0 = lv_label_create(lv_obj_1);
        lv_label_set_text(lv_label_0, "H7_MediaPlayer");
        lv_obj_set_style_text_color(lv_label_0, lv_color_hex(0x000000), 0);

        lv_obj_t * lv_obj_2 = lv_obj_create(lv_obj_0);
        lv_obj_add_style(lv_obj_2, &style_box, 0);
        lv_obj_t * lv_label_1 = lv_label_create(lv_obj_2);
        lv_label_set_text(lv_label_1, "Hello LVGL editor");
        lv_obj_set_style_text_color(lv_label_1, lv_color_hex(0x0000ff), 0);

        lv_obj_t * lv_obj_3 = lv_obj_create(lv_obj_0);
        lv_obj_set_flex_grow(lv_obj_3, 1);
        lv_obj_set_width(lv_obj_3, lv_pct(100));
        lv_obj_set_style_bg_opa(lv_obj_3, 0, 0);
        lv_obj_t * dot_0 = lv_obj_create(lv_obj_3);
        lv_obj_set_name(dot_0, "dot_0");
        lv_obj_set_width(dot_0, 64);
        lv_obj_set_height(dot_0, 64);
        lv_obj_set_align(dot_0, LV_ALIGN_CENTER);
        lv_obj_set_style_bg_opa(dot_0, 0, 0);
        lv_obj_set_style_transform_pivot_x(dot_0, 32, 0);
        lv_obj_set_style_transform_pivot_y(dot_0, 32, 0);
        lv_obj_t * lv_obj_4 = lv_obj_create(dot_0);
        lv_obj_set_width(lv_obj_4, 7);
        lv_obj_set_height(lv_obj_4, 7);
        lv_obj_set_align(lv_obj_4, LV_ALIGN_CENTER);
        lv_obj_set_y(lv_obj_4, -26);
        lv_obj_add_style(lv_obj_4, &style_dot, 0);

        lv_obj_t * dot_1 = lv_obj_create(lv_obj_3);
        lv_obj_set_name(dot_1, "dot_1");
        lv_obj_set_width(dot_1, 64);
        lv_obj_set_height(dot_1, 64);
        lv_obj_set_align(dot_1, LV_ALIGN_CENTER);
        lv_obj_set_style_bg_opa(dot_1, 0, 0);
        lv_obj_set_style_transform_pivot_x(dot_1, 32, 0);
        lv_obj_set_style_transform_pivot_y(dot_1, 32, 0);
        lv_obj_t * lv_obj_5 = lv_obj_create(dot_1);
        lv_obj_set_width(lv_obj_5, 6);
        lv_obj_set_height(lv_obj_5, 6);
        lv_obj_set_align(lv_obj_5, LV_ALIGN_CENTER);
        lv_obj_set_y(lv_obj_5, -26);
        lv_obj_add_style(lv_obj_5, &style_dot, 0);

        lv_obj_t * dot_2 = lv_obj_create(lv_obj_3);
        lv_obj_set_name(dot_2, "dot_2");
        lv_obj_set_width(dot_2, 64);
        lv_obj_set_height(dot_2, 64);
        lv_obj_set_align(dot_2, LV_ALIGN_CENTER);
        lv_obj_set_style_bg_opa(dot_2, 0, 0);
        lv_obj_set_style_transform_pivot_x(dot_2, 32, 0);
        lv_obj_set_style_transform_pivot_y(dot_2, 32, 0);
        lv_obj_t * lv_obj_6 = lv_obj_create(dot_2);
        lv_obj_set_width(lv_obj_6, 5);
        lv_obj_set_height(lv_obj_6, 5);
        lv_obj_set_align(lv_obj_6, LV_ALIGN_CENTER);
        lv_obj_set_y(lv_obj_6, -26);
        lv_obj_add_style(lv_obj_6, &style_dot, 0);

        lv_obj_t * dot_3 = lv_obj_create(lv_obj_3);
        lv_obj_set_name(dot_3, "dot_3");
        lv_obj_set_width(dot_3, 64);
        lv_obj_set_height(dot_3, 64);
        lv_obj_set_align(dot_3, LV_ALIGN_CENTER);
        lv_obj_set_style_bg_opa(dot_3, 0, 0);
        lv_obj_set_style_transform_pivot_x(dot_3, 32, 0);
        lv_obj_set_style_transform_pivot_y(dot_3, 32, 0);
        lv_obj_t * lv_obj_7 = lv_obj_create(dot_3);
        lv_obj_set_width(lv_obj_7, 4);
        lv_obj_set_height(lv_obj_7, 4);
        lv_obj_set_align(lv_obj_7, LV_ALIGN_CENTER);
        lv_obj_set_y(lv_obj_7, -26);
        lv_obj_add_style(lv_obj_7, &style_dot, 0);

        lv_obj_t * dot_4 = lv_obj_create(lv_obj_3);
        lv_obj_set_name(dot_4, "dot_4");
        lv_obj_set_width(dot_4, 64);
        lv_obj_set_height(dot_4, 64);
        lv_obj_set_align(dot_4, LV_ALIGN_CENTER);
        lv_obj_set_style_bg_opa(dot_4, 0, 0);
        lv_obj_set_style_transform_pivot_x(dot_4, 32, 0);
        lv_obj_set_style_transform_pivot_y(dot_4, 32, 0);
        lv_obj_t * lv_obj_8 = lv_obj_create(dot_4);
        lv_obj_set_width(lv_obj_8, 3);
        lv_obj_set_height(lv_obj_8, 3);
        lv_obj_set_align(lv_obj_8, LV_ALIGN_CENTER);
        lv_obj_set_y(lv_obj_8, -26);
        lv_obj_add_style(lv_obj_8, &style_dot, 0);

        lv_obj_t * lv_obj_9 = lv_obj_create(lv_obj_0);
        lv_obj_set_width(lv_obj_9, 200);
        lv_obj_set_height(lv_obj_9, 6);
        lv_obj_set_style_radius(lv_obj_9, 3, 0);
        lv_obj_set_style_bg_color(lv_obj_9, lv_color_hex(0xe5e7eb), 0);
        lv_obj_set_style_bg_opa(lv_obj_9, (255 * 100 / 100), 0);
        lv_obj_t * bar_fill = lv_obj_create(lv_obj_9);
        lv_obj_set_name(bar_fill, "bar_fill");
        lv_obj_set_width(bar_fill, 0);
        lv_obj_set_height(bar_fill, 4);
        lv_obj_set_style_radius(bar_fill, 2, 0);
        lv_obj_set_align(bar_fill, LV_ALIGN_LEFT_MID);
        lv_obj_set_style_bg_color(bar_fill, lv_color_hex(0x3b82f6), 0);
        lv_obj_set_style_bg_opa(bar_fill, (255 * 100 / 100), 0);


        /* create animation timeline(s) */
        lv_anim_timeline_t ** at_array = lv_malloc(sizeof(lv_anim_timeline_t *) * _HOME_TIMELINE_CNT);
        at_array[HOME_TIMELINE_TIMELINE_SPIN] = timeline_timeline_spin_create(lv_obj_0);
        lv_obj_set_user_data(lv_obj_0, at_array);
        lv_obj_add_event_cb(lv_obj_0, free_timeline_event_cb, LV_EVENT_DELETE, at_array);

        lv_obj_add_play_timeline_event(lv_obj_0, LV_EVENT_SCREEN_LOADED, home_get_timeline(lv_obj_0, HOME_TIMELINE_TIMELINE_SPIN), 0, false);

        the_root = lv_obj_0;
    }
    #endif

    LV_TRACE_OBJ_CREATE("finished");

    return the_root;
}

lv_anim_timeline_t * home_get_timeline(lv_obj_t * obj, home_timeline_t timeline_id)
{
    if (timeline_id >= _HOME_TIMELINE_CNT) {
        LV_LOG_WARN("home has no timeline with %d ID", timeline_id);
        return NULL;
    }

    lv_anim_timeline_t ** at_array = lv_obj_get_user_data(obj);
    return at_array[timeline_id];
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/* Helper function to execute animations */
static void int_anim_exec_cb(lv_anim_t * a, int32_t v)
{
    uint32_t data = (lv_uintptr_t)lv_anim_get_user_data(a);
    lv_style_prop_t prop = data >> 24;
    lv_style_selector_t selector = data & 0x00ffffff;

    lv_style_value_t style_value;
    style_value.num = v;
    lv_obj_set_local_style_prop(a->var, prop, style_value, selector);
}

static lv_anim_timeline_t * timeline_timeline_spin_create(lv_obj_t * obj)
{
    lv_anim_timeline_t * at = lv_anim_timeline_create();
    lv_anim_timeline_set_repeat_count(at, LV_ANIM_REPEAT_INFINITE);
    lv_anim_timeline_t * at_to_merge = NULL;

    lv_anim_t a;
    uint32_t selector_and_prop;

    selector_and_prop = ((LV_STYLE_TRANSFORM_ROTATION & 0xff) << 24) | 0;
    lv_anim_init(&a);
    lv_anim_set_custom_exec_cb(&a, int_anim_exec_cb);
    lv_anim_set_var(&a, lv_obj_find_by_name(obj, "dot_0"));
    lv_anim_set_values(&a, 0, 1200);
    lv_anim_set_duration(&a, 270);
    lv_anim_set_user_data(&a, (void *)((uintptr_t)selector_and_prop));
    lv_anim_set_early_apply(&a, false);
    lv_anim_timeline_add(at, 0, &a);

    selector_and_prop = ((LV_STYLE_TRANSFORM_ROTATION & 0xff) << 24) | 0;
    lv_anim_init(&a);
    lv_anim_set_custom_exec_cb(&a, int_anim_exec_cb);
    lv_anim_set_var(&a, lv_obj_find_by_name(obj, "dot_0"));
    lv_anim_set_values(&a, 1200, 2400);
    lv_anim_set_duration(&a, 1200);
    lv_anim_set_user_data(&a, (void *)((uintptr_t)selector_and_prop));
    lv_anim_set_early_apply(&a, false);
    lv_anim_timeline_add(at, 270, &a);

    selector_and_prop = ((LV_STYLE_TRANSFORM_ROTATION & 0xff) << 24) | 0;
    lv_anim_init(&a);
    lv_anim_set_custom_exec_cb(&a, int_anim_exec_cb);
    lv_anim_set_var(&a, lv_obj_find_by_name(obj, "dot_0"));
    lv_anim_set_values(&a, 2400, 3600);
    lv_anim_set_duration(&a, 270);
    lv_anim_set_user_data(&a, (void *)((uintptr_t)selector_and_prop));
    lv_anim_set_early_apply(&a, false);
    lv_anim_timeline_add(at, 1470, &a);

    selector_and_prop = ((LV_STYLE_TRANSFORM_ROTATION & 0xff) << 24) | 0;
    lv_anim_init(&a);
    lv_anim_set_custom_exec_cb(&a, int_anim_exec_cb);
    lv_anim_set_var(&a, lv_obj_find_by_name(obj, "dot_0"));
    lv_anim_set_values(&a, 3600, 4800);
    lv_anim_set_duration(&a, 270);
    lv_anim_set_user_data(&a, (void *)((uintptr_t)selector_and_prop));
    lv_anim_set_early_apply(&a, false);
    lv_anim_timeline_add(at, 1740, &a);

    selector_and_prop = ((LV_STYLE_TRANSFORM_ROTATION & 0xff) << 24) | 0;
    lv_anim_init(&a);
    lv_anim_set_custom_exec_cb(&a, int_anim_exec_cb);
    lv_anim_set_var(&a, lv_obj_find_by_name(obj, "dot_0"));
    lv_anim_set_values(&a, 4800, 6000);
    lv_anim_set_duration(&a, 1200);
    lv_anim_set_user_data(&a, (void *)((uintptr_t)selector_and_prop));
    lv_anim_set_early_apply(&a, false);
    lv_anim_timeline_add(at, 2010, &a);

    selector_and_prop = ((LV_STYLE_TRANSFORM_ROTATION & 0xff) << 24) | 0;
    lv_anim_init(&a);
    lv_anim_set_custom_exec_cb(&a, int_anim_exec_cb);
    lv_anim_set_var(&a, lv_obj_find_by_name(obj, "dot_0"));
    lv_anim_set_values(&a, 6000, 7200);
    lv_anim_set_duration(&a, 270);
    lv_anim_set_user_data(&a, (void *)((uintptr_t)selector_and_prop));
    lv_anim_set_early_apply(&a, false);
    lv_anim_timeline_add(at, 3210, &a);

    selector_and_prop = ((LV_STYLE_TRANSFORM_ROTATION & 0xff) << 24) | 0;
    lv_anim_init(&a);
    lv_anim_set_custom_exec_cb(&a, int_anim_exec_cb);
    lv_anim_set_var(&a, lv_obj_find_by_name(obj, "dot_1"));
    lv_anim_set_values(&a, -800, 0);
    lv_anim_set_duration(&a, 180);
    lv_anim_set_user_data(&a, (void *)((uintptr_t)selector_and_prop));
    lv_anim_set_early_apply(&a, false);
    lv_anim_timeline_add(at, 0, &a);

    selector_and_prop = ((LV_STYLE_TRANSFORM_ROTATION & 0xff) << 24) | 0;
    lv_anim_init(&a);
    lv_anim_set_custom_exec_cb(&a, int_anim_exec_cb);
    lv_anim_set_var(&a, lv_obj_find_by_name(obj, "dot_1"));
    lv_anim_set_values(&a, 0, 1200);
    lv_anim_set_duration(&a, 270);
    lv_anim_set_user_data(&a, (void *)((uintptr_t)selector_and_prop));
    lv_anim_set_early_apply(&a, false);
    lv_anim_timeline_add(at, 180, &a);

    selector_and_prop = ((LV_STYLE_TRANSFORM_ROTATION & 0xff) << 24) | 0;
    lv_anim_init(&a);
    lv_anim_set_custom_exec_cb(&a, int_anim_exec_cb);
    lv_anim_set_var(&a, lv_obj_find_by_name(obj, "dot_1"));
    lv_anim_set_values(&a, 1200, 2400);
    lv_anim_set_duration(&a, 1200);
    lv_anim_set_user_data(&a, (void *)((uintptr_t)selector_and_prop));
    lv_anim_set_early_apply(&a, false);
    lv_anim_timeline_add(at, 450, &a);

    selector_and_prop = ((LV_STYLE_TRANSFORM_ROTATION & 0xff) << 24) | 0;
    lv_anim_init(&a);
    lv_anim_set_custom_exec_cb(&a, int_anim_exec_cb);
    lv_anim_set_var(&a, lv_obj_find_by_name(obj, "dot_1"));
    lv_anim_set_values(&a, 2400, 3600);
    lv_anim_set_duration(&a, 270);
    lv_anim_set_user_data(&a, (void *)((uintptr_t)selector_and_prop));
    lv_anim_set_early_apply(&a, false);
    lv_anim_timeline_add(at, 1650, &a);

    selector_and_prop = ((LV_STYLE_TRANSFORM_ROTATION & 0xff) << 24) | 0;
    lv_anim_init(&a);
    lv_anim_set_custom_exec_cb(&a, int_anim_exec_cb);
    lv_anim_set_var(&a, lv_obj_find_by_name(obj, "dot_1"));
    lv_anim_set_values(&a, 3600, 4800);
    lv_anim_set_duration(&a, 270);
    lv_anim_set_user_data(&a, (void *)((uintptr_t)selector_and_prop));
    lv_anim_set_early_apply(&a, false);
    lv_anim_timeline_add(at, 1920, &a);

    selector_and_prop = ((LV_STYLE_TRANSFORM_ROTATION & 0xff) << 24) | 0;
    lv_anim_init(&a);
    lv_anim_set_custom_exec_cb(&a, int_anim_exec_cb);
    lv_anim_set_var(&a, lv_obj_find_by_name(obj, "dot_1"));
    lv_anim_set_values(&a, 4800, 6000);
    lv_anim_set_duration(&a, 1200);
    lv_anim_set_user_data(&a, (void *)((uintptr_t)selector_and_prop));
    lv_anim_set_early_apply(&a, false);
    lv_anim_timeline_add(at, 2190, &a);

    selector_and_prop = ((LV_STYLE_TRANSFORM_ROTATION & 0xff) << 24) | 0;
    lv_anim_init(&a);
    lv_anim_set_custom_exec_cb(&a, int_anim_exec_cb);
    lv_anim_set_var(&a, lv_obj_find_by_name(obj, "dot_1"));
    lv_anim_set_values(&a, 6000, 6400);
    lv_anim_set_duration(&a, 90);
    lv_anim_set_user_data(&a, (void *)((uintptr_t)selector_and_prop));
    lv_anim_set_early_apply(&a, false);
    lv_anim_timeline_add(at, 3390, &a);

    selector_and_prop = ((LV_STYLE_TRANSFORM_ROTATION & 0xff) << 24) | 0;
    lv_anim_init(&a);
    lv_anim_set_custom_exec_cb(&a, int_anim_exec_cb);
    lv_anim_set_var(&a, lv_obj_find_by_name(obj, "dot_2"));
    lv_anim_set_values(&a, -1290, -1200);
    lv_anim_set_duration(&a, 90);
    lv_anim_set_user_data(&a, (void *)((uintptr_t)selector_and_prop));
    lv_anim_set_early_apply(&a, false);
    lv_anim_timeline_add(at, 0, &a);

    selector_and_prop = ((LV_STYLE_TRANSFORM_ROTATION & 0xff) << 24) | 0;
    lv_anim_init(&a);
    lv_anim_set_custom_exec_cb(&a, int_anim_exec_cb);
    lv_anim_set_var(&a, lv_obj_find_by_name(obj, "dot_2"));
    lv_anim_set_values(&a, -1200, 0);
    lv_anim_set_duration(&a, 270);
    lv_anim_set_user_data(&a, (void *)((uintptr_t)selector_and_prop));
    lv_anim_set_early_apply(&a, false);
    lv_anim_timeline_add(at, 90, &a);

    selector_and_prop = ((LV_STYLE_TRANSFORM_ROTATION & 0xff) << 24) | 0;
    lv_anim_init(&a);
    lv_anim_set_custom_exec_cb(&a, int_anim_exec_cb);
    lv_anim_set_var(&a, lv_obj_find_by_name(obj, "dot_2"));
    lv_anim_set_values(&a, 0, 1200);
    lv_anim_set_duration(&a, 270);
    lv_anim_set_user_data(&a, (void *)((uintptr_t)selector_and_prop));
    lv_anim_set_early_apply(&a, false);
    lv_anim_timeline_add(at, 360, &a);

    selector_and_prop = ((LV_STYLE_TRANSFORM_ROTATION & 0xff) << 24) | 0;
    lv_anim_init(&a);
    lv_anim_set_custom_exec_cb(&a, int_anim_exec_cb);
    lv_anim_set_var(&a, lv_obj_find_by_name(obj, "dot_2"));
    lv_anim_set_values(&a, 1200, 2400);
    lv_anim_set_duration(&a, 1200);
    lv_anim_set_user_data(&a, (void *)((uintptr_t)selector_and_prop));
    lv_anim_set_early_apply(&a, false);
    lv_anim_timeline_add(at, 630, &a);

    selector_and_prop = ((LV_STYLE_TRANSFORM_ROTATION & 0xff) << 24) | 0;
    lv_anim_init(&a);
    lv_anim_set_custom_exec_cb(&a, int_anim_exec_cb);
    lv_anim_set_var(&a, lv_obj_find_by_name(obj, "dot_2"));
    lv_anim_set_values(&a, 2400, 3600);
    lv_anim_set_duration(&a, 270);
    lv_anim_set_user_data(&a, (void *)((uintptr_t)selector_and_prop));
    lv_anim_set_early_apply(&a, false);
    lv_anim_timeline_add(at, 1830, &a);

    selector_and_prop = ((LV_STYLE_TRANSFORM_ROTATION & 0xff) << 24) | 0;
    lv_anim_init(&a);
    lv_anim_set_custom_exec_cb(&a, int_anim_exec_cb);
    lv_anim_set_var(&a, lv_obj_find_by_name(obj, "dot_2"));
    lv_anim_set_values(&a, 3600, 4800);
    lv_anim_set_duration(&a, 270);
    lv_anim_set_user_data(&a, (void *)((uintptr_t)selector_and_prop));
    lv_anim_set_early_apply(&a, false);
    lv_anim_timeline_add(at, 2100, &a);

    selector_and_prop = ((LV_STYLE_TRANSFORM_ROTATION & 0xff) << 24) | 0;
    lv_anim_init(&a);
    lv_anim_set_custom_exec_cb(&a, int_anim_exec_cb);
    lv_anim_set_var(&a, lv_obj_find_by_name(obj, "dot_2"));
    lv_anim_set_values(&a, 4800, 5910);
    lv_anim_set_duration(&a, 1110);
    lv_anim_set_user_data(&a, (void *)((uintptr_t)selector_and_prop));
    lv_anim_set_early_apply(&a, false);
    lv_anim_timeline_add(at, 2370, &a);

    selector_and_prop = ((LV_STYLE_TRANSFORM_ROTATION & 0xff) << 24) | 0;
    lv_anim_init(&a);
    lv_anim_set_custom_exec_cb(&a, int_anim_exec_cb);
    lv_anim_set_var(&a, lv_obj_find_by_name(obj, "dot_3"));
    lv_anim_set_values(&a, -1470, -1200);
    lv_anim_set_duration(&a, 270);
    lv_anim_set_user_data(&a, (void *)((uintptr_t)selector_and_prop));
    lv_anim_set_early_apply(&a, false);
    lv_anim_timeline_add(at, 0, &a);

    selector_and_prop = ((LV_STYLE_TRANSFORM_ROTATION & 0xff) << 24) | 0;
    lv_anim_init(&a);
    lv_anim_set_custom_exec_cb(&a, int_anim_exec_cb);
    lv_anim_set_var(&a, lv_obj_find_by_name(obj, "dot_3"));
    lv_anim_set_values(&a, -1200, 0);
    lv_anim_set_duration(&a, 270);
    lv_anim_set_user_data(&a, (void *)((uintptr_t)selector_and_prop));
    lv_anim_set_early_apply(&a, false);
    lv_anim_timeline_add(at, 270, &a);

    selector_and_prop = ((LV_STYLE_TRANSFORM_ROTATION & 0xff) << 24) | 0;
    lv_anim_init(&a);
    lv_anim_set_custom_exec_cb(&a, int_anim_exec_cb);
    lv_anim_set_var(&a, lv_obj_find_by_name(obj, "dot_3"));
    lv_anim_set_values(&a, 0, 1200);
    lv_anim_set_duration(&a, 270);
    lv_anim_set_user_data(&a, (void *)((uintptr_t)selector_and_prop));
    lv_anim_set_early_apply(&a, false);
    lv_anim_timeline_add(at, 540, &a);

    selector_and_prop = ((LV_STYLE_TRANSFORM_ROTATION & 0xff) << 24) | 0;
    lv_anim_init(&a);
    lv_anim_set_custom_exec_cb(&a, int_anim_exec_cb);
    lv_anim_set_var(&a, lv_obj_find_by_name(obj, "dot_3"));
    lv_anim_set_values(&a, 1200, 2400);
    lv_anim_set_duration(&a, 1200);
    lv_anim_set_user_data(&a, (void *)((uintptr_t)selector_and_prop));
    lv_anim_set_early_apply(&a, false);
    lv_anim_timeline_add(at, 810, &a);

    selector_and_prop = ((LV_STYLE_TRANSFORM_ROTATION & 0xff) << 24) | 0;
    lv_anim_init(&a);
    lv_anim_set_custom_exec_cb(&a, int_anim_exec_cb);
    lv_anim_set_var(&a, lv_obj_find_by_name(obj, "dot_3"));
    lv_anim_set_values(&a, 2400, 3600);
    lv_anim_set_duration(&a, 270);
    lv_anim_set_user_data(&a, (void *)((uintptr_t)selector_and_prop));
    lv_anim_set_early_apply(&a, false);
    lv_anim_timeline_add(at, 2010, &a);

    selector_and_prop = ((LV_STYLE_TRANSFORM_ROTATION & 0xff) << 24) | 0;
    lv_anim_init(&a);
    lv_anim_set_custom_exec_cb(&a, int_anim_exec_cb);
    lv_anim_set_var(&a, lv_obj_find_by_name(obj, "dot_3"));
    lv_anim_set_values(&a, 3600, 4800);
    lv_anim_set_duration(&a, 270);
    lv_anim_set_user_data(&a, (void *)((uintptr_t)selector_and_prop));
    lv_anim_set_early_apply(&a, false);
    lv_anim_timeline_add(at, 2280, &a);

    selector_and_prop = ((LV_STYLE_TRANSFORM_ROTATION & 0xff) << 24) | 0;
    lv_anim_init(&a);
    lv_anim_set_custom_exec_cb(&a, int_anim_exec_cb);
    lv_anim_set_var(&a, lv_obj_find_by_name(obj, "dot_3"));
    lv_anim_set_values(&a, 4800, 5730);
    lv_anim_set_duration(&a, 930);
    lv_anim_set_user_data(&a, (void *)((uintptr_t)selector_and_prop));
    lv_anim_set_early_apply(&a, false);
    lv_anim_timeline_add(at, 2550, &a);

    selector_and_prop = ((LV_STYLE_TRANSFORM_ROTATION & 0xff) << 24) | 0;
    lv_anim_init(&a);
    lv_anim_set_custom_exec_cb(&a, int_anim_exec_cb);
    lv_anim_set_var(&a, lv_obj_find_by_name(obj, "dot_4"));
    lv_anim_set_values(&a, -1650, -1200);
    lv_anim_set_duration(&a, 450);
    lv_anim_set_user_data(&a, (void *)((uintptr_t)selector_and_prop));
    lv_anim_set_early_apply(&a, false);
    lv_anim_timeline_add(at, 0, &a);

    selector_and_prop = ((LV_STYLE_TRANSFORM_ROTATION & 0xff) << 24) | 0;
    lv_anim_init(&a);
    lv_anim_set_custom_exec_cb(&a, int_anim_exec_cb);
    lv_anim_set_var(&a, lv_obj_find_by_name(obj, "dot_4"));
    lv_anim_set_values(&a, -1200, 0);
    lv_anim_set_duration(&a, 270);
    lv_anim_set_user_data(&a, (void *)((uintptr_t)selector_and_prop));
    lv_anim_set_early_apply(&a, false);
    lv_anim_timeline_add(at, 450, &a);

    selector_and_prop = ((LV_STYLE_TRANSFORM_ROTATION & 0xff) << 24) | 0;
    lv_anim_init(&a);
    lv_anim_set_custom_exec_cb(&a, int_anim_exec_cb);
    lv_anim_set_var(&a, lv_obj_find_by_name(obj, "dot_4"));
    lv_anim_set_values(&a, 0, 1200);
    lv_anim_set_duration(&a, 270);
    lv_anim_set_user_data(&a, (void *)((uintptr_t)selector_and_prop));
    lv_anim_set_early_apply(&a, false);
    lv_anim_timeline_add(at, 720, &a);

    selector_and_prop = ((LV_STYLE_TRANSFORM_ROTATION & 0xff) << 24) | 0;
    lv_anim_init(&a);
    lv_anim_set_custom_exec_cb(&a, int_anim_exec_cb);
    lv_anim_set_var(&a, lv_obj_find_by_name(obj, "dot_4"));
    lv_anim_set_values(&a, 1200, 2400);
    lv_anim_set_duration(&a, 1200);
    lv_anim_set_user_data(&a, (void *)((uintptr_t)selector_and_prop));
    lv_anim_set_early_apply(&a, false);
    lv_anim_timeline_add(at, 990, &a);

    selector_and_prop = ((LV_STYLE_TRANSFORM_ROTATION & 0xff) << 24) | 0;
    lv_anim_init(&a);
    lv_anim_set_custom_exec_cb(&a, int_anim_exec_cb);
    lv_anim_set_var(&a, lv_obj_find_by_name(obj, "dot_4"));
    lv_anim_set_values(&a, 2400, 3600);
    lv_anim_set_duration(&a, 270);
    lv_anim_set_user_data(&a, (void *)((uintptr_t)selector_and_prop));
    lv_anim_set_early_apply(&a, false);
    lv_anim_timeline_add(at, 2190, &a);

    selector_and_prop = ((LV_STYLE_TRANSFORM_ROTATION & 0xff) << 24) | 0;
    lv_anim_init(&a);
    lv_anim_set_custom_exec_cb(&a, int_anim_exec_cb);
    lv_anim_set_var(&a, lv_obj_find_by_name(obj, "dot_4"));
    lv_anim_set_values(&a, 3600, 4800);
    lv_anim_set_duration(&a, 270);
    lv_anim_set_user_data(&a, (void *)((uintptr_t)selector_and_prop));
    lv_anim_set_early_apply(&a, false);
    lv_anim_timeline_add(at, 2460, &a);

    selector_and_prop = ((LV_STYLE_TRANSFORM_ROTATION & 0xff) << 24) | 0;
    lv_anim_init(&a);
    lv_anim_set_custom_exec_cb(&a, int_anim_exec_cb);
    lv_anim_set_var(&a, lv_obj_find_by_name(obj, "dot_4"));
    lv_anim_set_values(&a, 4800, 5550);
    lv_anim_set_duration(&a, 750);
    lv_anim_set_user_data(&a, (void *)((uintptr_t)selector_and_prop));
    lv_anim_set_early_apply(&a, false);
    lv_anim_timeline_add(at, 2730, &a);

    selector_and_prop = ((LV_STYLE_WIDTH & 0xff) << 24) | 0;
    lv_anim_init(&a);
    lv_anim_set_custom_exec_cb(&a, int_anim_exec_cb);
    lv_anim_set_var(&a, lv_obj_find_by_name(obj, "bar_fill"));
    lv_anim_set_values(&a, 0, 200);
    lv_anim_set_duration(&a, 1740);
    lv_anim_set_user_data(&a, (void *)((uintptr_t)selector_and_prop));
    lv_anim_set_early_apply(&a, false);
    lv_anim_timeline_add(at, 0, &a);

    selector_and_prop = ((LV_STYLE_WIDTH & 0xff) << 24) | 0;
    lv_anim_init(&a);
    lv_anim_set_custom_exec_cb(&a, int_anim_exec_cb);
    lv_anim_set_var(&a, lv_obj_find_by_name(obj, "bar_fill"));
    lv_anim_set_values(&a, 0, 200);
    lv_anim_set_duration(&a, 1740);
    lv_anim_set_user_data(&a, (void *)((uintptr_t)selector_and_prop));
    lv_anim_set_early_apply(&a, false);
    lv_anim_timeline_add(at, 1740, &a);

    return at;
}

static void free_timeline_event_cb(lv_event_t * e)
{
    lv_anim_timeline_t ** at_array = lv_event_get_user_data(e);
    uint32_t i;
    for(i = 0; i < _HOME_TIMELINE_CNT; i++) {
        lv_anim_timeline_delete(at_array[i]);
    }
    lv_free(at_array);
}

