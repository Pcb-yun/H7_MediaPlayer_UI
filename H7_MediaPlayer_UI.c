/**
 * @file H7_MediaPlayer_UI.c
 */

/*********************
 *      INCLUDES
 *********************/

#include "H7_MediaPlayer_UI.h"
#include "audio_player.h"
#include "apps/launcher/launcher_controller.h"
#include "apps/browser/browser_controller.h"
#include "apps/settings/settings_controller.h"
#include "apps/system_monitor/system_monitor.h"
#include "ui/ui_router.h"
#include "ui/ui_focus.h"
#include "boot.h"
#include <stdint.h>

/* 固件读片上RTC；模拟器用本机时间（区分机制与 audio_player_stub.c 相同） */
#ifdef STM32H723xx
#include "rtc.h"
#else
#include <time.h>
#endif

/*********************
 *      DEFINES
 *********************/

#define VOLUME_LEVEL_MAX        100     /* 音量上限（%） */
#define VOLUME_STEP             1       /* 每个棘轮格的音量步进（%） */
#define VOLUME_ICON_HIGH_MIN    50      /* 音量达到该值起显示大音量图标；0 显示静音图标，其余显示小音量图标 */

#define PLAYER_REFRESH_PERIOD   200     /* 播放器状态（进度/时间/播放暂停）刷新周期（ms） */

#define HOME_CLOCK_REFRESH_PERIOD   1000    // 主页时间/日期刷新周期（ms），按秒跳动
#define HOME_WIFI_REFRESH_PERIOD    1000    // 主页 Wi-Fi 显示刷新周期（ms），仅读取后台缓存
#define WIFI_RSSI_STRONG_MIN        (-60)   // dBm，强信号：两根弧线和底部扇形
#define WIFI_RSSI_MEDIUM_MIN        (-75)   // dBm，中信号：一根弧线和底部扇形

#define CJK_FONT_LINE_HEIGHT    19      // 中日文位图字体的每行高度（14号烘焙行高约27）
#define CJK_FONT_BASE_LINE      4       // 基线下方留白（14号烘焙值约8）
#define UI_RUNTIME_FONT_FILE    "fonts/SourceHanSansCN-Regular.ttf"

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void playpause_cb(lv_obj_t * obj, void * user_data);
static void prev_track_cb(lv_obj_t * obj, void * user_data);
static void next_track_cb(lv_obj_t * obj, void * user_data);
static void repeat_mode_cb(lv_obj_t * obj, void * user_data);
static const void * repeat_mode_icon(uint8_t mode);
static void volume_toggle_cb(lv_obj_t * obj, void * user_data);
static void volume_key_cb(lv_event_t * e);
static void progress_toggle_cb(lv_obj_t * obj, void * user_data);
static void progress_key_cb(lv_event_t * e);
static void power_cb(lv_obj_t * obj, void * user_data);
static void focus_watch_cb(lv_event_t * e);

static void volume_apply(void);
static void progress_apply(void);
static void player_refresh(void);
static void player_refresh_cb(lv_timer_t * t);
static void meta_refresh(const audio_player_snapshot_t * snapshot, bool active);
static void lyric_refresh(const audio_player_snapshot_t * snapshot, bool active);

static void playlist_open_cb(lv_obj_t * obj, void * user_data);
static bool music_browser_filter(const char * name, bool is_directory, void * user_data);
static void music_browser_select(const char * path, void * user_data);
static void firmware_browser_select(const char * path, void * user_data);
static void player_back_cb(lv_obj_t * obj, void * user_data);
static void home_open_launcher_cb(lv_obj_t * obj, void * user_data);
static void launcher_navigate(launcher_nav_target_t target);
static void settings_navigate(settings_nav_target_t target);
static void ui_home_cb(lv_obj_t * obj, void * user_data);
static void system_monitor_back_cb(lv_obj_t * obj, void * user_data);
static void placeholder_back_cb(lv_obj_t * obj, void * user_data);
static void placeholder_bind(lv_obj_t * screen);

static lv_font_t * cjk_font_for_text(const char * text);
static void cjk_font_line_height_compact(void);
static void runtime_fonts_init(const char * asset_path);
static void runtime_fonts_apply_home(lv_obj_t * screen);
static void obj_set_hidden(lv_obj_t * obj, bool hidden);

static const char * weekday_name(uint8_t weekday);
static void home_clock_refresh(void);
static void home_clock_refresh_cb(lv_timer_t * t);
static void home_wifi_refresh(void);
static void home_wifi_refresh_cb(lv_timer_t * t);

/**********************
 *  STATIC VARIABLES
 **********************/

/* 烘焙字体数据本身（const，编辑器生成）：gen.h 里只有字体指针，这里自己声明 */
extern const lv_font_t cjk_sc_14_data;
extern const lv_font_t cjk_jp_14_data;

/* 行高压缩用的可写字体描述符副本：生成的 cjk_*_data 是 const，改不了，拷贝一份出来改 */
static lv_font_t cjk_sc_compact;
static lv_font_t cjk_jp_compact;

/* TinyTTF 字体按字号各建一个实例；字体对象的 size 可变，不能让不同字号控件共用同一实例。 */
static lv_font_t * runtime_font_14;
static lv_font_t * runtime_font_44;

/* 默认字体指针，供固件 Applications/lvgl/lv_conf.h 的 LV_FONT_DEFAULT 使用
   （预览侧的 lv_conf.h 不引用它，所以预览用 montserrat_12 作默认字体）。
   指向下面行高压缩后的副本，必须在首次绘制前填好 —— 见 cjk_font_line_height_compact() */
const lv_font_t * const lv_ui_font_default = &cjk_sc_compact;

/* 循环模式：0=列表顺序 queue，1=列表循环 repeat，2=单曲循环 repeat_one，3=随机 shuffle。
   上电默认列表循环，且跨屏保留（切屏不复位，进播放器屏时再同步给播放器） */
static uint8_t repeat_mode = AUDIO_REPEAT_ALL;

static lv_obj_t * volume_popup;  /* 音量条外框（播放器屏重建，主屏或无此控件时为 NULL） */
static lv_obj_t * volume_fill;   /* 音量条填充，高度按音量比例 */
static lv_obj_t * volume_value;  /* 音量条顶部数字标签 */
static lv_obj_t * volume_icon;   /* 音量键内部图标，随音量在 mute / volume_low / volume_high 间切换 */
static bool volume_editing;      /* 音量调节态：外框展开期间为 true，此时旋转调音量而不是切换焦点 */
static uint8_t volume_level;     /* 当前音量 0..VOLUME_LEVEL_MAX */

