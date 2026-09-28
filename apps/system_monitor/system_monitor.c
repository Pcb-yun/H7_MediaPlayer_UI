#include "system_monitor.h"
#include "../../ui/ui_focus.h"
#include "../../components/info_row_gen.h"
#include "rtos_utils.h"
#include "cmsis_os2.h"
#include <stdio.h>

#define SYSMON_REFRESH_PERIOD  500
#define SYSMON_TEXT_BUF_SIZE   512
#define SYSMON_ITEM_MAX        SYSMON_TASK_MAX
#define SYSMON_SCROLL_STEP     18

typedef enum {
	SYSMON_PAGE_BASIC = 0,
	SYSMON_PAGE_MEMORY,
	SYSMON_PAGE_EVENT,
	SYSMON_PAGE_MUTEX,
	SYSMON_PAGE_TASK,
	SYSMON_PAGE_COUNT
} sysmon_page_t;

static const char * const page_titles[SYSMON_PAGE_COUNT] = {
	"基础信息", "堆内存", "事件标志", "互斥锁", "任务列表",
};

static const char * const event_flag_names[] = {
	"SYS_INIT_COMPLETE", "APP_NEED_USART", "SHELL_ONLINE", "FS_MOUNTED", "USART1_REFRESH",
};
#define SYSMON_FLAG_COUNT (sizeof(event_flag_names) / sizeof(event_flag_names[0]))

static const char * const mutex_names[] = { "Sem_Shellsend" };

static lv_obj_t * page_obj;
static lv_obj_t * page_title;
static lv_obj_t * page_body;
static lv_obj_t * page_text;
static lv_obj_t * item_box[SYSMON_ITEM_MAX];
static uint8_t item_count;
static lv_timer_t * refresh_timer;
static uint8_t cur_page;
static bool page_selected;
static sysmon_task_t * task_snapshot;
static char * page_buf;

static void sysmon_refresh(void);
static void sysmon_refresh_cb(lv_timer_t * t);
static void sysmon_delete_cb(lv_event_t * e);
static void sysmon_text_fill(char * buf, size_t len);
static uint8_t sysmon_items_fill(void);
static void sysmon_items_create(lv_obj_t * parent, ui_focus_action_cb_t back_cb);
static void sysmon_items_hide(void);
static void sysmon_item_set(uint8_t idx, const char * title, const char * value);
static bool sysmon_page_is_text(uint8_t page);
static void sysmon_page_show(void);
static void sysmon_focus_page(void);
static void sysmon_step(int32_t dir);
static void sysmon_select_indicator(void);
static void sysmon_key_cb(lv_event_t * e);
static void sysmon_click_cb(lv_obj_t * obj, void * user_data);
static void sysmon_item_click_cb(lv_obj_t * obj, void * user_data);
static void sysmon_item_focus_cb(lv_event_t * e);

static const char * sysmon_state_str(uint8_t state)
{
	switch((osThreadState_t)state) {
		case osThreadRunning: return "运行";
		case osThreadReady: return "就绪";
		case osThreadBlocked: return "阻塞";
		case osThreadInactive: return "挂起";
		case osThreadTerminated: return "结束";
		default: return "未知";
	}
}

static void label_set_if_changed(lv_obj_t * label, const char * text)
{
	if(label == NULL) return;
	if(lv_strcmp(lv_label_get_text(label), text) != 0) lv_label_set_text(label, text);
}

static void hz_to_mhz(char * buf, size_t len, uint32_t hz)
{
	snprintf(buf, len, "%lu.%lu", hz / 1000000u, (hz % 1000000u) / 100000u);
}

static bool sysmon_page_is_text(uint8_t page)
{
	return page == SYSMON_PAGE_BASIC || page == SYSMON_PAGE_MEMORY;
}

