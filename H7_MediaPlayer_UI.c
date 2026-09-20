/**
 * @file H7_MediaPlayer_UI.c
 */

/*********************
 *      INCLUDES
 *********************/

#include "H7_MediaPlayer_UI.h"
#include "audio_player.h"
#include "../lvgl_fatfs.h"
#include <stdint.h>

/*********************
 *      DEFINES
 *********************/

#define VOLUME_LEVEL_MAX        100     /* 音量上限（%） */
#define VOLUME_STEP             1       /* 每个棘轮格的音量步进（%） */
#define VOLUME_ICON_HIGH_MIN    50      /* 音量达到该值起显示大音量图标；0 显示静音图标，其余显示小音量图标 */

#define PLAYER_REFRESH_PERIOD   200     /* 播放器状态（进度/时间/播放暂停）刷新周期（ms） */

#define BROWSER_ROOT_PATH       "C:/"   /* 文件浏览的起始目录，盘符与 lvgl_fatfs.h 的 LVGL_FATFS_LETTER 一致 */
#define BROWSER_ENTRY_MAX       64      /* 单个目录最多列出多少行，防止超大目录把堆占满 */

#ifndef H7_MEDIAPLAYER_FIRMWARE_VERSION
#define H7_MEDIAPLAYER_FIRMWARE_VERSION "v0.1.0"
#endif

#define CJK_FONT_LINE_HEIGHT    19      // 中日文位图字体的每行高度（14号烘焙行高约27）
#define CJK_FONT_BASE_LINE      4       // 基线下方留白（14号烘焙值约8）

/**********************
 *      TYPEDEFS
 **********************/

/* 屏标识：主屏与播放器屏之间靠旋转循环，文件浏览屏由播放列表键进入、由返回行退出 */
typedef enum {
    SCREEN_HOME,
    SCREEN_LAUNCHER,
    SCREEN_SETTINGS,
    SCREEN_ABOUT,
    SCREEN_APP_PLACEHOLDER,
    SCREEN_PLAYER,
    SCREEN_BROWSER,
} screen_id_t;

typedef struct {
    const char * name;
} launcher_app_t;

/* 文件浏览列表行的类型，决定按下后干什么 */
typedef enum {
    BROWSER_ENTRY_NONE,     /* 不可操作行（打开失败、条目截断提示） */
    BROWSER_ENTRY_PARENT,   /* 上一级目录 */
    BROWSER_ENTRY_DIR,      /* 子目录 */
    BROWSER_ENTRY_FILE,     /* 普通文件；音乐选曲模式下仅创建支持的音频文件行 */
} browser_entry_kind_t;

typedef enum {
    BROWSER_MODE_FILES,
    BROWSER_MODE_MUSIC,
} browser_mode_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void switch_done_cb(lv_event_t * e);
static void playpause_cb(lv_event_t * e);
static void prev_track_cb(lv_event_t * e);
static void next_track_cb(lv_event_t * e);
static void repeat_mode_cb(lv_event_t * e);
static const void * repeat_mode_icon(uint8_t mode);
static void volume_toggle_cb(lv_event_t * e);
static void volume_key_cb(lv_event_t * e);
static void progress_toggle_cb(lv_event_t * e);
static void progress_key_cb(lv_event_t * e);
static void power_cb(lv_event_t * e);
static void focus_watch_cb(lv_event_t * e);

static void volume_apply(void);
static void progress_apply(void);
static void player_refresh(void);
static void player_refresh_cb(lv_timer_t * t);
static void meta_refresh(const audio_player_snapshot_t * snapshot, bool active);
static void lyric_refresh(const audio_player_snapshot_t * snapshot, bool active);

static void ui_goto(screen_id_t id, bool forward);
static void playlist_open_cb(lv_event_t * e);
static void browser_entry_cb(lv_event_t * e);
static void browser_file_double_cb(lv_event_t * e);
static void browser_delete_cb(lv_event_t * e);
static void browser_back_cb(lv_event_t * e);
static void browser_menu_close_cb(lv_event_t * e);
static void player_back_cb(lv_event_t * e);
static void home_open_launcher_cb(lv_event_t * e);
static void launcher_app_cb(lv_event_t * e);
static void launcher_back_cb(lv_event_t * e);
static void launcher_focus_cb(lv_event_t * e);
static void settings_about_cb(lv_event_t * e);
static void settings_back_cb(lv_event_t * e);
static void about_back_cb(lv_event_t * e);
static void placeholder_back_cb(lv_event_t * e);
static void launcher_bind(lv_obj_t * screen);
static void settings_bind(lv_obj_t * screen);
static void about_bind(lv_obj_t * screen);
static void placeholder_bind(lv_obj_t * screen);
static void launcher_update_page(lv_obj_t * screen);

static void browser_bind(lv_obj_t * screen);
static void browser_scan(void);
static void browser_focus_first(void);
static lv_obj_t * browser_add_row(const char * icon, const char * text, browser_entry_kind_t kind);
static lv_font_t * cjk_font_for_text(const char * text);
static void cjk_font_line_height_compact(void);
static void obj_set_hidden(lv_obj_t * obj, bool hidden);
static const char * browser_row_name(lv_obj_t * btn);
static bool browser_dir_enter(const char * name);
static bool browser_dir_up(void);
static void browser_menu_close(bool restore_focus);
static void browser_menu_focus_scope(bool menu_only);

/**********************
 *  STATIC VARIABLES
 **********************/

