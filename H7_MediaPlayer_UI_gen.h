/**
 * @file H7_MediaPlayer_UI_gen.h
 */

#ifndef LVGL_PRO_H7_MEDIAPLAYER_UI_GEN_H
#define LVGL_PRO_H7_MEDIAPLAYER_UI_GEN_H

#ifndef UI_SUBJECT_STRING_LENGTH
#define UI_SUBJECT_STRING_LENGTH 256
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
    #include "lvgl.h"
    #include "lvgl_private.h"
#else
    #include "lvgl/lvgl.h"
    #include "lvgl/lvgl_private.h"
#endif

#if defined(LV_USE_XML) && LV_USE_XML
    #include "lv_xml/lv_xml.h"
#endif



/* Prototypes for target functions, needed by responsive const definitions */

void H7_MediaPlayer_UI_set_target(uint32_t target);
uint32_t H7_MediaPlayer_UI_get_target(void);
bool H7_MediaPlayer_UI_check_target(uint32_t target);

/*********************
 *      DEFINES
 *********************/

#define H7_MEDIAPLAYER_UI_TARGET_UNDEFINED  (0 << 1)
#define H7_MEDIAPLAYER_UI_TARGET_TARGET1    (1 << 1)
#define H7_MEDIAPLAYER_UI_TARGET_ALL        0x0FFFFFFF

/* By default compile for all targets, allowing to switch to any targets at runtime */
#ifndef H7_MEDIAPLAYER_UI_COMPILE_TARGET
#define H7_MEDIAPLAYER_UI_COMPILE_TARGET H7_MEDIAPLAYER_UI_TARGET_ALL
#endif

#define H7_MEDIAPLAYER_UI_CHECK_COMPILE_TARGET(target) (H7_MEDIAPLAYER_UI_COMPILE_TARGET & (target) ? 1 : 0)

#ifndef LV_XML_EVAL_STRING_BUF_SIZE
    #define LV_XML_EVAL_STRING_BUF_SIZE 256
#endif

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL VARIABLES
 **********************/

/*-------------------
 * Permanent screens
 *------------------*/

/*----------------
 * Global styles
 *----------------*/

/*----------------
 * Fonts
 *----------------*/




/*----------------
 * Images
 *----------------*/



/*----------------
 * Subjects
 *----------------*/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/*----------------
 * Event Callbacks
 *----------------*/

/**
 * Initialize the component library
 */

void H7_MediaPlayer_UI_init_gen(const char * asset_path);

/**********************
 *      MACROS
 **********************/

/**********************
 *   POST INCLUDES
 **********************/

/*Include all the widgets, components and screens of this library*/
#include "screens/home_gen.h"

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LVGL_PRO_H7_MEDIAPLAYER_UI_GEN_H*/