static void sysmon_text_fill(char * buf, size_t len)
{
	sysmon_clock_t clk;
	sysmon_mem_t mem;
	int32_t temperature;
	uint32_t hours;
	uint32_t minutes;
	uint32_t seconds;
	uint32_t millis;
	char mhz[4][10];

	if(buf == NULL || len == 0u) return;

	switch((sysmon_page_t)cur_page) {
		case SYSMON_PAGE_BASIC:
			sysmon_clock(&clk);
			if(clk.tick_freq > 0u) {
				hours = clk.tick_count / (clk.tick_freq * 3600u);
				minutes = (clk.tick_count % (clk.tick_freq * 3600u)) / (clk.tick_freq * 60u);
				seconds = (clk.tick_count % (clk.tick_freq * 60u)) / clk.tick_freq;
				millis = (clk.tick_count % clk.tick_freq) * 1000u / clk.tick_freq;
			}
			else {
				hours = 0u;
				minutes = 0u;
				seconds = 0u;
				millis = 0u;
			}
			hz_to_mhz(mhz[0], sizeof(mhz[0]), clk.sysclk_hz);
			hz_to_mhz(mhz[1], sizeof(mhz[1]), clk.hclk_hz);
			hz_to_mhz(mhz[2], sizeof(mhz[2]), clk.pclk1_hz);
			hz_to_mhz(mhz[3], sizeof(mhz[3]), clk.pclk2_hz);
			if(sysmon_temperature(&temperature)) {
				snprintf(buf, len,
					"运行时间: %02lu:%02lu:%02lu.%03lu\n"
					"系统节拍: %lu @ %lu Hz\n"
					"芯片温度: %ld ℃\n\n"
					"系统时钟\nSYSCLK  %s MHz\nHCLK    %s MHz\nPCLK1   %s MHz\nPCLK2   %s MHz",
					hours, minutes, seconds, millis, clk.tick_count, clk.tick_freq,
					temperature, mhz[0], mhz[1], mhz[2], mhz[3]);
			}
			else {
				snprintf(buf, len,
					"运行时间: %02lu:%02lu:%02lu.%03lu\n"
					"系统节拍: %lu @ %lu Hz\n"
					"芯片温度: 不可用\n\n"
					"系统时钟\nSYSCLK  %s MHz\nHCLK    %s MHz\nPCLK1   %s MHz\nPCLK2   %s MHz",
					hours, minutes, seconds, millis, clk.tick_count, clk.tick_freq,
					mhz[0], mhz[1], mhz[2], mhz[3]);
			}
			break;

		case SYSMON_PAGE_MEMORY:
			sysmon_memory(&mem);
			snprintf(buf, len,
				"当前可用  %lu B\n历史最低  %lu B\n最大连续块  %lu B\n"
				"空闲块数量  %lu\n分配次数  %lu\n释放次数  %lu",
				mem.free_heap, mem.min_free_heap, mem.largest_block,
				mem.free_blocks, mem.alloc_count, mem.free_count);
			break;

		default:
			buf[0] = '\0';
			break;
	}
}

static void sysmon_item_set(uint8_t idx, const char * title, const char * value)
{
	lv_obj_t * box;

	if(idx >= SYSMON_ITEM_MAX) return;
	box = item_box[idx];
	if(box == NULL) return;
	label_set_if_changed(lv_obj_get_child(box, 0), title);
	label_set_if_changed(lv_obj_get_child(box, 1), value);
	lv_obj_set_hidden(box, false);
}

static void sysmon_item_focus_cb(lv_event_t * e)
{
	lv_obj_t * box = lv_event_get_current_target_obj(e);
	if(box != NULL) lv_obj_scroll_to_view(box, LV_ANIM_ON);
}

static void sysmon_items_create(lv_obj_t * parent, ui_focus_action_cb_t back_cb)
{
	lv_group_t * group = lv_group_get_default();
	uint32_t i;

	if(parent == NULL) return;
	for(i = 0u; i < SYSMON_ITEM_MAX; i++) {
		lv_obj_t * box = info_row_create(parent, "", "", NULL);
		if(box == NULL) break;
		item_box[i] = box;
		lv_obj_set_hidden(box, true);
		ui_focus_bind_row(box, sysmon_item_click_cb, back_cb, NULL);
		lv_obj_add_event_cb(box, sysmon_item_focus_cb, LV_EVENT_FOCUSED, NULL);
		if(group != NULL) lv_group_add_obj(group, box);
	}
}

static void sysmon_items_hide(void)
{
	uint32_t i;
	for(i = 0u; i < SYSMON_ITEM_MAX; i++) {
		if(item_box[i] != NULL) lv_obj_set_hidden(item_box[i], true);
	}
	item_count = 0u;
}