/* 烘焙字体数据本身（const，编辑器生成）：gen.h 里只有字体指针，这里自己声明 */
extern const lv_font_t cjk_sc_12_data;
extern const lv_font_t cjk_jp_12_data;

/* 行高压缩用的可写字体描述符副本：生成的 cjk_*_data 是 const，改不了，拷贝一份出来改 */
static lv_font_t cjk_sc_12_compact;
static lv_font_t cjk_jp_12_compact;

/* 默认字体指针，供固件 Applications/lvgl/lv_conf.h 的 LV_FONT_DEFAULT 使用
   （预览侧的 lv_conf.h 不引用它，所以预览用 montserrat_12 作默认字体）。
   指向下面行高压缩后的副本，必须在首次绘制前填好 —— 见 cjk_font_line_height_compact() */
const lv_font_t * const lv_ui_font_default = &cjk_sc_12_compact;

static screen_id_t cur_screen = SCREEN_HOME;    /* 当前所在屏（切屏入口唯一，用它记忆状态） */
static bool switching;           /* 屏幕过渡进行中，忽略边界回调的重复触发 */
static bool switch_forward = true;  /* 本次切屏方向：true=向右（正向），false=向左（反向） */
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

static lv_obj_t * browser_list;     /* 文件浏览屏的列表容器（不在浏览屏时为 NULL） */
static lv_obj_t * browser_path;     /* 文件浏览屏顶部的当前路径标签 */
static lv_obj_t * browser_menu_overlay;
static lv_obj_t * browser_menu_name;
static lv_obj_t * browser_menu_delete;
static lv_obj_t * browser_menu_source;
static char browser_dir[LV_FS_MAX_PATH_LENGTH];     /* 当前目录，含盘符且恒以 '/' 结尾（如 "C:/Music/"） */
static char browser_file[LV_FS_MAX_PATH_LENGTH];    /* 最近选中的文件，预留：接入播放器后用它开播 */
static char browser_menu_file[LV_FS_MAX_PATH_LENGTH];
static uint8_t browser_row_kind[BROWSER_ENTRY_MAX]; /* 各行的类型，下标与列表子项顺序一致 */
static browser_mode_t browser_mode = BROWSER_MODE_FILES;
static bool browser_menu_open;

static lv_obj_t * focus_list[12];   /* 本屏焦点遍历顺序（XML 创建顺序），容量留余量 */
static uint8_t focus_cnt;           /* 焦点列表长度 */
static int8_t focus_idx;            /* 当前焦点索引，-1=未知 */

static const launcher_app_t launcher_apps[] = {
	{ "设置" },
	{ "音乐" },
	{ "文件" },
	{ "图片" },
	{ "视频" },
	{ "相机" },
	{ "系统监视器" },
};
#define LAUNCHER_APP_COUNT ((uint8_t)(sizeof(launcher_apps) / sizeof(launcher_apps[0])))
#define LAUNCHER_PAGE_SIZE 4u
static uint8_t launcher_selected;   /* 全局应用序号；页面由它自动推导，顺序始终为行优先 */
static bool launcher_focus_guard;

/**********************
 *      MACROS
 **********************/

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

    /* 周期把播放器真实状态同步到播放器屏（不在播放器屏时回调直接返回） */
    lv_timer_create(player_refresh_cb, PLAYER_REFRESH_PERIOD, NULL);
}

void H7_MediaPlayer_UI_switch_screen(bool forward)
{
    /* 应用选择器由焦点组自身回环；不再通过旋转在不同业务屏之间切换。 */
    LV_UNUSED(forward);
}

