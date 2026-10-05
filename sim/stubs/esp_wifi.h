/**
 * @file esp_wifi.h
 * @brief ESP Wi-Fi 服务桌面桩头文件, 仅供 PC 模拟器编译使用
 * @note 模拟器无 ESP 硬件, 除 GetStatus 外的接口恒返回失败
 */

#ifndef H7_SIM_STUB_ESP_WIFI_H
#define H7_SIM_STUB_ESP_WIFI_H

#include <stdint.h>

#include "esp.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 字段取值与固件协议保持一致 */
#define ESP_NETCP_WIFI_SSID_MAX_LENGTH     32U
#define ESP_NETCP_WIFI_PASSWORD_MAX_LENGTH 64U
#define ESP_NETCP_WIFI_SCAN_MAX_RESULTS   32U

/* Wi-Fi 工作状态, 取值与固件协议保持一致 */
typedef enum {
	ESP_NETCP_WIFI_OFF = 0,
	ESP_NETCP_WIFI_ON,
	ESP_NETCP_WIFI_CONNECTING,
	ESP_NETCP_WIFI_CONNECTED,
} esp_netcp_wifi_state_t;

/* Wi-Fi 认证类型, 仅列出 UI 使用的开放网络取值 */
typedef enum {
	ESP_NETCP_WIFI_AUTH_OPEN = 0,
	ESP_NETCP_WIFI_AUTH_UNKNOWN = 255,
} esp_netcp_wifi_auth_t;

/* 服务结果码, UI 只判断成功与否, 失败值合并为 ERROR */
typedef enum {
	ESP_NETCP_RESULT_OK             = 0,
	ESP_NETCP_RESULT_ERROR          = -1,
	ESP_NETCP_RESULT_INVALID_ARG    = -2,
	ESP_NETCP_RESULT_NOT_INITIALIZED = -3,
} esp_netcp_result_t;

typedef struct {
	int32_t status;          // 远端状态码
	uint8_t password_required;	// 需要密码时为 1
	uint8_t state;           // 当前连接状态
	uint16_t reserved;       // 保留
} esp_netcp_wifi_connect_response_t;

typedef struct {
	esp_netcp_wifi_state_t state;
	int8_t                 rssi;
	char                   ssid[ESP_NETCP_WIFI_SSID_MAX_LENGTH + 1U];
} esp_netcp_wifi_status_response_t;

typedef struct {
	char     ssid[ESP_NETCP_WIFI_SSID_MAX_LENGTH + 1U];
	int8_t   rssi;
	uint8_t  channel;
	uint8_t  auth_mode;
} esp_netcp_wifi_scan_entry_t;

typedef struct {
	int32_t status;	// 远端状态码
	uint16_t count;	// 本次返回条目数
	uint16_t total;	// 实际扫描到的条目总数
	esp_netcp_wifi_scan_entry_t entries[ESP_NETCP_WIFI_SCAN_MAX_RESULTS];
} esp_netcp_wifi_scan_response_t;

esp_netcp_result_t ESP_WiFi_GetStatus(esp_coprocessor_t * device,
									  esp_netcp_wifi_status_response_t * status);

esp_netcp_result_t ESP_WiFi_Scan(esp_coprocessor_t * device,
								 esp_netcp_wifi_scan_response_t * response);

esp_netcp_result_t ESP_WiFi_Connect(esp_coprocessor_t * device,
									const char * ssid,
									esp_netcp_wifi_connect_response_t * response);

esp_netcp_result_t ESP_WiFi_ConnectPassword(esp_coprocessor_t * device,
											const char * password,
											esp_netcp_wifi_connect_response_t * response);

esp_netcp_result_t ESP_WiFi_Disconnect(esp_coprocessor_t * device,
									   esp_netcp_wifi_status_response_t * status);

esp_netcp_result_t ESP_WiFi_Forget(esp_coprocessor_t * device,
								   esp_netcp_wifi_status_response_t * status);

#ifdef __cplusplus
}
#endif

#endif /* H7_SIM_STUB_ESP_WIFI_H */
