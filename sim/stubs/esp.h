/**
 * @file esp.h
 * @brief ESP 协处理器桌面桩头文件, 仅供 PC 模拟器编译使用
 * @note 模拟器无 ESP 硬件, esp_coprocessor_is_online 恒返回 false
 */

#ifndef H7_SIM_STUB_ESP_H
#define H7_SIM_STUB_ESP_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	int unused;
} esp_coprocessor_t;

esp_coprocessor_t * ESP_NetCP_Device(void);
bool                esp_coprocessor_is_online(esp_coprocessor_t * device);

#ifdef __cplusplus
}
#endif

#endif /* H7_SIM_STUB_ESP_H */