void H7_MediaPlayer_UI_bind(lv_obj_t * screen)
{
    if(screen == NULL) return;

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

    /* 文件浏览屏的内容是按目录动态生成的列表，不参与"回绕切屏"，单独接线 */
    if(cur_screen == SCREEN_BROWSER) {
        browser_bind(screen);
        return;
    }
    if(cur_screen == SCREEN_LAUNCHER) {
        launcher_bind(screen);
        return;
    }
    if(cur_screen == SCREEN_SETTINGS) {
        settings_bind(screen);
        return;
    }
    if(cur_screen == SCREEN_ABOUT) {
        about_bind(screen);
        return;
    }
    if(cur_screen == SCREEN_APP_PLACEHOLDER) {
        placeholder_bind(screen);
        return;
    }

    if(cur_screen == SCREEN_HOME) {
        lv_obj_t * entry = lv_obj_find_by_name(screen, "text_focus");
        if(entry != NULL) {
            /* 尺寸与透明样式均在 home.xml；这里仅绑定行为。 */
            lv_obj_add_event_cb(entry, home_open_launcher_cb, LV_EVENT_SHORT_CLICKED, NULL);
            lv_group_focus_obj(entry);
        }
        return;
    }
    browser_list = NULL;    /* 离开浏览屏后清空，避免残留指向已删除屏的控件 */
    browser_path = NULL;
    browser_menu_overlay = NULL;
    browser_menu_name = NULL;
    browser_menu_delete = NULL;
    browser_menu_source = NULL;
    browser_menu_open = false;

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
            lv_obj_add_event_cb(obj, player_back_cb, LV_EVENT_LONG_PRESSED, NULL);
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
        lv_obj_add_event_cb(btn, playpause_cb, LV_EVENT_SHORT_CLICKED, NULL);
    }
    else {
        playpause_icon = NULL;
    }

    /* 播放器：上一曲/下一曲（预留接口）与循环模式切换 */
    lv_obj_t * btn_prev = lv_obj_find_by_name(screen, "btn_prev");
    if(btn_prev != NULL) lv_obj_add_event_cb(btn_prev, prev_track_cb, LV_EVENT_SHORT_CLICKED, NULL);

    lv_obj_t * btn_next = lv_obj_find_by_name(screen, "btn_next");
    if(btn_next != NULL) lv_obj_add_event_cb(btn_next, next_track_cb, LV_EVENT_SHORT_CLICKED, NULL);

    lv_obj_t * btn_repeat = lv_obj_find_by_name(screen, "btn_repeat");
    if(btn_repeat != NULL) {
        /* 循环模式跨屏保留（repeat_mode 是静态变量）：本屏图标按已保存的模式重建，
           再把模式同步给播放器，避免重进播放器屏时图标与实际模式不一致 */
        lv_obj_t * repeat_icon = lv_obj_get_child(btn_repeat, 0);
        if(repeat_icon != NULL) lv_image_set_src(repeat_icon, repeat_mode_icon(repeat_mode));

        (void)audio_player_set_repeat((audio_repeat_t)repeat_mode);
        lv_obj_add_event_cb(btn_repeat, repeat_mode_cb, LV_EVENT_SHORT_CLICKED, NULL);
    }

    /* 播放器：播放列表键进入文件浏览屏 */
    lv_obj_t * btn_playlist = lv_obj_find_by_name(screen, "btn_playlist");
    if(btn_playlist != NULL) lv_obj_add_event_cb(btn_playlist, playlist_open_cb, LV_EVENT_SHORT_CLICKED, NULL);

    /* 播放器：电源键暂接成"停止播放"（电源语义未定），给界面一个结束播放的入口 */
    lv_obj_t * btn_power = lv_obj_find_by_name(screen, "btn_power");
    if(btn_power != NULL) lv_obj_add_event_cb(btn_power, power_cb, LV_EVENT_SHORT_CLICKED, NULL);

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
        lv_obj_set_flag(volume_popup, LV_OBJ_FLAG_SCROLLABLE, false);   /* 避免内容超界时画出滚动条 */
    }

    lv_obj_t * btn_volume = lv_obj_find_by_name(screen, "btn_volume");
    if(btn_volume != NULL) {
        volume_icon = lv_obj_get_child(btn_volume, 0);    /* 按钮内唯一子控件即音量图标 */
        lv_obj_add_event_cb(btn_volume, volume_toggle_cb, LV_EVENT_SHORT_CLICKED, NULL);
        lv_obj_add_event_cb(btn_volume, volume_key_cb, LV_EVENT_KEY, NULL);
    }
    else {
        volume_icon = NULL;
    }

    /* 播放器：进度条与播放时间；数值由 player_refresh 按播放器状态刷新
       （时间字号由 XML 的 style_text_font="montserrat_10" 设置，保证编辑器预览一致） */
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
        lv_obj_add_event_cb(track, progress_toggle_cb, LV_EVENT_SHORT_CLICKED, NULL);
        lv_obj_add_event_cb(track, progress_key_cb, LV_EVENT_KEY, NULL);
    }

    volume_apply();
    player_refresh();
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/** @brief 显示/隐藏控件（使用 LVGL 9.5 的专用 flag setter）。 */
static void obj_set_hidden(lv_obj_t * obj, bool hidden)
{
	if(obj == NULL) return;
	lv_obj_set_flag(obj, LV_OBJ_FLAG_HIDDEN, hidden);
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
	cjk_sc_12_compact = cjk_sc_12_data;
	cjk_sc_12_compact.line_height = CJK_FONT_LINE_HEIGHT;
	cjk_sc_12_compact.base_line = CJK_FONT_BASE_LINE;
	cjk_jp_12_compact = cjk_jp_12_data;
	cjk_jp_12_compact.line_height = CJK_FONT_LINE_HEIGHT;
	cjk_jp_12_compact.base_line = CJK_FONT_BASE_LINE;

	/* 各屏的 create 读的就是这两个指针，换掉即全局生效（含默认字体） */
	cjk_sc_14 = &cjk_sc_12_compact;
	cjk_jp_14 = &cjk_jp_12_compact;
}

/* 切屏统一入口：创建目标屏、绑定、淡入过渡，旧屏在过渡结束后由 auto_del 删除 */
static void ui_goto(screen_id_t id, bool forward)
{
    /* 过渡期间忽略重复触发；已经在该屏则不动作 */
    if(switching || id == cur_screen) return;

    /* 记下方向供 bind 决定新屏焦点落点：正向落首项，反向落末项 */
    switch_forward = forward;

    /* 三屏均非 permanent，每次切换都重新创建 */
    lv_obj_t * next = NULL;
    switch(id) {
        case SCREEN_LAUNCHER: next = launcher_create(); break;
        case SCREEN_SETTINGS: next = settings_create(); break;
        case SCREEN_ABOUT: next = about_create(); break;
        case SCREEN_APP_PLACEHOLDER: next = app_placeholder_create(); break;
        case SCREEN_PLAYER:  next = player_create();  break;
        case SCREEN_BROWSER: next = browser_create(); break;
        default:             next = home_create();    break;
    }
    if(next == NULL) return;

    cur_screen = id;
    switching = true;
    H7_MediaPlayer_UI_bind(next);

    /* 挂在旧屏上：过渡完成（旧屏卸载）后解除防重，旧屏随后被 auto_del 删除 */
    lv_obj_add_event_cb(lv_screen_active(), switch_done_cb, LV_EVENT_SCREEN_UNLOADED, NULL);
    lv_screen_load_anim(next, LV_SCR_LOAD_ANIM_FADE_ON, 300, 0, true);
}