static bool progress_editing;    /* 进度条时间调整态：进入时不seek，旋钮转动才真正发seek请求 */
static uint16_t seek_target_s;   /* 调整态下的目标位置（秒），仅用于界面预览与发送seek */

static lv_obj_t * progress_fill;    /* 进度条填充，宽度按播放百分比拉伸 */
static lv_obj_t * progress_time;    /* 进度行右侧的时间文本 */
static lv_obj_t * playpause_icon;   /* 播放/暂停键内的图标，按播放器真实状态切换 */
static lv_obj_t * meta_box;         /* 曲目信息外框（无标签的曲目整框隐藏） */
static lv_obj_t * meta_title;       /* 曲目歌名标签（播放器屏顶部） */
static lv_obj_t * meta_artist;      /* 曲目艺术家标签 */
static const lv_font_t * meta_font;     /* 曲目信息在XML里设的字体，纯ASCII时恢复用它 */
static lv_obj_t * lyric_box;        /* 歌词容器；没有可显示歌词时整框隐藏 */
static lv_obj_t * lyric_main;       /* 歌词原词标签（播放器屏中部） */
static lv_obj_t * lyric_tras;       /* 歌词翻译标签 */
static const lv_font_t * lyric_font;    /* 歌词在XML里设的字体，纯ASCII时恢复用它 */
static uint16_t progress_pos;       /* 已播位置（秒），取自播放器状态 */
static uint16_t progress_dur;       /* 总时长（秒），取自播放器状态 */
static bool player_playing;         /* 播放器屏当前是否在播放（显示 pause 图标），由 player_refresh 刷新 */

static lv_obj_t * home_time_label;  // 主页大号时间标签（非主页时为 NULL）
static lv_obj_t * home_date_label;  // 主页日期与星期标签（非主页时为 NULL）
static lv_obj_t * home_wifi_icon;   // 主页 Wi-Fi 图标
static lv_obj_t * home_wifi_label;  // 主页 Wi-Fi 状态文字
static h7_ui_wifi_status_provider_t wifi_status_provider;
static uint8_t home_wifi_last_state = UINT8_MAX;
static uint8_t home_wifi_last_level = UINT8_MAX;

static lv_obj_t * focus_list[12];   /* 本屏焦点遍历顺序（XML 创建顺序），容量留余量 */
static uint8_t focus_cnt;           /* 焦点列表长度 */
static int8_t focus_idx;            /* 当前焦点索引，-1=未知 */

/**********************
 *      MACROS
 **********************/

#define SCREEN_HOME             UI_ROUTE_HOME
#define SCREEN_LAUNCHER         UI_ROUTE_LAUNCHER
#define SCREEN_SETTINGS         UI_ROUTE_SETTINGS
#define SCREEN_ABOUT            UI_ROUTE_ABOUT
#define SCREEN_APP_PLACEHOLDER  UI_ROUTE_APP_PLACEHOLDER
#define SCREEN_PLAYER           UI_ROUTE_PLAYER
#define SCREEN_BROWSER          UI_ROUTE_BROWSER
#define SCREEN_SYSTEM_MONITOR   UI_ROUTE_SYSTEM_MONITOR
#define cur_screen              ui_router_current()
#define switch_forward          ui_router_forward()
#define ui_goto                 ui_router_open

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void H7_MediaPlayer_UI_init(const char * asset_path)
{
    LV_LOG("Initializing custom C code using LVGL v%d.%d.%d", LVGL_VERSION_MAJOR, LVGL_VERSION_MINOR, LVGL_VERSION_PATCH);

    (void)audio_player_init();

    /* 换字体要在 init_gen 之前：里面是 if(!cjk_sc_14) 才赋值，提前指好就不会被改回去，
       而且此刻还没有任何屏幕/标签被创建 */
    cjk_font_line_height_compact();

    H7_MediaPlayer_UI_init_gen(asset_path);
    runtime_fonts_init(asset_path);

    ui_router_init(H7_MediaPlayer_UI_bind, UI_ROUTE_HOME);
    ui_focus_set_home_cb(ui_home_cb);    /* 全局长按动作：任何页面长按都回主页 */
    launcher_controller_init(launcher_navigate);
    settings_controller_init(settings_navigate);

    /* 周期把播放器真实状态同步到播放器屏（不在播放器屏时回调直接返回） */
    lv_timer_create(player_refresh_cb, PLAYER_REFRESH_PERIOD, NULL);

    /* 周期把RTC时间同步到主页时间/日期标签（不在主页时回调直接返回） */
    lv_timer_create(home_clock_refresh_cb, HOME_CLOCK_REFRESH_PERIOD, NULL);

    /* 状态查询在独立任务中完成；LVGL 定时器只读取缓存并更新控件。 */
    lv_timer_create(home_wifi_refresh_cb, HOME_WIFI_REFRESH_PERIOD, NULL);
}

void H7_MediaPlayer_UI_set_wifi_status_provider(
    h7_ui_wifi_status_provider_t provider)
{
    wifi_status_provider = provider;
    home_wifi_last_state = UINT8_MAX;
    home_wifi_last_level = UINT8_MAX;
}

void H7_MediaPlayer_UI_switch_screen(bool forward)
{
    /* 应用选择器由焦点组自身回环；不再通过旋转在不同业务屏之间切换。 */
    LV_UNUSED(forward);
}

