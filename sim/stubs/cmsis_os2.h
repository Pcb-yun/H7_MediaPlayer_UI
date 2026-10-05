/**
 * @file cmsis_os2.h
 * @brief CMSIS-RTOS2 桌面桩头文件, 仅供 PC 模拟器编译使用
 * @note 固件侧使用真实 CMSIS-RTOS2, 此处只保留 UI 层引用的最小 API
 */

#ifndef H7_SIM_STUB_CMSIS_OS2_H
#define H7_SIM_STUB_CMSIS_OS2_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define osWaitForever          0xFFFFFFFFU	// 无限等待超时值
#define osOK                   0			// 操作成功
#define osPriorityBelowNormal  8			// 低于普通优先级
#define osPriorityLow          4			// 低优先级
#define osFlagsWaitAny         0x00000000U	// 等待任一标志位置位

typedef int32_t  osStatus_t;
typedef void *   osMutexId_t;
typedef void *   osThreadId_t;
typedef void *   osEventFlagsId_t;
typedef void *   osMessageQueueId_t;

/* 任务状态, 取值与真实 CMSIS-RTOS2 保持一致 */
typedef enum {
	osThreadInactive   = 0,
	osThreadReady      = 1,
	osThreadRunning    = 2,
	osThreadBlocked    = 3,
	osThreadTerminated = 4,
} osThreadState_t;

typedef struct {
	const char * name;
	uint32_t     attr_bits;
	void *       cb_mem;
	uint32_t     cb_size;
} osMutexAttr_t;

typedef struct {
	const char * name;
	uint32_t     attr_bits;
	void *       cb_mem;
	uint32_t     cb_size;
	void *       stack_mem;
	uint32_t     stack_size;
	int32_t      priority;
	uint32_t     tz_module;
	uint32_t     reserved;
} osThreadAttr_t;

typedef struct {
	const char * name;
	uint32_t     attr_bits;
	void *       cb_mem;
	uint32_t     cb_size;
} osEventFlagsAttr_t;

typedef struct {
	const char * name;
	uint32_t     attr_bits;
	void *       cb_mem;
	uint32_t     cb_size;
	void *       mq_mem;
	uint32_t     mq_size;
} osMessageQueueAttr_t;

osMutexId_t        osMutexNew(const osMutexAttr_t * attr);
osStatus_t         osMutexAcquire(osMutexId_t mutex_id, uint32_t timeout);
osStatus_t         osMutexRelease(osMutexId_t mutex_id);
osThreadId_t       osThreadNew(void (* func)(void *), void * argument,
							   const osThreadAttr_t * attr);
void               osThreadExit(void);
osStatus_t         osDelay(uint32_t ticks);
osEventFlagsId_t   osEventFlagsNew(const osEventFlagsAttr_t * attr);
uint32_t           osEventFlagsSet(osEventFlagsId_t ef_id, uint32_t flags);
uint32_t           osEventFlagsClear(osEventFlagsId_t ef_id, uint32_t flags);
uint32_t           osEventFlagsWait(osEventFlagsId_t ef_id, uint32_t flags,
									uint32_t options, uint32_t timeout);
osMessageQueueId_t osMessageQueueNew(uint32_t msg_count, uint32_t msg_size,
									 const osMessageQueueAttr_t * attr);
osStatus_t         osMessageQueuePut(osMessageQueueId_t mq_id,
									 const void * msg_ptr, uint8_t msg_prio,
									 uint32_t timeout);
osStatus_t         osMessageQueueGet(osMessageQueueId_t mq_id,
									 void * msg_ptr, uint8_t * msg_prio,
									 uint32_t timeout);
osStatus_t         osMessageQueueReset(osMessageQueueId_t mq_id);

#ifdef __cplusplus
}
#endif

#endif /* H7_SIM_STUB_CMSIS_OS2_H */
