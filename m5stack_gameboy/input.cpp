#include "config.h"
#include "input.h"
#include "display.h"
#include <Arduino.h>
#include <stdint.h>

#ifdef M5CORE2
#  include <M5Core2.h>
#else
#  include <M5Stack.h>
#endif

/* ── タッチ仮想ボタン領域 (Core2 専用) ─────────────────────────────────
 *
 *  320x240 画面レイアウト（下部 72px を仮想パッド領域として使用）
 *
 *  y=168 ┌──────────────────────────────────────┐
 *        │  ↑      │ SEL │ STA │  B  │  A  │
 *        │ ←  → │     │     │     │     │
 *        │  ↓      │     │     │     │     │
 *  y=240 └──────────────────────────────────────┘
 *         x=0  x=80  x=160 x=208 x=256 x=288  x=320
 */
#ifdef M5CORE2

#define PAD_Y      GB_DISPLAY_H   /* 仮想パッド開始 Y (ゲーム画面の直下) */
#define PAD_H       72   /* 仮想パッド高さ */

/* 十字キー各方向の中心座標とヒット半径 */
#define DPAD_CX     40
#define DPAD_CY    (PAD_Y + 36)
#define DPAD_R      22

/* アクションボタン */
#define BTN_A_X    288
#define BTN_B_X    256
#define BTN_SEL_X  168
#define BTN_STA_X  216
#define BTN_ACT_Y  (PAD_Y + 36)
#define BTN_ACT_R   20

static inline bool touchHit(int tx, int ty, int cx, int cy, int r) {
  int dx = tx - cx, dy = ty - cy;
  return (dx * dx + dy * dy) <= r * r;
}

#endif /* M5CORE2 */

/* ── 初期化 ────────────────────────────────────────────────────────────── */
void inputInit() {
#ifdef M5CORE2
  /* タッチ仮想パッド背景を描画 */
  M5.Lcd.fillRect(0, PAD_Y, 320, PAD_H, 0x1082);

  /* 十字キー */
  M5.Lcd.drawCircle(DPAD_CX,      PAD_Y + 14, 10, TFT_DARKGREY);  /* ↑ */
  M5.Lcd.drawCircle(DPAD_CX,      PAD_Y + 58, 10, TFT_DARKGREY);  /* ↓ */
  M5.Lcd.drawCircle(DPAD_CX - 22, PAD_Y + 36, 10, TFT_DARKGREY);  /* ← */
  M5.Lcd.drawCircle(DPAD_CX + 22, PAD_Y + 36, 10, TFT_DARKGREY);  /* → */

  M5.Lcd.setTextColor(TFT_DARKGREY);
  M5.Lcd.setTextSize(1);
  M5.Lcd.setCursor(DPAD_CX - 3,      PAD_Y + 11); M5.Lcd.print("^");
  M5.Lcd.setCursor(DPAD_CX - 3,      PAD_Y + 55); M5.Lcd.print("v");
  M5.Lcd.setCursor(DPAD_CX - 24,    PAD_Y + 33); M5.Lcd.print("<");
  M5.Lcd.setCursor(DPAD_CX + 19,    PAD_Y + 33); M5.Lcd.print(">");

  /* SEL / STA */
  M5.Lcd.drawRoundRect(BTN_SEL_X - 15, PAD_Y + 26, 30, 18, 4, TFT_DARKGREY);
  M5.Lcd.drawRoundRect(BTN_STA_X - 15, PAD_Y + 26, 30, 18, 4, TFT_DARKGREY);
  M5.Lcd.setCursor(BTN_SEL_X - 12, PAD_Y + 30); M5.Lcd.print("SEL");
  M5.Lcd.setCursor(BTN_STA_X - 12, PAD_Y + 30); M5.Lcd.print("STA");

  /* A / B */
  M5.Lcd.drawCircle(BTN_A_X, BTN_ACT_Y, BTN_ACT_R, TFT_RED);
  M5.Lcd.drawCircle(BTN_B_X, BTN_ACT_Y, BTN_ACT_R, TFT_BLUE);
  M5.Lcd.setTextColor(TFT_RED);
  M5.Lcd.setCursor(BTN_A_X - 3, BTN_ACT_Y - 5); M5.Lcd.print("A");
  M5.Lcd.setTextColor(TFT_BLUE);
  M5.Lcd.setCursor(BTN_B_X - 3, BTN_ACT_Y - 5); M5.Lcd.print("B");
#endif
}