void H7_MediaPlayer_UI_bind(lv_obj_t * screen)
{
    lv_group_t * group;

    if(screen == NULL) return;

    /* 换屏后先清掉上一次遗留的组编辑态，否则旋钮只发 LEFT/RIGHT、无法移动焦点 */
    group = lv_group_get_default();
    if(group != NULL) lv_group_set_editing(group, false);

    /* 非播放器屏没有这些控件，先清空指针，避免切屏后定时器或回调访问已删除的控件 */
    playpause_icon = NULL;
    progress_fill = NULL;
    progress_time = NULL;
    volume_icon = NULL;
    meta_title = NULL;
    meta_artist = NULL;
    meta_box = NULL;
    lyric_box = NULL;
    lyric_main = NULL;
    lyric_tras = NULL;
    home_time_label = NULL;
    home_date_label = NULL;
    home_wifi_icon = NULL;
    home_wifi_label = NULL;
    home_wifi_last_state = UINT8_MAX;
    home_wifi_last_level = UINT8_MAX;

    /* 文件浏览屏的内容是按目录动态生成的列表，不参与"回绕切屏"，单独接线 */
    if(cur_screen == SCREEN_BROWSER) {
        browser_controller_bind(screen);
        return;
    }
    if(cur_screen == SCREEN_LAUNCHER) {
        launcher_controller_bind(screen);
        return;
    }
    if(cur_screen == SCREEN_SETTINGS) {
        settings_controller_bind(screen);
        return;
    }
    if(cur_screen == SCREEN_ABOUT) {
        settings_about_controller_bind(screen);
        return;
    }
    if(cur_screen == SCREEN_APP_PLACEHOLDER) {
        placeholder_bind(screen);
        return;
    }
    if(cur_screen == SCREEN_SYSTEM_MONITOR) {
        system_monitor_bind(screen, system_monitor_back_cb);
        return;
    }
    if(cur_screen == SCREEN_HOME) {
        runtime_fonts_apply_home(screen);
        /* 主页时间/日期：接管标签指针并立即刷一次，之后由定时器按秒维护 */
        home_time_label = lv_obj_find_by_name(screen, "home_time");
        home_date_label = lv_obj_find_by_name(screen, "home_date");
        home_wifi_icon = lv_obj_find_by_name(screen, "home_wifi_icon");
        home_wifi_label = lv_obj_find_by_name(screen, "home_wifi_state");
        home_clock_refresh();
        home_wifi_refresh();
        lv_obj_t * entry = lv_obj_find_by_name(screen, "text_focus");
        if(entry != NULL) {
            /* 尺寸与透明样式均在 home.xml；这里仅绑定行为：单击进入应用选择器 */
            ui_focus_bind_row(entry, home_open_launcher_cb, NULL, NULL);
            lv_group_focus_obj(entry);
        }
        return;
    }
    /* 记录本屏焦点遍历顺序并监听焦点变化：多控件屏中焦点到端点后 LVGL 会自动回绕
       （edge_cb 不触发），靠回绕检测实现遍历完切屏，所以顺序必须与 XML 创建顺序一致：
       主屏文字框；播放器顶栏电源、播放列表；进度条；播放器底栏音量、上一曲、播放/暂停、下一曲、循环模式 */
    static const char * names[] = {
        "text_focus",
        "btn_power", "btn_playlist", "progress_track",
        "btn_volume", "btn_prev", "btn_playpause", "btn_next", "btn_repeat",
    };
    focus_cnt = 0;
    focus_idx = -1;
    for(uint32_t i = 0; i < sizeof(names) / sizeof(names[0]); i++) {
        lv_obj_t * obj = lv_obj_find_by_name(screen, names[i]);
        if(obj == NULL) continue;
        if(focus_cnt >= sizeof(focus_list) / sizeof(focus_list[0])) continue;    /* 列表满则忽略，避免越界 */
        focus_list[focus_cnt++] = obj;
        lv_obj_add_event_cb(obj, focus_watch_cb, LV_EVENT_FOCUSED, NULL);
        if(cur_screen == SCREEN_PLAYER) {
            /* 播放器屏：双击任一控件返回启动器，长按回主页 */
            ui_focus_bind_row(obj, NULL, player_back_cb, NULL);
        }
    }

    /* 显式定焦点，避免旧屏焦点对象删除时组按 refocus 策略回退到末项：
       正向（向右旋转）切屏落遍历首项，反向（向左旋转）切屏落遍历末项，
       使焦点在跨屏时沿旋转方向连续（播放器首项 btn_power、末项 btn_repeat） */
    if(focus_cnt > 0) {
        uint8_t idx = switch_forward ? 0 : (uint8_t)(focus_cnt - 1);
        lv_group_focus_obj(focus_list[idx]);
    }

    /* 播放器：播放/暂停键；图标与实际状态由 player_refresh 按播放器状态刷新 */
    lv_obj_t * btn = lv_obj_find_by_name(screen, "btn_playpause");
    if(btn != NULL) {
        playpause_icon = lv_obj_get_child(btn, 0);    /* 按钮内唯一子控件即图标 */
        ui_focus_bind_row(btn, playpause_cb, NULL, NULL);
    }
    else {
        playpause_icon = NULL;
    }

    /* 播放器：上一曲/下一曲（预留接口）与循环模式切换 */
    lv_obj_t * btn_prev = lv_obj_find_by_name(screen, "btn_prev");
    if(btn_prev != NULL) ui_focus_bind_row(btn_prev, prev_track_cb, NULL, NULL);

    lv_obj_t * btn_next = lv_obj_find_by_name(screen, "btn_next");
    if(btn_next != NULL) ui_focus_bind_row(btn_next, next_track_cb, NULL, NULL);

    lv_obj_t * btn_repeat = lv_obj_find_by_name(screen, "btn_repeat");
    if(btn_repeat != NULL) {
        /* 循环模式跨屏保留（repeat_mode 是静态变量）：本屏图标按已保存的模式重建，
           再把模式同步给播放器，避免重进播放器屏时图标与实际模式不一致 */
        lv_obj_t * repeat_icon = lv_obj_get_child(btn_repeat, 0);
        if(repeat_icon != NULL) lv_image_set_src(repeat_icon, repeat_mode_icon(repeat_mode));

        (void)audio_player_set_repeat((audio_repeat_t)repeat_mode);
        ui_focus_bind_row(btn_repeat, repeat_mode_cb, NULL, NULL);
    }

    /* 播放器：播放列表键进入文件浏览屏 */
    lv_obj_t * btn_playlist = lv_obj_find_by_name(screen, "btn_playlist");
    if(btn_playlist != NULL) ui_focus_bind_row(btn_playlist, playlist_open_cb, NULL, NULL);

    /* 播放器：电源键暂接成"停止播放"（电源语义未定），给界面一个结束播放的入口 */
    lv_obj_t * btn_power = lv_obj_find_by_name(screen, "btn_power");
    if(btn_power != NULL) ui_focus_bind_row(btn_power, power_cb, NULL, NULL);

    /* 播放器：音量键按下展开音量条，展开期间旋转调音量、再次按下收起（主屏上查找为空，状态自然复位） */
    volume_popup = lv_obj_find_by_name(screen, "volume_popup");
    volume_fill = lv_obj_find_by_name(screen, "volume_fill");
    volume_value = lv_obj_find_by_name(screen, "volume_value");
    volume_editing = false;
    {
        audio_player_snapshot_t snapshot;
        volume_level = (audio_player_snapshot(&snapshot) == AUDIO_RES_OK) ?
            snapshot.volume : AUDIO_DEFAULT_VOLUME;
    }
    if(volume_popup != NULL) {
        obj_set_hidden(volume_popup, true);         /* 进屏与切屏重建后统一收起 */
        lv_obj_remove_flag(volume_popup, LV_OBJ_FLAG_SCROLLABLE);   /* 避免内容超界时画出滚动条 */
    }

    lv_obj_t * btn_volume = lv_obj_find_by_name(screen, "btn_volume");
    if(btn_volume != NULL) {
        volume_icon = lv_obj_get_child(btn_volume, 0);    /* 按钮内唯一子控件即音量图标 */
        ui_focus_bind_row(btn_volume, volume_toggle_cb, NULL, NULL);
        lv_obj_add_event_cb(btn_volume, volume_key_cb, LV_EVENT_KEY, NULL);
    }
    else {
        volume_icon = NULL;
    }

    /* 播放器：进度条与播放时间；数值由 player_refresh 按播放器状态刷新，字体使用 cjk_sc_14。 */
    progress_fill = lv_obj_find_by_name(screen, "progress_fill");
    progress_time = lv_obj_find_by_name(screen, "progress_time");
    progress_editing = false;           /* 每次进屏都回到非调整态 */

    /* 播放器：曲目信息两行标签（歌名 + 艺术家），文本由 player_refresh 刷新 */
    meta_box = lv_obj_find_by_name(screen, "meta_box");
    meta_title = lv_obj_find_by_name(screen, "meta_title");
    meta_artist = lv_obj_find_by_name(screen, "meta_artist");
    meta_font = (meta_title != NULL) ? lv_obj_get_style_text_font(meta_title, LV_PART_MAIN) :
        LV_FONT_DEFAULT;

    /* 播放器：歌词两行（原词 + 翻译），文本由 player_refresh 按播放位置刷新 */
    lyric_box = lv_obj_find_by_name(screen, "lyric_box");
    lyric_main = lv_obj_find_by_name(screen, "lyric_main");
    lyric_tras = lv_obj_find_by_name(screen, "lyric_tras");
    lyric_font = (lyric_main != NULL) ? lv_obj_get_style_text_font(lyric_main, LV_PART_MAIN) :
        LV_FONT_DEFAULT;

    /* 进度轨道（可聚焦）：按下进出时间调整态；调整态下旋钮的 KEY 事件才真正发 seek 请求 */
    lv_obj_t * track = lv_obj_find_by_name(screen, "progress_track");
    if(track != NULL) {
        ui_focus_bind_row(track, progress_toggle_cb, NULL, NULL);
        lv_obj_add_event_cb(track, progress_key_cb, LV_EVENT_KEY, NULL);
    }

    volume_apply();
    player_refresh();
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/** @brief 显示/隐藏控件（使用 LVGL 9 的对象 flag API）。 */
static void obj_set_hidden(lv_obj_t * obj, bool hidden)
{
	if(obj == NULL) return;
	if(hidden) lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
	else lv_obj_remove_flag(obj, LV_OBJ_FLAG_HIDDEN);
}

/**
 * @brief 用压缩行高的字体副本替换编辑器生成的烘焙字体
 *
 * 行高不是从TTF的度量表来的，而是 lv_font_conv 按实际烘焙字形算的（14号字号给到
 * 约27px = 上方19 + 下方8），每行上下各多出约5px空白。生成的 cjk_*_data 是 const，
 * 编辑器每次 Generate 都会覆盖，所以这里拷贝一份可写描述符改 line_height/base_line：
 * 19 = 上方15 + 下方4（按 12 号时的实测比例放大：字形只用到上方11到13、下方2到3），
 * 一个字都不缺，只有极少数竖线类生僻字可能被裁1px。必须在创建任何屏幕之前调用。
 */
static void cjk_font_line_height_compact(void)
{
	cjk_sc_compact = cjk_sc_14_data;
	cjk_sc_compact.line_height = CJK_FONT_LINE_HEIGHT;
	cjk_sc_compact.base_line = CJK_FONT_BASE_LINE;
	#if LV_FONT_MONTSERRAT_14
	cjk_sc_compact.fallback = &lv_font_montserrat_14;
	#endif
	cjk_jp_compact = cjk_jp_14_data;
	cjk_jp_compact.line_height = CJK_FONT_LINE_HEIGHT;
	cjk_jp_compact.base_line = CJK_FONT_BASE_LINE;
	#if LV_FONT_MONTSERRAT_14
	cjk_jp_compact.fallback = &lv_font_montserrat_14;
	#endif

	/* 各屏的 create 读的就是这两个指针，换掉即全局生效（含默认字体） */
	cjk_sc_14 = &cjk_sc_compact;
	cjk_jp_14 = &cjk_jp_compact;
}

/**
 * @brief 从资源根目录加载运行时思源黑体
 *
 * 固件传入 C:/，对应 SD 卡 /fonts/SourceHanSansCN-Regular.ttf；
 * PC 模拟器传入 A:，对应工程 fonts/SourceHanSansCN-Regular.ttf。
 * 加载失败时保留 XML 中的烘焙字体，不阻断界面启动。
 */
static void runtime_fonts_init(const char * asset_path)
{
#if LV_USE_TINY_TTF && LV_TINY_TTF_FILE_SUPPORT
	char path[LV_FS_MAX_PATH_LENGTH];
	size_t length;
	const char * separator;

	if(asset_path == NULL || asset_path[0] == '\0') return;

	length = lv_strlen(asset_path);
	separator = (asset_path[length - 1U] == ':' ||
	             asset_path[length - 1U] == '/' ||
	             asset_path[length - 1U] == '\\') ? "" : "/";
	lv_snprintf(path, sizeof(path), "%s%s%s", asset_path, separator, UI_RUNTIME_FONT_FILE);

	runtime_font_14 = lv_tiny_ttf_create_file(path, 14);
	runtime_font_44 = lv_tiny_ttf_create_file(path, 44);

#if LV_FONT_MONTSERRAT_14
	if(runtime_font_14 != NULL) runtime_font_14->fallback = &lv_font_montserrat_14;
#endif

	if(runtime_font_14 == NULL || runtime_font_44 == NULL) {
		LV_LOG_WARN("TinyTTF font load incomplete: %s", path);
	}
#else
	LV_UNUSED(asset_path);
#endif
}

/**
 * @brief 只给主页面低刷新频率文本应用 TinyTTF
 *
 * 播放器歌词、浏览器列表等高频刷新区域继续使用烘焙字体。
 */
static void runtime_fonts_apply_home(lv_obj_t * screen)
{
	lv_obj_t * obj;

	if(screen == NULL) return;

	if(runtime_font_44 != NULL) {
		obj = lv_obj_find_by_name(screen, "home_time");
		if(obj != NULL) lv_obj_set_style_text_font(obj, runtime_font_44, 0);
	}

	if(runtime_font_14 != NULL) {
		static const char * names[] = {"home_date", "home_wifi_state"};
		for(uint32_t i = 0; i < sizeof(names) / sizeof(names[0]); i++) {
			obj = lv_obj_find_by_name(screen, names[i]);
			if(obj != NULL) lv_obj_set_style_text_font(obj, runtime_font_14, 0);
		}
	}
}

/* 把缓存状态映射到主页文字和四态 Wi-Fi 图标。
   三格轮廓始终保留：激活格为白色，未激活格为深灰，关闭态叠加白色斜杠。 */
static void home_wifi_refresh(void)
{
	h7_ui_wifi_status_t status = {
		.state = H7_UI_WIFI_OFF,
		.rssi = INT8_MIN,
	};
	const void * icon_src = wifi_off;
	const char * label_text = "已关闭";
	uint8_t signal_level = 0;

	if(home_wifi_icon == NULL || home_wifi_label == NULL) return;
	if(wifi_status_provider != NULL && !wifi_status_provider(&status)) return;

	switch(status.state) {
		case H7_UI_WIFI_CONNECTED:
			label_text = "已连接";
			if(status.rssi >= WIFI_RSSI_STRONG_MIN) {
				signal_level = 2;
				icon_src = wifi_connected;
			} else if(status.rssi >= WIFI_RSSI_MEDIUM_MIN) {
				signal_level = 1;
				icon_src = wifi_signal_medium;
			}
			break;

		case H7_UI_WIFI_CONNECTING:
			label_text = "连接中";
			signal_level = 1;
			icon_src = wifi_signal_medium;
			break;

		case H7_UI_WIFI_ON:
			label_text = "已打开";
			icon_src = wifi_signal_weak;
			break;

		case H7_UI_WIFI_OFF:
		default:
			status.state = H7_UI_WIFI_OFF;
			break;
	}

	if(home_wifi_last_state == (uint8_t)status.state &&
	   home_wifi_last_level == signal_level) {
		return;
	}

	lv_label_set_text(home_wifi_label, label_text);
	lv_image_set_src(home_wifi_icon, icon_src);
	lv_obj_set_style_image_opa(home_wifi_icon, LV_OPA_COVER, LV_PART_MAIN);
	home_wifi_last_state = (uint8_t)status.state;
	home_wifi_last_level = signal_level;
}

static void home_wifi_refresh_cb(lv_timer_t * t)
{
	LV_UNUSED(t);
	home_wifi_refresh();
}

/* 星期序号 → 中文名（RTC WeekDay 与 tm_wday 均已归一化为 1=周一 ... 7=周日） */
static const char * weekday_name(uint8_t weekday)
{
	static const char * names[] = { "周一", "周二", "周三", "周四", "周五", "周六", "周日" };

	if(weekday < 1 || weekday > 7) return "";
	return names[weekday - 1];
}

/* 读取当前时间刷新主页时间/日期标签；文本未变化时不重设，避免无谓重排版。
   固件读片上RTC（WiFi时间同步已写入），模拟器用本机时间便于预览 */
static void home_clock_refresh(void)
{
	uint8_t hour;
	uint8_t minute;
	uint8_t month;
	uint8_t day;
	uint8_t weekday;
	char buf[32];

	if(home_time_label == NULL) return;    // 不在主页

#ifdef STM32H723xx
	RTC_TimeTypeDef time = {0};
	RTC_DateTypeDef date = {0};

	HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN);
	HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN);    // 必须紧跟 GetTime，解锁日历影子寄存器
	hour = time.Hours;
	minute = time.Minutes;
	month = date.Month;
	day = date.Date;
	weekday = date.WeekDay;
