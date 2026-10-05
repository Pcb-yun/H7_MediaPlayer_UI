/**
 * @file image.h
 * @brief 项目图片解码公共接口桌面桩头文件, 声明与固件保持一致
 * @note 模拟器不真正解码JPEG, 接口实现固定返回失败, 封面逻辑自动回退默认图
 */

#ifndef H7_SIM_STUB_IMAGE_H
#define H7_SIM_STUB_IMAGE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 图片解码、缓存与显示公共结果码, 取值与固件保持一致 */
typedef enum {
	IMAGE_RESULT_OK = 0,	// 操作成功
	IMAGE_RESULT_INVALID_ARG,	// 参数无效
	IMAGE_RESULT_OPEN_FAILED,	// 源文件打开或查询失败
	IMAGE_RESULT_UNSUPPORTED,	// 图片格式或尺寸不支持
	IMAGE_RESULT_NO_MEMORY,	// 动态内存不足
	IMAGE_RESULT_DECODE_FAILED,	// 图片解码或BIN校验失败
	IMAGE_RESULT_OUTPUT_FAILED,	// 像素输出或缓存提交失败
	IMAGE_RESULT_CANCELLED	// 解码被外部取消
} image_result_t;

/* JPEG转BIN高层配置 */
typedef struct {
	uint16_t max_width;	// 输出宽度上限
	uint16_t max_height;	// 输出高度上限
	const volatile bool *cancelled;	// 可选的外部取消标志
} image_convert_config_t;

/* JPEG解码后的实际输出信息 */
typedef struct {
	uint16_t width;	// 实际输出宽度
	uint16_t height;	// 实际输出高度
	uint8_t scale;	// JPEGDEC快速缩放倍数, 缓存命中时为0
} image_jpeg_info_t;

image_result_t image_jpeg_cache_get(const char *jpeg_path,
									const image_convert_config_t *config,
									bool exact_size,
									char *bin_path,
									size_t bin_path_size,
									image_jpeg_info_t *info,
									bool *cache_hit);

image_result_t image_bin_scale_cache_get(const char *source_bin_path,
										 const image_convert_config_t *config,
										 char *bin_path,
										 size_t bin_path_size,
										 image_jpeg_info_t *info,
										 bool *cache_hit);

#ifdef __cplusplus
}
#endif

#endif /* H7_SIM_STUB_IMAGE_H */
