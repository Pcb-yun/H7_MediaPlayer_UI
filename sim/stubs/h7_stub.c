/**
 * @file h7_stub.c
 * @brief 固件依赖桌面桩实现, 仅供 PC 模拟器与编辑器预览链接使用
 * @note 固件侧由真实驱动/中间件提供实现, 通过 STM32H723xx 宏隔离
 */

#ifndef STM32H723xx

#include "boot.h"
#include "cmsis_os2.h"
#include "esp.h"
#include "esp_wifi.h"
#include "rtc.h"
#include "rtos_utils.h"
#include "stm32h7xx_hal.h"
#include "tim.h"

#include <string.h>
#include <time.h>


TIM_HandleTypeDef htim23;	// 背光 PWM 定时器桩句柄
RTC_HandleTypeDef hrtc;		// RTC 桩句柄


osMutexId_t osMutexNew(const osMutexAttr_t * attr) {
	(void)attr;
	return (osMutexId_t)1;	// 返回非空句柄即可, UI 层只做空判断
}

osStatus_t osMutexAcquire(osMutexId_t mutex_id, uint32_t timeout) {
	(void)mutex_id;
	(void)timeout;
	return osOK;	// 模拟器单线程运行 LVGL, 互斥锁直接成功
}

osStatus_t osMutexRelease(osMutexId_t mutex_id) {
	(void)mutex_id;
	return osOK;
}

osThreadId_t osThreadNew(void (* func)(void *), void * argument,
						 const osThreadAttr_t * attr) {
	(void)func;
	(void)argument;
	(void)attr;
	/* 不真正创建线程, 避免后台线程与 LVGL 主循环竞争;
	 * Wi-Fi 等后台状态保持默认离线值 */
	return (osThreadId_t)1;
}

void osThreadExit(void) {
	/* 桩线程从不真正创建, 不会被调用 */
}

osStatus_t osDelay(uint32_t ticks) {
	(void)ticks;
	return osOK;
}

osEventFlagsId_t osEventFlagsNew(const osEventFlagsAttr_t * attr) {
	(void)attr;
	return (osEventFlagsId_t)1;
}

uint32_t osEventFlagsSet(osEventFlagsId_t ef_id, uint32_t flags) {
	(void)ef_id;
	return flags;
}

uint32_t osEventFlagsClear(osEventFlagsId_t ef_id, uint32_t flags) {
	(void)ef_id;
	return 0U;
}

uint32_t osEventFlagsWait(osEventFlagsId_t ef_id, uint32_t flags,
						  uint32_t options, uint32_t timeout) {
	(void)ef_id;
	(void)options;
	(void)timeout;
	return flags;	// 直接视为已等到, 避免模拟器单线程模型下死等
}

osMessageQueueId_t osMessageQueueNew(uint32_t msg_count, uint32_t msg_size,
									 const osMessageQueueAttr_t * attr) {
	(void)msg_count;
	(void)msg_size;
	(void)attr;
	return (osMessageQueueId_t)1;
}

osStatus_t osMessageQueuePut(osMessageQueueId_t mq_id, const void * msg_ptr,
							 uint8_t msg_prio, uint32_t timeout) {
	(void)mq_id;
	(void)msg_ptr;
	(void)msg_prio;
	(void)timeout;
	return osOK;
}

osStatus_t osMessageQueueGet(osMessageQueueId_t mq_id, void * msg_ptr,
							 uint8_t * msg_prio, uint32_t timeout) {
	(void)mq_id;
	(void)msg_ptr;
	(void)msg_prio;
	(void)timeout;
	return osOK;
}

osStatus_t osMessageQueueReset(osMessageQueueId_t mq_id) {
	(void)mq_id;
	return osOK;
}

void HAL_NVIC_SystemReset(void) {
	/* 模拟器不执行复位 */
}

HAL_StatusTypeDef HAL_RTC_GetTime(RTC_HandleTypeDef * hrtc_handle,
								  RTC_TimeTypeDef * rtc_time, uint32_t format) {
	time_t now;
	struct tm * local;

	(void)hrtc_handle;
	(void)format;
	if(rtc_time == NULL) return HAL_ERROR;

	/* 使用 PC 本地时间, 让主页时钟与真实环境一致 */
	now   = time(NULL);
	local = localtime(&now);
	if(local == NULL) return HAL_ERROR;

	rtc_time->Hours   = (uint8_t)local->tm_hour;
	rtc_time->Minutes = (uint8_t)local->tm_min;
	rtc_time->Seconds = (uint8_t)local->tm_sec;
	return HAL_OK;
}

HAL_StatusTypeDef HAL_RTC_GetDate(RTC_HandleTypeDef * hrtc_handle,
								  RTC_DateTypeDef * date, uint32_t format) {
	time_t now;
	struct tm * local;

	(void)hrtc_handle;
	(void)format;
	if(date == NULL) return HAL_ERROR;

	now   = time(NULL);
	local = localtime(&now);
	if(local == NULL) return HAL_ERROR;

	/* tm_wday 为 0(周日)~6(周六), STM32 RTC 为 1(周一)~7(周日) */
	date->WeekDay = (uint8_t)(local->tm_wday == 0 ? 7 : local->tm_wday);
	date->Month   = (uint8_t)(local->tm_mon + 1);
	date->Date    = (uint8_t)local->tm_mday;
	date->Year    = (uint8_t)(local->tm_year % 100);
	return HAL_OK;
}