#else
	time_t now = time(NULL);
	struct tm * local = localtime(&now);

	if(local == NULL) return;
	hour = (uint8_t)local->tm_hour;
	minute = (uint8_t)local->tm_min;
	month = (uint8_t)(local->tm_mon + 1);
	day = (uint8_t)local->tm_mday;
	weekday = (uint8_t)((local->tm_wday == 0) ? 7 : local->tm_wday);
#endif

	lv_snprintf(buf, sizeof(buf), "%02u:%02u", (unsigned)hour, (unsigned)minute);
	if(lv_strcmp(lv_label_get_text(home_time_label), buf) != 0) {
		lv_label_set_text(home_time_label, buf);
	}

	if(home_date_label == NULL) return;
	lv_snprintf(buf, sizeof(buf), "%u月%u日  %s",
	            (unsigned)month, (unsigned)day, weekday_name(weekday));
	if(lv_strcmp(lv_label_get_text(home_date_label), buf) != 0) {
		lv_label_set_text(home_date_label, buf);
	}
}

/* 时钟定时器回调：非主页时标签指针为空，刷新直接返回 */
static void home_clock_refresh_cb(lv_timer_t * t)
{
	LV_UNUSED(t);
	home_clock_refresh();
}

/* 主页面短按：进入应用选择器时始终从左上角的“设置”开始。 */
static void home_open_launcher_cb(lv_obj_t * obj, void * user_data)
{
    LV_UNUSED(obj);
    LV_UNUSED(user_data);
    launcher_controller_reset();
    ui_goto(SCREEN_LAUNCHER, true);
}

