#pragma once
#include "config.h"
#include <stdint.h>

/**
 * @brief オーディオモジュールを初期化する
 *        ENABLE_SOUND=1 の場合のみ有効
 *        minigb_apu APU コンテキストを初期化し、ESP32 I2S を設定する
 */
void audioInit();

/**
 * @brief 1 フレーム分のオーディオサンプルを生成して I2S へ書き出す
 *        gb_run_frame() の後に呼び出す
 */
void audioUpdate();

/**
 * @brief オーディオを一時停止 / 再開する
 * @param mute true=消音, false=再生
 */
void audioSetMute(bool mute);

/* ─── peanut-gb から呼ばれる APU I/O 関数 (ENABLE_SOUND=1 時のみ) ───────
 * peanut_gb.h が直接呼び出すため C リンケージで公開する必要がある
 * 定義は audio.cpp 内にあり、ここは宣言のみ                         */
#if ENABLE_SOUND
#  ifdef __cplusplus
extern "C" {
#  endif
uint8_t audio_read(uint16_t addr);
void    audio_write(uint16_t addr, uint8_t val);
#  ifdef __cplusplus
}
#  endif
#endif