esp_coprocessor_t * ESP_NetCP_Device(void) {
	static esp_coprocessor_t device;
	return &device;
}

bool esp_coprocessor_is_online(esp_coprocessor_t * device) {
	(void)device;
	return false;	// 模拟器无 ESP 硬件, 主页 Wi-Fi 显示离线
}

esp_netcp_result_t ESP_WiFi_GetStatus(esp_coprocessor_t * device,
									  esp_netcp_wifi_status_response_t * status) {
	(void)device;
	(void)status;
	return ESP_NETCP_RESULT_ERROR;
}

/* 模拟器无 ESP 硬件, 以下接口仅满足链接, 恒返回失败 */
esp_netcp_result_t ESP_WiFi_Scan(esp_coprocessor_t * device,
								  esp_netcp_wifi_scan_response_t * response) {
	(void)device;
	if(response != NULL) response->count = 0U;
	return ESP_NETCP_RESULT_ERROR;
}

esp_netcp_result_t ESP_WiFi_Connect(esp_coprocessor_t * device, const char * ssid,
									esp_netcp_wifi_connect_response_t * response) {
	(void)device;
	(void)ssid;
	if(response != NULL) response->password_required = 0U;
	return ESP_NETCP_RESULT_ERROR;
}

esp_netcp_result_t ESP_WiFi_ConnectPassword(esp_coprocessor_t * device,
											const char * password,
											esp_netcp_wifi_connect_response_t * response) {
	(void)device;
	(void)password;
	if(response != NULL) response->password_required = 0U;
	return ESP_NETCP_RESULT_ERROR;
}

esp_netcp_result_t ESP_WiFi_Disconnect(esp_coprocessor_t * device,
									   esp_netcp_wifi_status_response_t * status) {
	(void)device;
	(void)status;
	return ESP_NETCP_RESULT_ERROR;
}

esp_netcp_result_t ESP_WiFi_Forget(esp_coprocessor_t * device,
								   esp_netcp_wifi_status_response_t * status) {
	(void)device;
	(void)status;
	return ESP_NETCP_RESULT_ERROR;
}

void BootShared_Update(const uint8_t * path) {
	(void)path;	// 模拟器无 Bootloader, 固件更新直接忽略
}

void sysmon_memory(sysmon_mem_t * out) {
	if(out == NULL) return;

	/* 填充一组接近实机的演示数据 */
	out->free_heap      = 180U * 1024U;
	out->min_free_heap  = 160U * 1024U;
	out->largest_block  = 120U * 1024U;
	out->smallest_block = 64U;
	out->free_blocks    = 9U;
	out->alloc_count    = 1234U;
	out->free_count     = 1200U;
}

void sysmon_clock(sysmon_clock_t * out) {
	if(out == NULL) return;

	out->tick_freq  = 1000U;
	out->tick_count = (uint32_t)clock();	// 毫秒级运行节拍
	out->sysclk_hz  = 550000000UL;
	out->hclk_hz    = 275000000UL;
	out->pclk1_hz   = 137500000UL;
	out->pclk2_hz   = 137500000UL;
}

uint8_t sysmon_tasks(sysmon_task_t * out, uint8_t max) {
	static const struct {
		const char * name;
		uint8_t      state;
		uint8_t      priority;
		uint16_t     stack_high;
		uint16_t     cpu_x100;
	} demo_tasks[] = {
		{ "lvgl",  osThreadRunning, 24U, 512U, 1830U },
		{ "audio", osThreadBlocked, 23U, 640U,  940U },
		{ "usb",   osThreadBlocked, 22U, 384U,  120U },
	};
	uint8_t count = (uint8_t)(sizeof(demo_tasks) / sizeof(demo_tasks[0]));
	uint8_t i;

	if(out == NULL) return 0U;
	if(count > max) count = max;

	for(i = 0U; i < count; i++) {
		out[i].task_id    = NULL;
		strncpy(out[i].name, demo_tasks[i].name, SYSMON_NAME_MAX - 1U);
		out[i].name[SYSMON_NAME_MAX - 1U] = '\0';
		out[i].state      = demo_tasks[i].state;
		out[i].priority   = demo_tasks[i].priority;
		out[i].stack_high = demo_tasks[i].stack_high;
		out[i].cpu_x100   = demo_tasks[i].cpu_x100;
	}
	return count;
}

bool sysmon_temperature(int32_t * out) {
	(void)out;
	return false;	// 模拟器无温度传感器, UI 自动隐藏温度行
}

uint32_t sysmon_event_flags(void) {
	return 0U;
}

const char * sysmon_mutex_state(void) {
	return "空闲";
}

#endif /* STM32H723xx */
