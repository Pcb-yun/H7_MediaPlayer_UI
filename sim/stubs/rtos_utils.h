/**
 * @file rtos_utils.h
 * @brief RTOS 实用接口桌面桩头文件, 类型定义与固件保持一致
 * @note 模拟器实现见 sim/stubs/h7_stub.c, 填充演示数据
 */

#ifndef H7_SIM_STUB_RTOS_UTILS_H
#define H7_SIM_STUB_RTOS_UTILS_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SYSMON_TASK_MAX 20	// 系统监视页单次最多采集的任务数
#define SYSMON_NAME_MAX 16	// 任务名缓冲长度

/* 单个任务的监视快照 */
typedef struct {
	void * task_id;		// 任务句柄(用于 shell 打印)
	char name[SYSMON_NAME_MAX];	// 任务名(截断拷贝)
	uint8_t state;		// 任务状态(osThreadState_t 取值)
	uint8_t priority;	// 当前优先级
	uint16_t stack_high;	// 栈高水位(单位: 字)
	uint16_t cpu_x100;	// CPU 占用率 ×100
} sysmon_task_t;

/* 堆内存统计快照 */
typedef struct {
	uint32_t free_heap;	// 当前空闲堆总量(字节)
	uint32_t min_free_heap;	// 历史最小空闲堆总量(字节)
	uint32_t largest_block;	// 最大空闲块(字节)
	uint32_t smallest_block;	// 最小空闲块(字节)
	uint32_t free_blocks;	// 空闲块数量
	uint32_t alloc_count;	// 累计成功分配次数
	uint32_t free_count;	// 累计成功释放次数
} sysmon_mem_t;

/* 时钟与内核节拍快照 */
typedef struct {
	uint32_t tick_freq;	// 内核节拍频率(Hz)
	uint32_t tick_count;	// 内核节拍计数
	uint32_t sysclk_hz;	// 系统时钟(Hz)
	uint32_t hclk_hz;	// AHB 时钟(Hz)
	uint32_t pclk1_hz;	// APB1 时钟(Hz)
	uint32_t pclk2_hz;	// APB2 时钟(Hz)
} sysmon_clock_t;

void        sysmon_memory(sysmon_mem_t * out);
void        sysmon_clock(sysmon_clock_t * out);
uint8_t     sysmon_tasks(sysmon_task_t * out, uint8_t max);
bool        sysmon_temperature(int32_t * out);
uint32_t    sysmon_event_flags(void);
const char * sysmon_mutex_state(void);

#ifdef __cplusplus
}
#endif

#endif /* H7_SIM_STUB_RTOS_UTILS_H */
