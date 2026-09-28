#include "ui_focus.h"

#define UI_FOCUS_ROW_MAX 96		// 登记槽上限：文件浏览最长 64 行，换屏过渡期间新旧屏的行会同时存在
#define UI_FOCUS_CONFIRM_DELAY 230	// 单击确认延迟（ms），需略大于编码器的双击判定窗口

/**
 * 一行的按键行为登记项：对象删除后回收，供新行复用
 */
typedef struct {
	lv_obj_t * obj;				// 行对象，NULL 表示该槽位空闲
	ui_focus_action_cb_t click_cb;		// 单击确认动作
	ui_focus_action_cb_t back_cb;		// 双击返回上一级动作
	ui_focus_action_cb_t long_cb;		// 该行长按动作，NULL 表示用全局长按动作
	void * user_data;			// 回调附带数据
} ui_focus_row_t;

static ui_focus_row_t rows[UI_FOCUS_ROW_MAX];
static ui_focus_action_cb_t home_cb;	// 全局长按动作（返回主页）
static lv_obj_t * pending_obj;		// 已单击、等待双击窗口结束才确认的行
static lv_timer_t * confirm_timer;	// 单击确认定时器

static ui_focus_row_t * row_find(lv_obj_t * obj);
static ui_focus_row_t * row_alloc(lv_obj_t * obj);
static void click_arm(lv_obj_t * obj);
static void click_cancel(void);
static void confirm_timer_cb(lv_timer_t * t);
static void row_short_cb(lv_event_t * e);
static void row_double_cb(lv_event_t * e);
static void row_long_cb(lv_event_t * e);
static void row_delete_cb(lv_event_t * e);
static void ui_focus_scroll_cb(lv_event_t * e);

/**
 * @brief 按对象查找登记项
 * @param obj 行对象
 * @retval 登记项指针，未登记返回 NULL
 */
static ui_focus_row_t * row_find(lv_obj_t * obj)
{
	for(uint32_t i = 0; i < UI_FOCUS_ROW_MAX; i++) {
		if(rows[i].obj == obj) return &rows[i];
	}
	return NULL;
}

/**
 * @brief 取对象对应的登记项，没有则占用一个空槽并挂上三个按键回调
 * @param obj 行对象
 * @retval 登记项指针，槽位耗尽返回 NULL（该行退化为各页自己注册的即时回调）
 */
static ui_focus_row_t * row_alloc(lv_obj_t * obj)
{
	ui_focus_row_t * row = row_find(obj);

	if(row != NULL) return row;
	for(uint32_t i = 0; i < UI_FOCUS_ROW_MAX; i++) {
		if(rows[i].obj != NULL) continue;

		rows[i].obj = obj;
		rows[i].click_cb = NULL;
		rows[i].back_cb = NULL;
		rows[i].long_cb = NULL;
		rows[i].user_data = NULL;
		lv_obj_add_event_cb(obj, row_short_cb, LV_EVENT_SHORT_CLICKED, NULL);
		lv_obj_add_event_cb(obj, row_double_cb, LV_EVENT_DOUBLE_CLICKED, NULL);
		lv_obj_add_event_cb(obj, row_long_cb, LV_EVENT_LONG_PRESSED, NULL);
		lv_obj_add_event_cb(obj, row_delete_cb, LV_EVENT_DELETE, NULL);
		return &rows[i];
	}
	return NULL;
}

/**
 * @brief 记下待确认的行并（重）启动确认定时器：连击只确认最后一个对象
 * @param obj 刚被单击的行
 */
static void click_arm(lv_obj_t * obj)
{
	pending_obj = obj;
	if(confirm_timer == NULL) {
		confirm_timer = lv_timer_create(confirm_timer_cb, UI_FOCUS_CONFIRM_DELAY, NULL);
	}
	else {
		lv_timer_reset(confirm_timer);
		lv_timer_resume(confirm_timer);
	}
}

/**
 * @brief 取消待确认的单击（双击/长按已经处理了这次按键）
 */
static void click_cancel(void)
{
	pending_obj = NULL;
	if(confirm_timer != NULL) lv_timer_pause(confirm_timer);
}

/**
 * @brief 双击窗口结束：此刻还没等到第二次按键，按单击确认处理
 * @param t 定时器
 */