static void launcher_navigate(launcher_nav_target_t target)
{
    switch(target) {
        case LAUNCHER_NAV_SETTINGS:
            ui_goto(SCREEN_SETTINGS, true);
            break;
        case LAUNCHER_NAV_MUSIC:
            ui_goto(SCREEN_PLAYER, true);
            break;
        case LAUNCHER_NAV_FILES:
            (void)browser_controller_open(&(browser_controller_request_t) {
                .root_path = "C:/", .allow_directories = true, .allow_delete = true,
                .return_route = SCREEN_LAUNCHER,
            });
            break;
        case LAUNCHER_NAV_SYSTEM_MONITOR:
            ui_goto(SCREEN_SYSTEM_MONITOR, true);
            break;
        case LAUNCHER_NAV_PLACEHOLDER:
            ui_goto(SCREEN_APP_PLACEHOLDER, true);
            break;
        default:
            ui_goto(SCREEN_HOME, false);
            break;
    }
}

/* 设置页统一采用长按返回；具体设置动作在各功能接入时再绑定。 */
static void settings_navigate(settings_nav_target_t target)
{
    switch(target) {
        case SETTINGS_NAV_ABOUT:
            ui_goto(SCREEN_ABOUT, true);
            break;
        case SETTINGS_NAV_SETTINGS:
            ui_goto(SCREEN_SETTINGS, false);
            break;
        case SETTINGS_NAV_FIRMWARE_BROWSER:
            /* 本地更新：复用浏览屏只列出 .bin 固件文件，选中后由 BootShared_Update 复位烧录 */
            (void)browser_controller_open(&(browser_controller_request_t) {
                .root_path = "C:/", .extensions = "hex", .allow_directories = false,
                .select_cb = firmware_browser_select, .return_route = SCREEN_SETTINGS,
            });
            break;
        default:
            ui_goto(SCREEN_LAUNCHER, false);
            break;
    }
}

