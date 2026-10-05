/**
 * @file stm32h7xx_hal.h
 * @brief STM32H7 HAL 桌面桩头文件, 仅供 PC 模拟器编译使用
 * @note 固件侧使用真实 HAL 库, 此处只保留 UI 层引用的类型、宏与函数
 */

#ifndef H7_SIM_STUB_STM32H7XX_HAL_H
#define H7_SIM_STUB_STM32H7XX_HAL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TIM_CHANNEL_1  0x00000000U	// 定时器通道1
#define RTC_FORMAT_BIN 0x00000000U	// RTC 二进制格式

/* 定时器比较值写入宏, 模拟器无背光硬件, 直接丢弃 */
#define __HAL_TIM_SET_COMPARE(h, ch, v) do { \
	(void)(h); \
	(void)(ch); \
	(void)(v); \
} while(0)

typedef enum {
	HAL_OK    = 0,
	HAL_ERROR = 1,
} HAL_StatusTypeDef;

typedef struct {
	uint32_t dummy;
} TIM_HandleTypeDef;

typedef struct {
	uint32_t dummy;
} RTC_HandleTypeDef;

typedef struct {
	uint8_t Hours;
	uint8_t Minutes;
	uint8_t Seconds;
} RTC_TimeTypeDef;

typedef struct {
	uint8_t WeekDay;
	uint8_t Month;
	uint8_t Date;
	uint8_t Year;
} RTC_DateTypeDef;

void             HAL_NVIC_SystemReset(void);
HAL_StatusTypeDef HAL_RTC_GetTime(RTC_HandleTypeDef * hrtc, RTC_TimeTypeDef * time,
								  uint32_t format);
HAL_StatusTypeDef HAL_RTC_GetDate(RTC_HandleTypeDef * hrtc, RTC_DateTypeDef * date,
								  uint32_t format);

#ifdef __cplusplus
}
#endif

#endif /* H7_SIM_STUB_STM32H7XX_HAL_H */