/* ── フレーム入力更新 ──────────────────────────────────────────────────── */
void inputUpdate(uint8_t *joypad) {
  /* 全ボタン解放状態から開始 (active low: 0 = 押下) */
  *joypad = 0xFF;

#ifdef M5CORE2
  /* ── タッチ入力 (Core2) ─────────────────────────────────────────── */
  TouchPoint_t tp = M5.Touch.getPressPoint();
  if (tp.x >= 0 && tp.y >= PAD_Y) {
    int tx = tp.x, ty = tp.y;

    /* 十字キー */
    if (touchHit(tx, ty, DPAD_CX,      PAD_Y + 14, DPAD_R)) *joypad &= ~GB_BTN_UP;
    if (touchHit(tx, ty, DPAD_CX,      PAD_Y + 58, DPAD_R)) *joypad &= ~GB_BTN_DOWN;
    if (touchHit(tx, ty, DPAD_CX - 22, PAD_Y + 36, DPAD_R)) *joypad &= ~GB_BTN_LEFT;
    if (touchHit(tx, ty, DPAD_CX + 22, PAD_Y + 36, DPAD_R)) *joypad &= ~GB_BTN_RIGHT;

    /* A / B */
    if (touchHit(tx, ty, BTN_A_X, BTN_ACT_Y, BTN_ACT_R)) *joypad &= ~GB_BTN_A;
    if (touchHit(tx, ty, BTN_B_X, BTN_ACT_Y, BTN_ACT_R)) *joypad &= ~GB_BTN_B;

    /* SEL / STA */
    if (tx >= BTN_SEL_X - 15 && tx < BTN_SEL_X + 15 &&
        ty >= PAD_Y + 26      && ty < PAD_Y + 44) {
      *joypad &= ~GB_BTN_SELECT;
    }
    if (tx >= BTN_STA_X - 15 && tx < BTN_STA_X + 15 &&
        ty >= PAD_Y + 26      && ty < PAD_Y + 44) {
      *joypad &= ~GB_BTN_START;
    }
  }

  /* 物理ボタン (長押し: メニュー系コントロールは main.cpp で処理) */
  if (M5.BtnA.isPressed()) *joypad &= ~GB_BTN_LEFT;
  if (M5.BtnC.isPressed()) *joypad &= ~GB_BTN_RIGHT;

#else
  /* ── 物理 3 ボタン (Core1) ─────────────────────────────────────── */
  /* A → GB_A / B → GB_B / C → GB_START  (単押し) */
  if (M5.BtnA.isPressed()) *joypad &= ~GB_BTN_A;
  if (M5.BtnB.isPressed()) *joypad &= ~GB_BTN_B;
  if (M5.BtnC.isPressed()) *joypad &= ~GB_BTN_START;
  /* A+B 同時押し → SELECT */
  if (M5.BtnA.isPressed() && M5.BtnB.isPressed()) {
    *joypad &= ~GB_BTN_SELECT;
    *joypad |=  GB_BTN_A;
    *joypad |=  GB_BTN_B;
  }
#endif
}

/* ── 任意ボタン待機 ────────────────────────────────────────────────────── */
uint8_t inputWaitAny() {
  while (true) {
    M5.update();
    if (M5.BtnA.wasPressed()) return GB_BTN_A;
    if (M5.BtnB.wasPressed()) return GB_BTN_B;
#ifdef M5CORE2
    TouchPoint_t tp = M5.Touch.getPressPoint();
    if (tp.x >= 0) return GB_BTN_START;
#else
    if (M5.BtnC.wasPressed()) return GB_BTN_START;
#endif
    delay(10);
  }
}