/* 全局长按动作：任意页面长按都回主页。 */
static void ui_home_cb(lv_obj_t * obj, void * user_data)
{
    LV_UNUSED(obj);
    LV_UNUSED(user_data);
    ui_goto(SCREEN_HOME, false);
}

/* 系统监视页双击返回应用选择器，并保留刚才选中的应用。 */
static void system_monitor_back_cb(lv_obj_t * obj, void * user_data)
{
    LV_UNUSED(obj);
    LV_UNUSED(user_data);
    ui_goto(SCREEN_LAUNCHER, false);
}

/* 应用占位页双击返回应用选择器，并保留刚才选中的应用。 */
static void placeholder_back_cb(lv_obj_t * obj, void * user_data)
{
    LV_UNUSED(obj);
    LV_UNUSED(user_data);
    ui_goto(SCREEN_LAUNCHER, false);
}

/* 音乐应用双击返回启动器只离开界面，播放器任务继续运行。 */
static void player_back_cb(lv_obj_t * obj, void * user_data)
{
    LV_UNUSED(obj);
    LV_UNUSED(user_data);
    ui_goto(SCREEN_LAUNCHER, false);
}

static void placeholder_bind(lv_obj_t * screen)
{
    lv_obj_t * back = lv_obj_find_by_name(screen, "app_placeholder_back");
    lv_obj_t * title = lv_obj_find_by_name(screen, "app_placeholder_title");
    if(title != NULL) lv_label_set_text(title, launcher_controller_selected_name());
    if(back == NULL) return;
    ui_focus_bind_row(back, NULL, placeholder_back_cb, NULL);
    lv_group_focus_obj(back);
}

/* 播放/暂停键回调：正在播放则切换暂停；空闲且此前选过文件则重新起播。
   图标与进度不在这里改，统一由 player_refresh 按播放器真实状态刷新 */
static void playpause_cb(lv_obj_t * obj, void * user_data)
{
    audio_player_snapshot_t snapshot;

    LV_UNUSED(obj);
    LV_UNUSED(user_data);
    if(audio_player_snapshot(&snapshot) != AUDIO_RES_OK) return;
    if(snapshot.state == AUDIO_PLAYER_PAUSED) {
        (void)audio_player_resume();
    }
    else if(snapshot.state == AUDIO_PLAYER_PLAYING) {
        (void)audio_player_pause();
    }
    else if((snapshot.state == AUDIO_PLAYER_IDLE || snapshot.state == AUDIO_PLAYER_ERROR) &&
        snapshot.source[0] != '\0') {
        /* 最近播放路径属于播放器状态，不再依赖已经移除的旧浏览器全局变量。 */
        (void)audio_player_play(snapshot.source);
    }
}

/* 上一曲按钮回调：在播放列表内往前切一首 */
static void prev_track_cb(lv_obj_t * obj, void * user_data)
{
    LV_UNUSED(obj);
    LV_UNUSED(user_data);
    (void)audio_player_previous();
}

/* 下一曲按钮回调：在播放列表内往后切一首 */
static void next_track_cb(lv_obj_t * obj, void * user_data)
{
    LV_UNUSED(obj);
    LV_UNUSED(user_data);
    (void)audio_player_next();
}

/* 取循环模式对应的图标：图片符号是指针变量，值链接期才确定，不能用 static 数组初始化 */
static const void * repeat_mode_icon(uint8_t mode)
{
    const void * modes[4] = { queue, repeat, repeat_one, shuffle };

    return modes[mode % 4];
}

/* 循环模式按钮回调：列表顺序 → 列表循环 → 单曲循环 → 随机，点击循环切换图标 */
static void repeat_mode_cb(lv_obj_t * obj, void * user_data)
{
    lv_obj_t * icon = (obj != NULL) ? lv_obj_get_child(obj, 0) : NULL;

    LV_UNUSED(user_data);
    if(icon == NULL) return;

    repeat_mode = (uint8_t)((repeat_mode + 1) % 4);
    lv_image_set_src(icon, repeat_mode_icon(repeat_mode));
    (void)audio_player_set_repeat((audio_repeat_t)repeat_mode);
}

/* 音量键回调：按下展开/收起音量条，并同步切换焦点组的编辑态
   （编辑态下编码器旋转才会以 LV_KEY_LEFT/RIGHT 发给焦点对象，而不是移动焦点） */
static void volume_toggle_cb(lv_obj_t * obj, void * user_data)
{
    lv_group_t * group = (obj != NULL) ? lv_obj_get_group(obj) : NULL;

    LV_UNUSED(user_data);
    if(group == NULL) return;

    volume_editing = !volume_editing;
    if(volume_popup != NULL) obj_set_hidden(volume_popup, !volume_editing);
    volume_apply();
    lv_group_set_editing(group, volume_editing);
}

/* 音量条展开时的旋转：组编辑态把旋转转成 LV_KEY_LEFT/RIGHT 的 KEY 事件发给音量键 */
static void volume_key_cb(lv_event_t * e)
{
    if(!volume_editing) return;

    uint32_t key = *(uint32_t *)lv_event_get_param(e);
    int32_t level = (int32_t)volume_level;
    if(key == LV_KEY_RIGHT) level += VOLUME_STEP;
    else if(key == LV_KEY_LEFT) level -= VOLUME_STEP;
    else return;

    if(level < 0) level = 0;
    if(level > VOLUME_LEVEL_MAX) level = VOLUME_LEVEL_MAX;
    volume_level = (uint8_t)level;
    volume_apply();
    (void)audio_player_set_volume(volume_level);
}

/* 把当前音量应用到界面：音量键图标、顶部数字、填充高度。
   数字高度从填充上限里扣掉，填充最多长到数字下方，不会压住数字 */
