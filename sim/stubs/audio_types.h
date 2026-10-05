/**
 * @file audio_types.h
 * @brief 音频播放器公共数据类型桌面桩头文件, 字段定义与固件保持一致
 */

#ifndef H7_SIM_STUB_AUDIO_TYPES_H
#define H7_SIM_STUB_AUDIO_TYPES_H

#include <stdbool.h>
#include <stdint.h>

#include "audio_cfg.h"
#include "audio_res.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AUDIO_PATH_MAX          256u		// 文件路径最大长度(含结束符)
#define AUDIO_LYRIC_TEXT_MAX    256u		// 单行歌词文本最大长度(原文/译文各自)

/* 音频流信息与标签元数据 */
typedef struct {
	uint32_t sample_rate;	// 采样率 (Hz)
	uint8_t  channels;	// 声道数
	uint8_t  bits_per_sample;	// 位深 (bit), MP3 等有损格式记为 0
	uint64_t total_frames;	// PCM 总帧数(每帧含全部声道)
	uint32_t duration_ms;	// 总时长 (ms)
	char title[AUDIO_META_TAG_LEN];	// 歌名标签
	char artist[AUDIO_META_TAG_LEN];	// 艺术家标签
	char album[AUDIO_META_TAG_LEN];	// 专辑标签
	char album_artist[AUDIO_META_TAG_LEN];	// 专辑艺术家标签
	char genre[AUDIO_META_TAG_LEN];	// 流派标签
	char date[AUDIO_META_TAG_LEN];	// 发行日期标签
	char track[AUDIO_META_TAG_LEN];	// 音轨号标签
	char comment[AUDIO_META_TAG_LEN];	// 备注标签
} audio_meta_t;

/* 播放器状态机状态 */
typedef enum {
	AUDIO_PLAYER_IDLE = 0,	// 空闲(无播放任务)
	AUDIO_PLAYER_STARTING,	// 正在打开文件并启动播放
	AUDIO_PLAYER_PLAYING,	// 播放中
	AUDIO_PLAYER_PAUSED,	// 已暂停
	AUDIO_PLAYER_SEEKING,	// 正在跳转进度
	AUDIO_PLAYER_STOPPING,	// 正在停止
	AUDIO_PLAYER_ERROR,	// 播放出错(见 last_error)
} audio_player_state_t;

/* 歌单循环模式 */
typedef enum {
	AUDIO_REPEAT_QUEUE = 0,	// 列表顺序播放, 播完末曲停止
	AUDIO_REPEAT_ALL,	// 列表循环
	AUDIO_REPEAT_ONE,	// 单曲循环
	AUDIO_REPEAT_SHUFFLE,	// 随机播放
} audio_repeat_t;

/* 单条歌词(原文+翻译) */
typedef struct {
	uint32_t timestamp_ms;	// 该行起始时间 (ms)
	char original[AUDIO_LYRIC_TEXT_MAX];	// 原唱歌词
	char translation[AUDIO_LYRIC_TEXT_MAX];	// 翻译歌词, 无翻译时为空串
} audio_lyric_line_t;

/* 播放器对外状态快照, 成员不指向播放器内部存储 */
typedef struct {
	audio_player_state_t state;	// 当前状态
	audio_res_t          last_error;	// 最近一次错误码
	uint32_t position_ms;	// 当前播放位置 (ms)
	uint32_t duration_ms;	// 当前曲目总时长 (ms)
	uint32_t track_id;	// 曲目切换计数, 每次切歌自增
	uint8_t  volume;	// 当前音量 (0-100)
	audio_repeat_t repeat;	// 当前循环模式
	char source[AUDIO_PATH_MAX];	// 当前曲目路径
	audio_meta_t meta;	// 当前曲目元数据
	audio_lyric_line_t lyric;	// 当前歌词行
} audio_player_snapshot_t;

#ifdef __cplusplus
}
#endif

#endif /* H7_SIM_STUB_AUDIO_TYPES_H */