/* 过渡完成回调：解除切屏防重 */
static void switch_done_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    switching = false;
}

/* 主页面短按：进入应用选择器时始终从左上角的“设置”开始。 */
static void home_open_launcher_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    launcher_selected = 0u;
    ui_goto(SCREEN_LAUNCHER, true);
}

/* 应用磁贴短按只进入对应的占位页；后续接入业务时在此按序号分发即可。 */
static void launcher_app_cb(lv_event_t * e)
{
    launcher_selected = (uint8_t)(uintptr_t)lv_event_get_user_data(e);
    if(launcher_selected == 0u) {
        ui_goto(SCREEN_SETTINGS, true);
    } else if(launcher_selected == 1u) {
        ui_goto(SCREEN_PLAYER, true);
    } else if(launcher_selected == 2u) {
        browser_mode = BROWSER_MODE_FILES;
        ui_goto(SCREEN_BROWSER, true);
    } else {
        ui_goto(SCREEN_APP_PLACEHOLDER, true);
    }
}

/* 设置页统一采用长按返回；具体设置动作在各功能接入时再绑定。 */
static void settings_back_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    ui_goto(SCREEN_LAUNCHER, false);
}

static void settings_about_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    ui_goto(SCREEN_ABOUT, true);
}

static void about_back_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    ui_goto(SCREEN_SETTINGS, false);
}

/* 选择器长按退出至纯背景主页面。 */
static void launcher_back_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    ui_goto(SCREEN_HOME, false);
}

/* 当前页首尾回绕时，切换 XML 页面容器，并继续聚焦全局序列中的相邻应用。 */
static void launcher_focus_cb(lv_event_t * e)
{
    uint8_t focused = (uint8_t)(uintptr_t)lv_event_get_user_data(e);
    uint8_t page = launcher_selected / LAUNCHER_PAGE_SIZE;
    uint8_t first = page * LAUNCHER_PAGE_SIZE;
    uint8_t last = first + LAUNCHER_PAGE_SIZE - 1u;
    lv_obj_t * screen;
    char name[24];

    if(launcher_focus_guard) return;
    if(last >= LAUNCHER_APP_COUNT) last = LAUNCHER_APP_COUNT - 1u;

    if(launcher_selected == last && focused == first) {
        launcher_selected = (uint8_t)((last + 1u) % LAUNCHER_APP_COUNT);
    } else if(launcher_selected == first && focused == last) {
        launcher_selected = (uint8_t)((first + LAUNCHER_APP_COUNT - 1u) % LAUNCHER_APP_COUNT);
    } else {
        launcher_selected = focused;
        return;
    }

    screen = lv_obj_get_screen(lv_event_get_target(e));
    launcher_update_page(screen);
    lv_snprintf(name, sizeof(name), "launcher_tile_%u", (unsigned)launcher_selected);
    lv_obj_t * target = lv_obj_find_by_name(screen, name);
    if(target != NULL) {
        launcher_focus_guard = true;
        lv_group_focus_obj(target);
        launcher_focus_guard = false;
    }
}

/* 应用占位页长按返回应用选择器，并保留刚才选中的应用。 */
static void placeholder_back_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    ui_goto(SCREEN_LAUNCHER, false);
}

/* 音乐应用返回启动器只离开界面，播放器任务继续运行。 */
static void player_back_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    ui_goto(SCREEN_LAUNCHER, false);
}

static void launcher_bind(lv_obj_t * screen)
{
    char name[24];
    lv_obj_t * selected = NULL;
    lv_group_t * group;
    uint8_t i;

    launcher_update_page(screen);
    for(i = 0u; i < LAUNCHER_APP_COUNT; i++) {
        lv_obj_t * tile;
        lv_snprintf(name, sizeof(name), "launcher_tile_%u", (unsigned)i);
        tile = lv_obj_find_by_name(screen, name);
        if(tile == NULL) continue;
        lv_obj_add_event_cb(tile, launcher_app_cb, LV_EVENT_SHORT_CLICKED,
            (void *)(uintptr_t)i);
        lv_obj_add_event_cb(tile, launcher_back_cb, LV_EVENT_LONG_PRESSED, NULL);
        lv_obj_add_event_cb(tile, launcher_focus_cb, LV_EVENT_FOCUSED,
            (void *)(uintptr_t)i);
        if(i == launcher_selected) selected = tile;
    }
    if(selected != NULL) {
        group = lv_obj_get_group(selected);
        if(group != NULL) lv_group_set_wrap(group, true);
        launcher_focus_guard = true;
        lv_group_focus_obj(selected);
        launcher_focus_guard = false;
    }
}

static void settings_bind(lv_obj_t * screen)
{
    static const char * row_names[] = {
        "settings_brightness",
        "settings_orientation",
        "settings_reboot",
        "settings_local_update",
        "settings_about",
    };
    lv_obj_t * first = NULL;
    lv_group_t * group = NULL;

    for(uint32_t i = 0; i < sizeof(row_names) / sizeof(row_names[0]); i++) {
        lv_obj_t * row = lv_obj_find_by_name(screen, row_names[i]);
        if(row == NULL) continue;
        if(first == NULL) first = row;
        lv_obj_add_event_cb(row, settings_back_cb, LV_EVENT_LONG_PRESSED, NULL);
        if(i == 4u) lv_obj_add_event_cb(row, settings_about_cb, LV_EVENT_SHORT_CLICKED, NULL);
    }

    if(first != NULL) {
        group = lv_obj_get_group(first);
        if(group != NULL) lv_group_set_wrap(group, true);
        lv_group_focus_obj(first);
    }
}

