/**
 * @file boot.h
 * @brief Bootloader 共享接口桌面桩头文件, 仅供 PC 模拟器编译使用
 */

#ifndef H7_SIM_STUB_BOOT_H
#define H7_SIM_STUB_BOOT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void BootShared_Update(const uint8_t * path);

#ifdef __cplusplus
}
#endif

#endif /* H7_SIM_STUB_BOOT_H */