static void volume_apply(void)
{
    if(volume_icon != NULL) {
        const void * src = volume_low;                  /* 默认：小音量图标 */
        if(volume_level == 0) src = mute;               /* 音量为 0：静音图标 */
        else if(volume_level >= VOLUME_ICON_HIGH_MIN) src = volume_high;
        lv_image_set_src(volume_icon, src);
    }
    if(volume_fill == NULL || volume_popup == NULL) return;

    int32_t max_h = lv_obj_get_content_height(volume_popup);
    if(volume_value != NULL) {
        lv_label_set_text_fmt(volume_value, "%d", volume_level);    /* 先刷文本，再按其高度让出顶部空间 */
        max_h -= lv_obj_get_height(volume_value);
    }
    if(max_h < 0) max_h = 0;

    int32_t h = 0;
    if(volume_level > 0 && max_h > 0) {
        h = (int32_t)volume_level * max_h / VOLUME_LEVEL_MAX;
        if(h < 1) h = 1;    /* 非零音量至少 1px，避免看不见 */
    }
    lv_obj_set_height(volume_fill, h);
}

/* 把当前播放进度应用到界面：进度条按百分比拉伸，时间文本显示剩余时间（-MM:SS）。
   停止或播放结束时播放器把时长归零，这里同步显示 00:00 */
static void progress_apply(void)
{
    int32_t pct = 0;
    uint16_t remain_s;

    if(progress_dur > 0) {
        pct = (int32_t)progress_pos * 100 / (int32_t)progress_dur;
        if(pct > 100) pct = 100;
    }
    if(progress_fill != NULL) lv_obj_set_width(progress_fill, lv_pct(pct));
    if(progress_time == NULL) return;

    if(progress_dur == 0) {         /* 未起播或已播完：剩余时间归零 */
        lv_label_set_text(progress_time, "00:00");
        return;
    }
    remain_s = (progress_pos < progress_dur) ? (uint16_t)(progress_dur - progress_pos) : 0;
    lv_label_set_text_fmt(progress_time, "-%02u:%02u",
                          (unsigned)(remain_s / 60u), (unsigned)(remain_s % 60u));
}

/* 进度条回调：按下进入/退出时间调整态。进入时只做界面预览，不发送 seek；
   只有调整态下旋钮转动（KEY 事件）才真正发 seek 请求 */
static void progress_toggle_cb(lv_obj_t * obj, void * user_data)
{
    lv_group_t * group = (obj != NULL) ? lv_obj_get_group(obj) : NULL;

    LV_UNUSED(user_data);
    if(group == NULL) return;

    progress_editing = !progress_editing;
    if(progress_editing) {
        seek_target_s = progress_pos;      /* 从当前位置起调 */
        progress_apply();
    }
    else {
        player_refresh();                  /* 退出调整态：回到播放器真实位置 */
    }
    lv_group_set_editing(group, progress_editing);
}

/* 时间调整态下的旋钮：左右转动调整目标位置，并在此刻才真正发送 seek 请求 */
static void progress_key_cb(lv_event_t * e)
{
    uint32_t key;

    if(!progress_editing) return;

    key = *(uint32_t *)lv_event_get_param(e);
    if(key != LV_KEY_RIGHT && key != LV_KEY_LEFT) return;
    if(progress_dur == 0) return;          /* 时长未知：不处理 */

    if(key == LV_KEY_RIGHT) {
        seek_target_s = (uint16_t)(seek_target_s + AUDIO_SEEK_STEP);
    }
    else {
        seek_target_s = (seek_target_s > AUDIO_SEEK_STEP) ? (uint16_t)(seek_target_s - AUDIO_SEEK_STEP) : 0;
    }
    if(seek_target_s > progress_dur) seek_target_s = progress_dur;

    progress_pos = seek_target_s;          /* 界面先预览目标位置 */
    progress_apply();
    (void)audio_player_seek((uint32_t)seek_target_s * 1000u);
}

/* 曲目信息刷新：取歌名/艺术家写进两块标签。同样每 200ms 调一次，先比文本再决定是否重设；
   停止时不显示（留空），避免残留上一首的信息 */
static void meta_refresh(const audio_player_snapshot_t * snapshot, bool active)
{
    const audio_meta_t * meta;
    const lv_font_t * font;
    static const audio_meta_t empty_meta;

    if(meta_title == NULL) return;      /* 不在播放器屏 */
    meta = active ? &snapshot->meta : &empty_meta;

    /* 曲目没有标签时两条都是空串，把整框隐藏，避免屏幕上留一条空底 */
    if(meta_box != NULL) obj_set_hidden(meta_box,
        (meta->title[0] == '\0') && (meta->artist[0] == '\0'));

    if(lv_strcmp(lv_label_get_text(meta_title), meta->title) != 0) {
        lv_label_set_text(meta_title, meta->title);
        font = cjk_font_for_text(meta->title);
        lv_obj_set_style_text_font(meta_title, (font != NULL) ? font : meta_font, 0);
    }

    if(meta_artist != NULL && lv_strcmp(lv_label_get_text(meta_artist), meta->artist) != 0) {
        lv_label_set_text(meta_artist, meta->artist);
        font = cjk_font_for_text(meta->artist);
        lv_obj_set_style_text_font(meta_artist, (font != NULL) ? font : meta_font, 0);
    }
}

/* 歌词刷新：按播放位置取当前行的原词与翻译写进两块标签。LRC 模块内部是增量扫描
   （顺序读指针，只在位置回退时回头重找），每 200ms 调一次几乎没有开销；
   文本没变就不重设标签，避免反复重排版。停止播放时两行清空，免得残留上一首 */
static void lyric_refresh(const audio_player_snapshot_t * snapshot, bool active)
{
    const char * raw = active ? snapshot->lyric.original : "";
    const char * tras = active ? snapshot->lyric.translation : "";
    const lv_font_t * font;
    bool has_raw;
    bool has_translation;

    if(lyric_main == NULL) return;      /* 不在播放器屏 */
    has_raw = raw[0] != '\0';
    has_translation = tras[0] != '\0';

    /* 播放器尚未给出当前歌词（无歌词或首句尚未到达）时不显示空框。
       单语歌词隐藏译词标签，让原词继续由容器的 flex 布局垂直居中。 */
    obj_set_hidden(lyric_box, !has_raw && !has_translation);
    obj_set_hidden(lyric_tras, !has_translation);

    if(lv_strcmp(lv_label_get_text(lyric_main), raw) != 0) {
        lv_label_set_text(lyric_main, raw);
        font = cjk_font_for_text(raw);
        lv_obj_set_style_text_font(lyric_main, (font != NULL) ? font : lyric_font, 0);
    }

    if(lyric_tras != NULL && lv_strcmp(lv_label_get_text(lyric_tras), tras) != 0) {
        lv_label_set_text(lyric_tras, tras);
        font = cjk_font_for_text(tras);
        lv_obj_set_style_text_font(lyric_tras, (font != NULL) ? font : lyric_font, 0);
    }
}

