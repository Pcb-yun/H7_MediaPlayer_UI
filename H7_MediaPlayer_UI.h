/**
 * @file H7_MediaPlayer_UI.h
 */

#ifndef LVGL_PRO_H7_MEDIAPLAYER_UI_H
#define LVGL_PRO_H7_MEDIAPLAYER_UI_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/

#include "H7_MediaPlayer_UI_gen.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

typedef enum {
    H7_UI_WIFI_OFF = 0,
    H7_UI_WIFI_ON,
    H7_UI_WIFI_CONNECTING,
    H7_UI_WIFI_CONNECTED,
} h7_ui_wifi_state_t;

typedef struct {
    h7_ui_wifi_state_t state;
    int8_t rssi;
} h7_ui_wifi_status_t;

typedef bool (*h7_ui_wifi_status_provider_t)(h7_ui_wifi_status_t * status);

/**********************
 * GLOBAL VARIABLES
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Initialize the component library
 */
void H7_MediaPlayer_UI_init(const char * asset_path);

/**
 * 注册非阻塞的 Wi-Fi 状态提供者。回调只能读取缓存，不应执行同步网络请求。
 * 未注册时首页显示“已关闭”；偶发读取失败时保留上一次显示。
 */
void H7_MediaPlayer_UI_set_wifi_status_provider(
    h7_ui_wifi_status_provider_t provider);

/**
 * 切换到另一块屏幕：主屏↔播放器循环，fade 过渡，旧屏在过渡结束后删除。
 * 由固件在焦点组边界回调（lv_group_set_edge_cb）中调用：焦点遍历到底后继续旋转即触发。
 * 文件浏览屏不在这条循环里：它由播放器屏的播放列表键进入、由列表首行"Back"退出。
 * @param forward 旋转方向：true=向右（正向），新屏焦点落遍历首项；
 *                false=向左（反向），新屏焦点落遍历末项
 */
void H7_MediaPlayer_UI_switch_screen(bool forward);

/**
 * 绑定屏内交互控件（屏幕创建后调用一次）：
 * 主屏动画承载按钮按下暂停/重播加载动画；播放器底部按钮按下切换播放/暂停图标。
 * @param screen 已创建的屏幕（home 或 player）
 */
void H7_MediaPlayer_UI_bind(lv_obj_t * screen);

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LVGL_PRO_H7_MEDIAPLAYER_UI_H*/