static uint8_t sysmon_event_items_fill(void)
{
	uint32_t flags = sysmon_event_flags();
	char value[12];
	uint32_t i;

	snprintf(value, sizeof(value), "0x%08lx", flags);
	sysmon_item_set(0u, "System_Status", value);
	for(i = 0u; i < SYSMON_FLAG_COUNT; i++) {
		sysmon_item_set((uint8_t)(i + 1u), event_flag_names[i],
			(flags & (1u << i)) ? "SET" : "RESET");
	}
	return (uint8_t)(SYSMON_FLAG_COUNT + 1u);
}

static uint8_t sysmon_mutex_items_fill(void)
{
	sysmon_item_set(0u, mutex_names[0], sysmon_mutex_state());
	return 1u;
}

static uint8_t sysmon_task_items_fill(void)
{
	uint8_t count;
	uint8_t i;

	if(task_snapshot == NULL) return 0u;
	count = sysmon_tasks(task_snapshot, SYSMON_TASK_MAX);
	if(count > SYSMON_ITEM_MAX) count = SYSMON_ITEM_MAX;
	for(i = 0u; i < count; i++) {
		char detail[80];
		snprintf(detail, sizeof(detail),
			"状态：%s  优先级：%u\n剩余栈：%u  CPU：%u.%02u%%",
			sysmon_state_str(task_snapshot[i].state),
			(unsigned)task_snapshot[i].priority,
			(unsigned)task_snapshot[i].stack_high,
			(unsigned)(task_snapshot[i].cpu_x100 / 100u),
			(unsigned)(task_snapshot[i].cpu_x100 % 100u));
		sysmon_item_set(i, task_snapshot[i].name, detail);
	}
	return count;
}

static uint8_t sysmon_items_fill(void)
{
	uint8_t count = 0u;
	uint8_t i;

	switch((sysmon_page_t)cur_page) {
		case SYSMON_PAGE_EVENT: count = sysmon_event_items_fill(); break;
		case SYSMON_PAGE_MUTEX: count = sysmon_mutex_items_fill(); break;
		case SYSMON_PAGE_TASK: count = sysmon_task_items_fill(); break;
		default: break;
	}
	for(i = count; i < SYSMON_ITEM_MAX; i++) {
		if(item_box[i] != NULL) lv_obj_set_hidden(item_box[i], true);
	}
	item_count = count;
	return count;
}

static void sysmon_refresh(void)
{
	if(!sysmon_page_is_text(cur_page)) {
		sysmon_items_fill();
		return;
	}
	if(page_buf == NULL) {
		label_set_if_changed(page_text, "监视器内存不足");
		return;
	}
	sysmon_text_fill(page_buf, SYSMON_TEXT_BUF_SIZE);
	label_set_if_changed(page_text, page_buf);
}

/* 外层聚焦大项；点按进入后，文本页滚动，项目页才聚焦内部信息框。 */
static void sysmon_focus_page(void)
{
	lv_group_t * group = lv_group_get_default();
	bool text_page = sysmon_page_is_text(cur_page);

	if(group == NULL || page_obj == NULL) return;
	if(!page_selected || text_page || item_count == 0u) {
		if(lv_obj_get_group(page_obj) == NULL) lv_group_add_obj(group, page_obj);
		lv_group_focus_obj(page_obj);
		lv_group_set_editing(group, true);
		return;
	}

	if(lv_obj_get_group(page_obj) != NULL) lv_group_remove_obj(page_obj);
	if(item_box[0] != NULL) lv_group_focus_obj(item_box[0]);
	lv_group_set_editing(group, false);
}

static void sysmon_select_indicator(void)
{
	if(page_body == NULL) return;
	lv_obj_set_style_bg_opa(page_body, LV_OPA_COVER, 0);
	lv_obj_set_style_border_width(page_body, page_selected ? 2 : 1, 0);
	lv_obj_set_style_border_color(page_body, lv_color_hex(0xff72b6), 0);
}

static void sysmon_page_show(void)
{
	if(page_title != NULL) lv_label_set_text(page_title, page_titles[cur_page]);
	page_selected = false;

	if(sysmon_page_is_text(cur_page)) {
		sysmon_items_hide();
		if(page_text != NULL) lv_obj_set_hidden(page_text, false);
	}
	else if(page_text != NULL) {
		lv_obj_set_hidden(page_text, true);
	}

	if(page_body != NULL) lv_obj_scroll_to_y(page_body, 0, LV_ANIM_OFF);
	sysmon_refresh();
	sysmon_focus_page();
	sysmon_select_indicator();
}

