#pragma once
#include "config.h"
#include <stdint.h>

/* GB ジョイパッドビット (active low: 0 = 押下)
 * peanut_gb.h の JOYPAD_* 定数と完全一致 */
#define GB_BTN_A      0x01u
#define GB_BTN_B      0x02u
#define GB_BTN_SELECT 0x04u
#define GB_BTN_START  0x08u
#define GB_BTN_RIGHT  0x10u
#define GB_BTN_LEFT   0x20u
#define GB_BTN_UP     0x40u
#define GB_BTN_DOWN   0x80u

/**
 * @brief 入力モジュールを初期化する
 *        Core2 の場合はタッチ仮想ボタンのヒット領域を設定する
 */
void inputInit();

/**
 * @brief フレームごとに呼び出し、GB ジョイパッドレジスタを更新する
 *        M5.update() は呼び出し元 (main.cpp) で行うこと
 * @param joypad  gb.direct.joypad へのポインタ。初期値 0xFF = 全ボタン解放
 */
void inputUpdate(uint8_t *joypad);

/**
 * @brief メニュー操作用: 何らかのボタンが押されるまで待機する
 * @return 押された GB_BTN_* ビットマスク
 */
uint8_t inputWaitAny();