static void about_bind(lv_obj_t * screen)
{
    static const char * row_names[] = {
        "about_intro",
        "about_repository",
        "about_version_row",
        "about_build_row",
    };
    lv_obj_t * version = lv_obj_find_by_name(screen, "about_firmware_version");
    lv_obj_t * build_time = lv_obj_find_by_name(screen, "about_build_time");
    lv_obj_t * first = NULL;
    lv_group_t * group = NULL;

    if(version != NULL) lv_label_set_text(version, H7_MEDIAPLAYER_FIRMWARE_VERSION);
    if(build_time != NULL) lv_label_set_text(build_time, __DATE__ " " __TIME__);

    for(uint32_t i = 0; i < sizeof(row_names) / sizeof(row_names[0]); i++) {
        lv_obj_t * row = lv_obj_find_by_name(screen, row_names[i]);
        if(row == NULL) continue;
        if(first == NULL) first = row;
        lv_obj_add_event_cb(row, about_back_cb, LV_EVENT_LONG_PRESSED, NULL);
    }

    if(first != NULL) {
        group = lv_obj_get_group(first);
        if(group != NULL) lv_group_set_wrap(group, true);
        lv_group_focus_obj(first);
    }
}

/* XML 定义两页的全部视觉内容；运行时只切换对应容器和页码指示器。 */
static void launcher_update_page(lv_obj_t * screen)
{
    uint8_t active_page = launcher_selected / LAUNCHER_PAGE_SIZE;
    lv_obj_t * page0 = lv_obj_find_by_name(screen, "launcher_page_0");
    lv_obj_t * page1 = lv_obj_find_by_name(screen, "launcher_page_1");
    lv_obj_t * dots0 = lv_obj_find_by_name(screen, "launcher_dots_page_0");
    lv_obj_t * dots1 = lv_obj_find_by_name(screen, "launcher_dots_page_1");
    obj_set_hidden(page0, active_page != 0u);
    obj_set_hidden(page1, active_page != 1u);
    obj_set_hidden(dots0, active_page != 0u);
    obj_set_hidden(dots1, active_page != 1u);
}

static void placeholder_bind(lv_obj_t * screen)
{
    lv_obj_t * back = lv_obj_find_by_name(screen, "app_placeholder_back");
    lv_obj_t * title = lv_obj_find_by_name(screen, "app_placeholder_title");
    if(title != NULL) lv_label_set_text(title, launcher_apps[launcher_selected].name);
    if(back == NULL) return;
    lv_obj_add_event_cb(back, placeholder_back_cb, LV_EVENT_LONG_PRESSED, NULL);
    lv_group_focus_obj(back);
}

/* 播放/暂停键回调：正在播放则切换暂停；空闲且此前选过文件则重新起播。
   图标与进度不在这里改，统一由 player_refresh 按播放器真实状态刷新 */
static void playpause_cb(lv_event_t * e)
{
    audio_player_snapshot_t snapshot;

    LV_UNUSED(e);
    if(audio_player_snapshot(&snapshot) != AUDIO_RES_OK) return;
    if(snapshot.state == AUDIO_PLAYER_PAUSED) {
        (void)audio_player_resume();
    }
    else if(snapshot.state == AUDIO_PLAYER_PLAYING) {
        (void)audio_player_pause();
    }
    else if((snapshot.state == AUDIO_PLAYER_IDLE || snapshot.state == AUDIO_PLAYER_ERROR) &&
        browser_file[0] != '\0') {
        (void)audio_player_play(browser_file);
    }
}

/* 上一曲按钮回调：在播放列表内往前切一首 */
static void prev_track_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    (void)audio_player_previous();
}

/* 下一曲按钮回调：在播放列表内往后切一首 */
static void next_track_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    (void)audio_player_next();
}

/* 取循环模式对应的图标：图片符号是指针变量，值链接期才确定，不能用 static 数组初始化 */
static const void * repeat_mode_icon(uint8_t mode)
{
    const void * modes[4] = { queue, repeat, repeat_one, shuffle };

    return modes[mode % 4];
}

/* 循环模式按钮回调：列表顺序 → 列表循环 → 单曲循环 → 随机，点击循环切换图标 */
static void repeat_mode_cb(lv_event_t * e)
{
    lv_obj_t * btn = lv_event_get_current_target_obj(e);
    lv_obj_t * icon = lv_obj_get_child(btn, 0);
    if(icon == NULL) return;

    repeat_mode = (uint8_t)((repeat_mode + 1) % 4);
    lv_image_set_src(icon, repeat_mode_icon(repeat_mode));
    (void)audio_player_set_repeat((audio_repeat_t)repeat_mode);
}

/* 音量键回调：按下展开/收起音量条，并同步切换焦点组的编辑态
   （编辑态下编码器旋转才会以 LV_KEY_LEFT/RIGHT 发给焦点对象，而不是移动焦点） */
