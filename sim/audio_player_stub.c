/** Desktop preview implementation of the frontend-neutral player API. */
#ifndef STM32H723xx

#include "audio_player.h"
#include <stddef.h>
#include <string.h>

static audio_player_snapshot_t s_snapshot = {
	.state = AUDIO_PLAYER_PLAYING,
	.last_error = AUDIO_RES_OK,
	.position_ms = 42000,
	.duration_ms = 215000,
	.volume = 50,
	.repeat = AUDIO_REPEAT_ALL,
	.meta = {
		.title = "LVGL Audio Player",
		.artist = "H7 MediaPlayer",
	},
	.lyric = {
		.timestamp_ms = 40000,
		.original = "\xE6\xAD\xA3\xE5\x9C\xA8\xE6\x92\xAD\xE6\x94\xBE\xE7\x9A\x84\xE6\xAD\x8C\xE8\xAF\x8D",
		.translation = "Current lyric translation",
	},
};

const char *audio_res_str(audio_res_t result) {
	return result == AUDIO_RES_OK ? "ok" : "preview has no audio backend";
}

audio_res_t audio_player_init(void) { return AUDIO_RES_OK; }

audio_res_t audio_player_play(const char *path) {
	(void)path;
	s_snapshot.state = AUDIO_PLAYER_ERROR;
	s_snapshot.last_error = AUDIO_RES_UNSUPPORTED_FORMAT;
	return AUDIO_RES_UNSUPPORTED_FORMAT;
}

audio_res_t audio_player_stop(void) {
	s_snapshot.state = AUDIO_PLAYER_IDLE;
	s_snapshot.last_error = AUDIO_RES_OK;
	s_snapshot.position_ms = 0u;
	memset(&s_snapshot.lyric, 0, sizeof(s_snapshot.lyric));
	return AUDIO_RES_OK;
}

audio_res_t audio_player_pause(void) { return AUDIO_RES_NOT_ACTIVE; }
audio_res_t audio_player_resume(void) { return AUDIO_RES_NOT_ACTIVE; }
audio_res_t audio_player_seek(uint32_t position_ms) {
	(void)position_ms; return AUDIO_RES_NOT_ACTIVE;
}
audio_res_t audio_player_next(void) { return AUDIO_RES_NOT_ACTIVE; }
audio_res_t audio_player_previous(void) { return AUDIO_RES_NOT_ACTIVE; }

audio_res_t audio_player_set_volume(uint8_t volume) {
	s_snapshot.volume = volume > 100u ? 100u : volume;
	return AUDIO_RES_OK;
}

audio_res_t audio_player_set_repeat(audio_repeat_t repeat) {
	if (repeat > AUDIO_REPEAT_SHUFFLE) return AUDIO_RES_INVALID_ARG;
	s_snapshot.repeat = repeat;
	return AUDIO_RES_OK;
}

audio_res_t audio_player_read_metadata(const char *path, audio_meta_t *meta) {
	if (path == NULL || meta == NULL) return AUDIO_RES_INVALID_ARG;
	memset(meta, 0, sizeof(*meta));
	return AUDIO_RES_UNSUPPORTED_FORMAT;
}

audio_res_t audio_player_snapshot(audio_player_snapshot_t *snapshot) {
	if (snapshot == NULL) return AUDIO_RES_INVALID_ARG;
	*snapshot = s_snapshot;
	return AUDIO_RES_OK;
}

bool audio_player_supports(const char *path) {
	static const char *extensions[] = {"wav", "mp3", "flac"};
	const char *dot = NULL, *cursor;
	uint32_t i;
	if (path == NULL) return false;
	for (cursor = path; *cursor != '\0'; cursor++) if (*cursor == '.') dot = cursor;
	if (dot == NULL) return false;
	dot++;
	for (i = 0; i < sizeof(extensions) / sizeof(extensions[0]); i++) {
		const char *left = dot, *right = extensions[i];
		while (*left != '\0' && *right != '\0') {
			char value = (*left >= 'A' && *left <= 'Z') ? (char)(*left + 32) : *left;
			if (value != *right) break;
			left++; right++;
		}
		if (*left == '\0' && *right == '\0') return true;
	}
	return false;
}

#endif
