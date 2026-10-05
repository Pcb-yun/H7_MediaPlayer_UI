/**
 * @file lvgl_fatfs.h
 * @brief LVGL FatFs 辅助接口桌面桩头文件, 声明与固件保持一致
 * @note 模拟器只实现删除操作, 其余文件系统能力由 LVGL stdio 后端提供
 */

#ifndef H7_SIM_STUB_LVGL_FATFS_H
#define H7_SIM_STUB_LVGL_FATFS_H

#include "lvgl.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LVGL_FATFS_LETTER     'C'

bool lvgl_fatfs_init(void);
bool lvgl_fatfs_remove(const char * path);

#ifdef __cplusplus
}
#endif

#endif /* H7_SIM_STUB_LVGL_FATFS_H */