static void confirm_timer_cb(lv_timer_t * t)
{
	lv_obj_t * obj = pending_obj;
	ui_focus_row_t * row;

	lv_timer_pause(t);
	pending_obj = NULL;
	if(obj == NULL) return;

	row = row_find(obj);
	if(row != NULL && row->click_cb != NULL) row->click_cb(obj, row->user_data);
}

/**
 * @brief 短击：只登记，等双击窗口过去后才真正确认
 */
static void row_short_cb(lv_event_t * e)
{
	click_arm(lv_event_get_current_target_obj(e));
}

/**
 * @brief 双击：取消待确认的单击并返回上一级
 */
static void row_double_cb(lv_event_t * e)
{
	lv_obj_t * obj = lv_event_get_current_target_obj(e);
	ui_focus_row_t * row = row_find(obj);

	click_cancel();
	if(row != NULL && row->back_cb != NULL) row->back_cb(obj, row->user_data);
}

/**
 * @brief 长按：取消待确认的单击，执行该行长按动作（未覆盖则回主页）
 */
static void row_long_cb(lv_event_t * e)
{
	lv_obj_t * obj = lv_event_get_current_target_obj(e);
	ui_focus_row_t * row = row_find(obj);

	click_cancel();
	if(row != NULL && row->long_cb != NULL) {
		row->long_cb(obj, row->user_data);
		return;
	}
	if(home_cb != NULL) home_cb(NULL, NULL);
}

/**
 * @brief 行被删除（换屏、列表重建）时回收槽位，并清掉指向它的待确认记录
 */
static void row_delete_cb(lv_event_t * e)
{
	lv_obj_t * obj = lv_event_get_current_target_obj(e);
	ui_focus_row_t * row = row_find(obj);

	if(row != NULL) {
		row->obj = NULL;
		row->click_cb = NULL;
		row->back_cb = NULL;
		row->long_cb = NULL;
		row->user_data = NULL;
	}
	if(pending_obj == obj) pending_obj = NULL;
}

/**
 * @brief 焦点变化时把该行滚入可视区
 */
static void ui_focus_scroll_cb(lv_event_t * e)
{
	lv_obj_t * obj = lv_event_get_current_target_obj(e);
	if(obj != NULL) lv_obj_scroll_to_view(obj, LV_ANIM_ON);
}

void ui_focus_set_home_cb(ui_focus_action_cb_t home_fn)
{
	home_cb = home_fn;
}

void ui_focus_bind_row(lv_obj_t * obj,
                       ui_focus_action_cb_t click_fn,
                       ui_focus_action_cb_t back_fn,
                       void * user_data)
{
	ui_focus_row_t * row;

	if(obj == NULL) return;
	row = row_alloc(obj);
	if(row == NULL) return;

	if(click_fn != NULL) row->click_cb = click_fn;
	if(back_fn != NULL) row->back_cb = back_fn;
	if(user_data != NULL) row->user_data = user_data;
}

void ui_focus_set_row_long_cb(lv_obj_t * obj, ui_focus_action_cb_t long_fn)
{
	ui_focus_row_t * row;

	if(obj == NULL) return;
	row = row_alloc(obj);
	if(row == NULL) return;
	row->long_cb = long_fn;
}

lv_obj_t * ui_focus_bind_names(lv_obj_t * screen,
                               const char * const * names,
                               uint32_t count,
                               bool wrap,
                               ui_focus_action_cb_t back_cb)
{
	lv_obj_t * first = NULL;
	lv_group_t * group = NULL;

	if(screen == NULL || names == NULL) return NULL;

	for(uint32_t i = 0; i < count; i++) {
		lv_obj_t * obj = lv_obj_find_by_name(screen, names[i]);
		if(obj == NULL) continue;
		if(first == NULL) first = obj;
		ui_focus_bind_row(obj, NULL, back_cb, NULL);	// 双击返回；长按走全局动作
		lv_obj_add_event_cb(obj, ui_focus_scroll_cb, LV_EVENT_FOCUSED, NULL);
	}

	if(first != NULL) {
		group = lv_obj_get_group(first);
		if(group != NULL) lv_group_set_wrap(group, wrap);
		lv_group_focus_obj(first);
	}
	return first;
}