static void volume_toggle_cb(lv_event_t * e)
{
    lv_obj_t * btn = lv_event_get_current_target_obj(e);
    lv_group_t * group = lv_obj_get_group(btn);
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

/* 把当前播放进度应用到界面：进度条按百分比拉伸，时间文本显示 已播 / 总时长。
   总时长未知（0）时进度条保持空，只显示时间 */
static void progress_apply(void)
{
    int32_t pct = 0;
    if(progress_dur > 0) {
        pct = (int32_t)progress_pos * 100 / (int32_t)progress_dur;
        if(pct > 100) pct = 100;
    }
    if(progress_fill != NULL) lv_obj_set_width(progress_fill, lv_pct(pct));
    if(progress_time != NULL) {
        lv_label_set_text_fmt(progress_time, "%02u:%02u / %02u:%02u",
                              (unsigned)(progress_pos / 60), (unsigned)(progress_pos % 60),
                              (unsigned)(progress_dur / 60), (unsigned)(progress_dur % 60));
    }
}

/* 进度条回调：按下进入/退出时间调整态。进入时只做界面预览，不发送 seek；
   只有调整态下旋钮转动（KEY 事件）才真正发 seek 请求 */
static void progress_toggle_cb(lv_event_t * e)
{
    lv_obj_t * track = lv_event_get_current_target_obj(e);
    lv_group_t * group = lv_obj_get_group(track);
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
static void playlist_open_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    browser_mode = BROWSER_MODE_MUSIC;
    ui_goto(SCREEN_BROWSER, true);
}

/* 电源键回调：停止播放（电源语义未定，先借它给界面一个结束播放的入口） */
static void power_cb(lv_event_t * e)
{
    LV_UNUSED(e);
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

/* 文件浏览屏接线：取控件、从盘根目录开始扫描、把焦点放到首行 */
static void browser_bind(lv_obj_t * screen)
{
    browser_list = lv_obj_find_by_name(screen, "browser_list");
    browser_path = lv_obj_find_by_name(screen, "browser_path");
    browser_menu_overlay = lv_obj_find_by_name(screen, "browser_menu_overlay");
    browser_menu_name = lv_obj_find_by_name(screen, "browser_menu_name");
    browser_menu_delete = lv_obj_find_by_name(screen, "browser_menu_delete");
    browser_menu_source = NULL;
    browser_menu_file[0] = '\0';
    browser_menu_open = false;
    if(browser_list != NULL) lv_obj_set_scroll_dir(browser_list, LV_DIR_VER);   /* 只竖向滚动 */
    obj_set_hidden(browser_menu_overlay, true);
    if(browser_menu_delete != NULL) {
        lv_obj_add_event_cb(browser_menu_delete, browser_delete_cb, LV_EVENT_SHORT_CLICKED, NULL);
        lv_obj_add_event_cb(browser_menu_delete, browser_menu_close_cb, LV_EVENT_LONG_PRESSED, NULL);
    }

    lv_snprintf(browser_dir, sizeof(browser_dir), "%s", BROWSER_ROOT_PATH);
    browser_scan();
    browser_focus_first();
}

/* 取文件名的扩展名（不含 '.'）：没有扩展名返回 NULL */
static const char * file_ext(const char * name)
{
    const char * ext = NULL;
    const char * p;
    for(p = name; *p != '\0'; p++) {
        if(*p == '.') ext = p + 1;
    }
    return (ext != NULL && *ext != '\0') ? ext : NULL;
}

/* 扩展名比较：大小写不敏感；right 需为小写（与音频解码器的 extension_equal 同一写法） */
static bool ext_equal(const char * left, const char * right)
{
    while(*left != '\0' && *right != '\0') {
        char a = *left++;
        char b = *right++;
        if(a >= 'A' && a <= 'Z') a = (char)(a + ('a' - 'A'));
        if(a != b) return false;
    }
    return *left == '\0' && *right == '\0';
}

/* 文件行的图标：音频按解码器实际支持的格式判断（与音乐选曲同一套），
   视频/图片按常见扩展名识别，识别不出才用通用文件图标；目录图标由调用处给。
   字形都取自烘焙的 icon_12（LV_SYMBOL_* 的码位在 0xF000-0xF8FF） */
static const char * browser_file_icon(const char * name)
{
    static const char * const video_ext[] = { "mp4", "m4v", "avi", "mkv", "mov", "mjpg", "mjpeg" };
    static const char * const image_ext[] = { "jpg", "jpeg", "png", "bmp", "gif", "webp" };
    const char * ext = file_ext(name);
    uint32_t i;

    if(audio_player_supports(name)) return LV_SYMBOL_AUDIO;
    if(ext == NULL) return LV_SYMBOL_FILE;
    for(i = 0; i < sizeof(video_ext) / sizeof(video_ext[0]); i++) {
        if(ext_equal(ext, video_ext[i])) return LV_SYMBOL_VIDEO;
    }
    for(i = 0; i < sizeof(image_ext) / sizeof(image_ext[0]); i++) {
        if(ext_equal(ext, image_ext[i])) return LV_SYMBOL_IMAGE;
    }
    return LV_SYMBOL_FILE;
}

/* 重建列表：上级目录、子目录和文件。文件应用显示全部文件，音乐选曲只显示支持格式。
   行图标按类型给：目录 / 音频 / 视频 / 图片 / 通用文件。
   目录项由驱动保证以 '/' 开头且已过滤掉 '.' 与 '..'（见 lvgl_fatfs.c 的 fs_dir_read） */
static void browser_scan(void)
{
    if(browser_list == NULL) return;

    lv_obj_clean(browser_list);
    if(browser_path != NULL) {
        lv_label_set_long_mode(browser_path, LV_LABEL_LONG_MODE_DOTS);    /* 路径过长时省略号截断 */
        lv_label_set_text(browser_path, browser_dir);
        lv_font_t * path_font = cjk_font_for_text(browser_dir);    /* 目录名含中文/日文时换字体 */
        if(path_font != NULL) lv_obj_set_style_text_font(browser_path, path_font, 0);
    }

    if(lv_strcmp(browser_dir, BROWSER_ROOT_PATH) != 0) {
        browser_add_row(LV_SYMBOL_UP, "..", BROWSER_ENTRY_PARENT);
    }

    lv_fs_dir_t dir;
    if(lv_fs_dir_open(&dir, browser_dir) != LV_FS_RES_OK) {
        browser_add_row(LV_SYMBOL_WARNING, "open failed", BROWSER_ENTRY_NONE);
        return;
    }

    char fn[LV_FS_MAX_PATH_LENGTH];
    while(lv_fs_dir_read(&dir, fn, sizeof(fn)) == LV_FS_RES_OK && fn[0] != '\0') {
        bool is_dir = (fn[0] == '/');
        const char * name = is_dir ? (fn + 1) : fn;
        if(!is_dir && browser_mode == BROWSER_MODE_MUSIC && !audio_player_supports(name)) continue;

        if(lv_obj_get_child_count(browser_list) >= BROWSER_ENTRY_MAX - 1) {
            browser_add_row(LV_SYMBOL_LIST, "...", BROWSER_ENTRY_NONE);    /* 超出上限，只列前一部分 */
            break;
        }
        browser_add_row(is_dir ? LV_SYMBOL_DIRECTORY : browser_file_icon(name), name,
                        is_dir ? BROWSER_ENTRY_DIR : BROWSER_ENTRY_FILE);
    }
    lv_fs_dir_close(&dir);
    if(lv_obj_get_child_count(browser_list) == 0u) {
        browser_add_row(LV_SYMBOL_LIST, "empty", BROWSER_ENTRY_NONE);
    }
}

/* 把焦点放到首行：切屏后旧屏焦点对象被删时会触发组重定位，这里显式定住 */
static void browser_focus_first(void)
{
    if(browser_list == NULL) return;

    lv_obj_t * first = lv_obj_get_child(browser_list, 0);
    if(first != NULL) lv_group_focus_obj(first);
}

/* 追加一行：图标 + 名称（名称固定是第 2 个子项，取名字时按这个下标读），并记录行类型 */
static lv_obj_t * browser_add_row(const char * icon, const char * text, browser_entry_kind_t kind)
{
    if(browser_list == NULL) return NULL;

    uint32_t idx = lv_obj_get_child_count(browser_list);
    if(idx >= BROWSER_ENTRY_MAX) return NULL;
    browser_row_kind[idx] = (uint8_t)kind;

    lv_obj_t * btn = lv_button_create(browser_list);
    lv_obj_set_width(btn, lv_pct(100));
    lv_obj_set_flex_flow(btn, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_flex_cross_place(btn, LV_FLEX_ALIGN_CENTER, 0);
    lv_obj_set_style_pad_all(btn, 4, 0);
    lv_obj_set_style_pad_column(btn, 6, 0);
    lv_obj_set_style_radius(btn, 4, 0);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x171717), LV_STATE_FOCUSED);
    lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_STATE_FOCUSED);
    lv_obj_set_style_border_width(btn, 0, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_set_style_border_width(btn, 2, LV_STATE_FOCUSED);
    lv_obj_set_style_border_color(btn, lv_color_hex(0x34d399), LV_STATE_FOCUSED);

    lv_obj_t * ic = lv_label_create(btn);
    lv_label_set_text(ic, icon);
    /* 图标字形只在烘焙的 icon_14 里（LV_SYMBOL_* 的码位在 0xF000-0xF8FF 私有区），
       文件名与图标是两个标签，互不影响 */
    if(icon_14 != NULL) lv_obj_set_style_text_font(ic, icon_14, 0);
    lv_obj_set_style_text_color(ic, lv_color_hex(0xa7f3d0), 0);

    lv_obj_t * name = lv_label_create(btn);
    lv_obj_set_flex_grow(name, 1);
    lv_label_set_long_mode(name, LV_LABEL_LONG_MODE_DOTS);    /* 长文件名省略号截断 */
    lv_label_set_text(name, text);
    lv_obj_set_style_text_color(name, lv_color_hex(0xf5f5f5), 0);
    /* 文件名含中文/日文时换成对应字形（假名走日文，其余走简体）；图标标签用 icon_12，不受影响 */
    lv_font_t * name_font = cjk_font_for_text(text);
    if(name_font != NULL) lv_obj_set_style_text_font(name, name_font, 0);

    lv_obj_add_event_cb(btn, browser_entry_cb, LV_EVENT_SHORT_CLICKED, NULL);
    lv_obj_add_event_cb(btn, browser_back_cb, LV_EVENT_LONG_PRESSED, NULL);
    if(kind == BROWSER_ENTRY_FILE) {
        lv_obj_add_event_cb(btn, browser_file_double_cb, LV_EVENT_DOUBLE_CLICKED, NULL);
    }
    return btn;
}

/* 取行内的文件名：行内固定为 图标(0) + 名称(1) 两个标签 */
static const char * browser_row_name(lv_obj_t * btn)
{
    lv_obj_t * name = lv_obj_get_child(btn, 1);
    return (name != NULL) ? lv_label_get_text(name) : "";
}

/* 进入子目录：browser_dir 恒以 '/' 结尾，直接追加"名字/" */
static bool browser_dir_enter(const char * name)
{
    size_t len = lv_strlen(browser_dir);
    if(len + lv_strlen(name) + 2 > sizeof(browser_dir)) return false;    /* 路径过长，忽略本次进入 */

    lv_snprintf(browser_dir + len, sizeof(browser_dir) - len, "%s/", name);
    return true;
}

/* 回到上一级目录：已在盘根目录时返回 false */
static bool browser_dir_up(void)
{
    size_t len = lv_strlen(browser_dir);
    if(len <= sizeof(BROWSER_ROOT_PATH) - 1) return false;    /* "C:/" 已是最上层 */

    len--;                                                 /* 去掉末尾 '/' */
    while(len > 0 && browser_dir[len - 1] != '/') len--;    /* 回退到上一级分隔符之后 */
    browser_dir[len] = '\0';
    return true;
}

/* 行回调：按行类型决定动作 */
static void browser_entry_cb(lv_event_t * e)
{
    lv_obj_t * btn = lv_event_get_current_target_obj(e);
    int32_t idx = lv_obj_get_index(btn);
    audio_res_t result;
    if(idx < 0 || idx >= BROWSER_ENTRY_MAX) return;

    switch((browser_entry_kind_t)browser_row_kind[idx]) {
        case BROWSER_ENTRY_PARENT:
            if(browser_dir_up()) {
                browser_scan();
                browser_focus_first();
            }
            break;

        case BROWSER_ENTRY_DIR:
            if(browser_dir_enter(browser_row_name(btn))) {
                browser_scan();
                browser_focus_first();
            }
            break;

        case BROWSER_ENTRY_FILE:
            if(browser_mode != BROWSER_MODE_MUSIC) break;
            lv_snprintf(browser_file, sizeof(browser_file), "%s%s", browser_dir, browser_row_name(btn));
            result = audio_player_play(browser_file);
            if(result != AUDIO_RES_OK) {
                LV_LOG_USER("player rejected (%s): %s", audio_res_str(result), browser_file);
            }
            ui_goto(SCREEN_PLAYER, true);
            break;

        default:
            break;
    }
}

/* 菜单打开/关闭时同步焦点组成员：打开时把背景列表行移出焦点组，
   旋钮只能在菜单内的控件之间遍历，不会跑到被遮住的文件行上；
   关闭时按列表顺序把行加回去（lv_group_add_obj 自己会先移除，顺序即列表顺序） */
static void browser_menu_focus_scope(bool menu_only)
{
    lv_group_t * group = lv_group_get_default();
    uint32_t i;
    uint32_t cnt;

    if(group == NULL || browser_list == NULL) return;

    cnt = lv_obj_get_child_count(browser_list);
    for(i = 0; i < cnt; i++) {
        lv_obj_t * row = lv_obj_get_child(browser_list, i);
        if(row == NULL) continue;
        if(menu_only) lv_group_remove_obj(row);
        else lv_group_add_obj(group, row);
    }
}

/* 文件应用中双击当前文件，呼出 XML 定义的上下文菜单。 */
static void browser_file_double_cb(lv_event_t * e)
{
    lv_obj_t * btn = lv_event_get_current_target_obj(e);
    if(browser_mode != BROWSER_MODE_FILES || browser_menu_overlay == NULL || btn == NULL) return;

    browser_menu_source = btn;
    lv_snprintf(browser_menu_file, sizeof(browser_menu_file), "%s%s", browser_dir, browser_row_name(btn));
    if(browser_menu_name != NULL) lv_label_set_text(browser_menu_name, browser_row_name(btn));
    obj_set_hidden(browser_menu_overlay, false);
    browser_menu_open = true;
    lv_obj_move_foreground(browser_menu_overlay);
    browser_menu_focus_scope(true);     /* 先把文件行移出焦点组，再定焦到菜单里 */
    if(browser_menu_delete != NULL) lv_group_focus_obj(browser_menu_delete);
}

static void browser_menu_close(bool restore_focus)
{
    obj_set_hidden(browser_menu_overlay, true);
    browser_menu_open = false;
    browser_menu_focus_scope(false);    /* 文件行重新参与焦点遍历 */
    if(restore_focus && browser_menu_source != NULL) lv_group_focus_obj(browser_menu_source);
    browser_menu_source = NULL;
    browser_menu_file[0] = '\0';
}

static void browser_menu_close_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    browser_menu_close(true);
}

/* 删除只针对普通文件；目录删除以后需要单独设计确认与递归策略。 */
static void browser_delete_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    if(browser_menu_file[0] == '\0') return;

    if(lvgl_fatfs_remove(browser_menu_file)) {
        browser_menu_close(false);
        browser_scan();
        browser_focus_first();
    } else if(browser_menu_name != NULL) {
		lv_label_set_text(browser_menu_name,
					  "\xE5\x88\xA0\xE9\x99\xA4\xE5\xA4\xB1\xE8\xB4\xA5");
    }
}

/* 文件浏览器长按返回；若菜单已打开则只关闭菜单。 */
static void browser_back_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    if(browser_menu_open) {
        browser_menu_close(true);
        return;
    }
    ui_goto(browser_mode == BROWSER_MODE_MUSIC ? SCREEN_PLAYER : SCREEN_LAUNCHER, false);
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
