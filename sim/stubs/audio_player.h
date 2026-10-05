/**
 * @file audio_player.h
 * @brief 音频播放服务接口桌面桩头文件, 声明与固件保持一致
 */

#ifndef H7_SIM_STUB_AUDIO_PLAYER_H
#define H7_SIM_STUB_AUDIO_PLAYER_H

#include <stdbool.h>
#include <stddef.h>

#include "audio_types.h"

#ifdef __cplusplus
extern "C" {
#endif

audio_res_t audio_player_play(const char * path);
audio_res_t audio_player_pause(void);
audio_res_t audio_player_resume(void);
audio_res_t audio_player_stop(void);
audio_res_t audio_player_previous(void);
audio_res_t audio_player_next(void);
audio_res_t audio_player_seek(uint32_t position_ms);
audio_res_t audio_player_set_repeat(audio_repeat_t repeat);
audio_res_t audio_player_snapshot(audio_player_snapshot_t * snapshot);
audio_res_t audio_player_set_volume(uint8_t volume);
bool audio_player_cover_path(char *path, size_t size);
uint32_t audio_player_lyric_get(uint32_t position_ms, uint32_t before,
	uint32_t after, audio_lyric_line_t *lines, uint32_t capacity,
	uint32_t *current_index);

#ifdef __cplusplus
}
#endif

#endif /* H7_SIM_STUB_AUDIO_PLAYER_H */