static void sysmon_step(int32_t dir)
{
	int32_t next;

	if(page_selected) {
		if(sysmon_page_is_text(cur_page) && page_body != NULL) {
			lv_obj_scroll_to_y(page_body,
				lv_obj_get_scroll_y(page_body) + dir * SYSMON_SCROLL_STEP, LV_ANIM_ON);
		}
		return;
	}

	next = (int32_t)cur_page + dir;
	if(next < 0 || next >= (int32_t)SYSMON_PAGE_COUNT) return;
	cur_page = (uint8_t)next;
	sysmon_page_show();
}

static void sysmon_key_cb(lv_event_t * e)
{
	uint32_t key = *(uint32_t *)lv_event_get_param(e);
	if(key == LV_KEY_LEFT) sysmon_step(-1);
	else if(key == LV_KEY_RIGHT) sysmon_step(1);
}

static void sysmon_click_cb(lv_obj_t * obj, void * user_data)
{
	LV_UNUSED(obj);
	LV_UNUSED(user_data);
	page_selected = !page_selected;
	sysmon_focus_page();
	sysmon_select_indicator();
}

static void sysmon_item_click_cb(lv_obj_t * obj, void * user_data)
{
	LV_UNUSED(obj);
	LV_UNUSED(user_data);
	page_selected = false;
	sysmon_focus_page();
	sysmon_select_indicator();
}

static void sysmon_refresh_cb(lv_timer_t * t)
{
	LV_UNUSED(t);
	sysmon_refresh();
}

static void sysmon_delete_cb(lv_event_t * e)
{
	lv_group_t * group = lv_group_get_default();
	uint32_t i;

	LV_UNUSED(e);
	if(refresh_timer != NULL) {
		lv_timer_delete(refresh_timer);
		refresh_timer = NULL;
	}
	if(group != NULL) lv_group_set_wrap(group, true);
	for(i = 0u; i < SYSMON_ITEM_MAX; i++) item_box[i] = NULL;
	page_obj = NULL;
	page_title = NULL;
	page_body = NULL;
	page_text = NULL;
	cur_page = SYSMON_PAGE_BASIC;
	page_selected = false;
	item_count = 0u;
	if(task_snapshot != NULL) {
		lv_free(task_snapshot);
		task_snapshot = NULL;
	}
	if(page_buf != NULL) {
		lv_free(page_buf);
		page_buf = NULL;
	}
}

void system_monitor_bind(lv_obj_t * screen, ui_focus_action_cb_t back_cb)
{
	lv_group_t * group;
	uint32_t i;

	if(screen == NULL) return;
	page_obj = lv_obj_find_by_name(screen, "sysmon_page");
	page_title = lv_obj_find_by_name(screen, "sysmon_page_title");
	page_body = lv_obj_find_by_name(screen, "sysmon_page_body");
	page_text = lv_obj_find_by_name(screen, "sysmon_page_text");

	cur_page = SYSMON_PAGE_BASIC;
	page_selected = false;
	item_count = 0u;
	for(i = 0u; i < SYSMON_ITEM_MAX; i++) item_box[i] = NULL;
	task_snapshot = lv_malloc(sizeof(sysmon_task_t) * SYSMON_TASK_MAX);
	page_buf = lv_malloc(SYSMON_TEXT_BUF_SIZE);

	group = lv_group_get_default();
	sysmon_items_create(page_body, back_cb);

	if(page_obj != NULL) {
		lv_obj_set_scrollable(page_obj, false);
		ui_focus_bind_row(page_obj, sysmon_click_cb, back_cb, NULL);
		lv_obj_add_event_cb(page_obj, sysmon_key_cb, LV_EVENT_KEY, NULL);
	}
	/* 内层小窗口到首尾时停止；必须点按退出内层后才能继续切换大项。 */
	if(group != NULL) lv_group_set_wrap(group, false);

	sysmon_page_show();
	if(refresh_timer != NULL) {
		lv_timer_delete(refresh_timer);
		refresh_timer = NULL;
	}
	refresh_timer = lv_timer_create(sysmon_refresh_cb, SYSMON_REFRESH_PERIOD, NULL);
	lv_obj_add_event_cb(screen, sysmon_delete_cb, LV_EVENT_DELETE, NULL);
}
