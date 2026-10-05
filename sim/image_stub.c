/**
 * @file image_stub.c
 * @brief 图片解码接口桌面桩实现, 仅供 PC 模拟器与编辑器预览链接使用
 * @note 模拟器不做JPEG解码与BIN缩放, 固定返回失败, 封面逻辑回退默认图
 */

#ifndef STM32H723xx

#include "image.h"

image_result_t image_jpeg_cache_get(const char *jpeg_path,
									const image_convert_config_t *config,
									bool exact_size,
									char *bin_path,
									size_t bin_path_size,
									image_jpeg_info_t *info,
									bool *cache_hit) {
	(void)jpeg_path;
	(void)config;
	(void)exact_size;
	(void)bin_path;
	(void)bin_path_size;
	(void)info;
	(void)cache_hit;
	return IMAGE_RESULT_UNSUPPORTED;
}

image_result_t image_bin_scale_cache_get(const char *source_bin_path,
										 const image_convert_config_t *config,
										 char *bin_path,
										 size_t bin_path_size,
										 image_jpeg_info_t *info,
										 bool *cache_hit) {
	(void)source_bin_path;
	(void)config;
	(void)bin_path;
	(void)bin_path_size;
	(void)info;
	(void)cache_hit;
	return IMAGE_RESULT_UNSUPPORTED;
}

#endif
