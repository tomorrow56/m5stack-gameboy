#pragma once
#include "config.h"
#include <stdint.h>

#define GB_W  160
#define GB_H  144
#define LCD_W 320
#define LCD_H 240

#ifdef M5CORE2
/** Core2: 下部 72px をタッチ仮想パッド用に予約 */
#  define GB_DISPLAY_H  168
#else
/** Core1: フルスクリーン使用 */
#  define GB_DISPLAY_H  LCD_H
#endif

/** カラーパレット種類 */
typedef enum {
  PALETTE_GREEN = 0,  /**< クラシック DMG グリーン */
  PALETTE_GRAY,       /**< グレースケール */
  PALETTE_WARM,       /**< ウォーム (セピア調) */
  PALETTE_COUNT
} PaletteType;

/**
 * @brief ディスプレイモジュールを初期化する
 *        フレームバッファを PSRAM に確保し LCD をクリアする
 */
void displayInit();

/**
 * @brief カラーパレットを切り替える
 * @param p パレット種類
 */
void displaySetPalette(PaletteType p);

/**
 * @brief peanut-gb の lcd_draw_line コールバックから呼び出す
 *        1 スキャンライン分のパレットインデックス (0-3) を記録する
 * @param pixels  160 要素のパレットインデックス配列
 * @param line    スキャンライン番号 (0-143)
 */
void displayDrawLine(const uint8_t *pixels, uint_fast8_t line);

/**
 * @brief gb_run_frame() 後に呼び出し、フレームバッファを LCD に転送する
 *        320x240 へのスケーリング (X:2x, Y:nearest-neighbor) を行う
 */
void displayPushFrame();

/**
 * @brief 画面中央にメッセージを表示する（起動時・エラー時用）
 * @param msg 表示文字列（'\n' で改行可）
 */
void displayShowMessage(const char *msg);

/** @brief 現在のパレットを取得する */
PaletteType displayGetPalette();
