#pragma once
/**
 * config.h  -  M5Stack Game Boy Emulator ユーザー設定
 *
 * Arduino IDE でビルドする場合:
 *   使用するハードウェアに合わせて下の設定を編集してください。
 *
 * PlatformIO でビルドする場合:
 *   platformio.ini の build_flags で設定します。
 *   このファイルは自動的に無視されます (-DCONFIG_BOARD_SELECTED)。
 */

#ifndef CONFIG_BOARD_SELECTED  /* PlatformIO ビルドでは定義済みのためスキップ */

/* ── ハードウェア選択 ──────────────────────────────────────────────────
 * Core2 を使う場合: #define M5CORE2 を有効のままにしてください
 * Core / Basic / Gray / Fire を使う場合: 下行をコメントアウトし
 *   その下の #define M5CORE1 を有効にしてください              */
#define M5CORE2
/* #define M5CORE1 */

/* ── オーディオ ────────────────────────────────────────────────────────
 * 1 = I2S オーディオ有効 (Core2 推奨)
 * 0 = 無効 (Core / 低メモリ環境)                               */
#define ENABLE_SOUND 1

/* ── LCD 精度 ──────────────────────────────────────────────────────────
 * 0 = 高速モード (推奨)
 * 1 = 高精度モード (描画速度低下)                              */
#define PEANUT_GB_HIGH_LCD_ACCURACY 0

#endif /* CONFIG_BOARD_SELECTED */