/* 从播放器取一次状态，刷新进度行与播放/暂停图标；不在播放器屏时相关指针为空，各自跳过 */
static void player_refresh(void)
{
    audio_player_snapshot_t snapshot;
    bool active;

    if(audio_player_snapshot(&snapshot) != AUDIO_RES_OK) return;
    active = snapshot.state == AUDIO_PLAYER_PLAYING ||
        snapshot.state == AUDIO_PLAYER_PAUSED || snapshot.state == AUDIO_PLAYER_SEEKING;

    if(!progress_editing) {    /* 调整态显示的是目标位置，不被播放器实时位置覆盖 */
        progress_pos = (uint16_t)(snapshot.position_ms / 1000u);
        progress_dur = (uint16_t)(snapshot.duration_ms / 1000u);
        progress_apply();
    }

    if(playpause_icon != NULL) {
        player_playing = snapshot.state == AUDIO_PLAYER_PLAYING ||
            snapshot.state == AUDIO_PLAYER_SEEKING;
        lv_image_set_src(playpause_icon, player_playing ? pause : play);
    }

    meta_refresh(&snapshot, active);
    lyric_refresh(&snapshot, active);
}

/* 定时器回调：只在本屏有播放键时才刷新，其它屏直接返回 */
static void player_refresh_cb(lv_timer_t * t)
{
    LV_UNUSED(t);
    if(playpause_icon == NULL) return;
    player_refresh();
}

/* 播放列表键回调：进入文件浏览屏 */
static void playlist_open_cb(lv_obj_t * obj, void * user_data)
{
    LV_UNUSED(obj);
    LV_UNUSED(user_data);
    (void)browser_controller_open(&(browser_controller_request_t) {
        .root_path = "C:/", .allow_directories = true,
        .filter_cb = music_browser_filter, .select_cb = music_browser_select,
        .return_route = SCREEN_PLAYER,
    });
}

static bool music_browser_filter(const char * name, bool is_directory, void * user_data)
{
    LV_UNUSED(user_data);
    return is_directory || audio_player_supports(name);
}

static void music_browser_select(const char * path, void * user_data)
{
    audio_res_t result;
    LV_UNUSED(user_data);
    result = audio_player_play(path);
    if(result != AUDIO_RES_OK) {
        LV_LOG_USER("player rejected (%s): %s", audio_res_str(result), path);
        return;
    }
    ui_goto(SCREEN_PLAYER, true);
}

static void firmware_browser_select(const char * path, void * user_data)
{
    const char *name = path;
    const char *p;
    LV_UNUSED(user_data);
    for(p = path; *p != '\0'; p++) if(*p == '/' || *p == ':') name = p + 1;
    BootShared_Update((const uint8_t *)name);
}

/* 电源键回调：停止播放（电源语义未定，先借它给界面一个结束播放的入口） */
static void power_cb(lv_obj_t * obj, void * user_data)
{
    LV_UNUSED(obj);
    LV_UNUSED(user_data);
    (void)audio_player_stop();
}

/**
 * @brief 判断文本该用哪份中日文字体
 * @param text UTF-8文本，可为NULL
 * @return 含平/片假名返回日文字体 cjk_jp_12，其余含CJK字符的文本返回简体字体 cjk_sc_12；
 *         纯ASCII等非CJK文本返回NULL（保持标签在XML里设的字体）
 */
static lv_font_t * cjk_font_for_text(const char * text)
{
    const uint8_t * p = (const uint8_t *)text;
    bool has_cjk = false;

    if(p == NULL) return NULL;

    while(*p != '\0') {
        uint32_t cp;

        if(*p < 0x80) {                 /* ASCII：跳过 */
            p++;
            continue;
        }
        if((*p & 0xE0) == 0xC0) {       /* 2字节序列 */
            cp = (uint32_t)((*p & 0x1F) << 6) | (uint32_t)(p[1] & 0x3F);
            p += 2;
        }
        else if((*p & 0xF0) == 0xE0) {  /* 3字节序列：汉字与假名都在这里 */
            cp = (uint32_t)((*p & 0x0F) << 12) | (uint32_t)((p[1] & 0x3F) << 6) |
                (uint32_t)(p[2] & 0x3F);
            p += 3;
        }
        else {                          /* 4字节序列或非法字节：不在CJK常用区，跳过 */
            p += ((*p & 0xF8) == 0xF0) ? 4 : 1;
            continue;
        }

        /* 平假名(0x3040-0x309F)与片假名(0x30A0-0x30FF)：命中即用日文字形，无需继续扫 */
        if(cp >= 0x3040 && cp <= 0x30FF) {
            return (cjk_jp_14 != NULL) ? cjk_jp_14 : cjk_sc_14;
        }

        /* CJK部首/标点(0x2E80-0x303F)、扩展A与基本区(0x3400-0x9FFF)、
           兼容汉字(0xF900-0xFAFF)、全角与半角形式(0xFF00-0xFFEF) */
        if((cp >= 0x2E80 && cp <= 0x303F) || (cp >= 0x3400 && cp <= 0x9FFF) ||
            (cp >= 0xF900 && cp <= 0xFAFF) || (cp >= 0xFF00 && cp <= 0xFFEF)) {
            has_cjk = true;
        }
    }

    return has_cjk ? cjk_sc_14 : NULL;      /* 中日文用简体字形；纯ASCII交回XML里的字体 */
}

/* 焦点变化监听：焦点在端点继续同方向滚动会回绕到另一端，此时视为遍历完成，切屏 */
static void focus_watch_cb(lv_event_t * e)
{
    lv_obj_t * obj = lv_event_get_current_target_obj(e);
    int32_t i;
    for(i = 0; i < (int32_t)focus_cnt; i++) {
        if(focus_list[i] == obj) break;
    }
    if(i >= (int32_t)focus_cnt) return;    /* 获焦对象不在列表内 */

    if(focus_idx >= 0 && focus_cnt > 1) {
        bool wrap_tail = (focus_idx == (int32_t)focus_cnt - 1) && (i == 0);                  /* 从尾回绕到头 */
        bool wrap_head = (focus_idx == 0) && (i == (int32_t)focus_cnt - 1);                  /* 从头回绕到尾 */
        if(wrap_tail || wrap_head) {
            focus_idx = (int8_t)i;
            H7_MediaPlayer_UI_switch_screen(wrap_tail);    /* 尾→头为正向，头→尾为反向 */
            return;
        }
    }
    focus_idx = (int8_t)i;
}
