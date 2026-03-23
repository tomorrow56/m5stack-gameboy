#include "config.h"
#include "display.h"
#include <Arduino.h>

#ifdef M5CORE2
#  include <M5Core2.h>
#else
#  include <M5Stack.h>
#endif

/* ── パレット定義 (RGB565) ────────────────────────────────────────────── */
static const uint16_t PALETTES[PALETTE_COUNT][4] = {
  /* GREEN: クラシック DMG グリーン */
  { 0x9FF3, 0x57A0, 0x2145, 0x0841 },
  /* GRAY: グレースケール */
  { 0xFFFF, 0xAD55, 0x52AA, 0x0000 },
  /* WARM: セピア調 */
  { 0xFF75, 0xCE49, 0x8B26, 0x2120 }
};

/* ── 内部状態 ─────────────────────────────────────────────────────────── */
static uint8_t  pixelBuf[GB_H][GB_W];        /* パレットインデックス (0-3) */
static uint16_t *frameBuf = nullptr;          /* RGB565 フレームバッファ (PSRAM) */
static PaletteType currentPalette = PALETTE_GREEN;

/* スケーリング用ルックアップテーブル: LCD 行 → GB 行 (0-143) */
static uint8_t rowLUT[GB_DISPLAY_H];

/* ── 初期化 ────────────────────────────────────────────────────────────── */
void displayInit() {
  /* フレームバッファ確保 */
#ifdef BOARD_HAS_PSRAM
  frameBuf = (uint16_t *)ps_malloc((size_t)LCD_W * GB_DISPLAY_H * 2);
#else
  frameBuf = (uint16_t *)malloc((size_t)LCD_W * GB_DISPLAY_H * 2);
#endif
  if (!frameBuf) {
    Serial.println("[display] frameBuf alloc failed");
  }

  /* 行ルックアップテーブル構築 (最近傍補間) */
  for (int r = 0; r < GB_DISPLAY_H; r++) {
    rowLUT[r] = (uint8_t)((r * GB_H) / GB_DISPLAY_H);
  }

  M5.Lcd.fillScreen(TFT_BLACK);
}

/* ── パレット切替 ──────────────────────────────────────────────────────── */
void displaySetPalette(PaletteType p) {
  if (p < PALETTE_COUNT) currentPalette = p;
}

PaletteType displayGetPalette() {
  return currentPalette;
}

/* ── スキャンライン記録 ────────────────────────────────────────────────── */
void displayDrawLine(const uint8_t *pixels, uint_fast8_t line) {
  if (line >= GB_H) return;
  memcpy(pixelBuf[line], pixels, GB_W);
}

/* ── フレーム転送 ──────────────────────────────────────────────────────── */
void displayPushFrame() {
  const uint16_t *pal = PALETTES[currentPalette];

  if (frameBuf) {
    /* 高速パス: PSRAM フレームバッファに全ピクセルを書いて一括転送 */
    for (int r = 0; r < GB_DISPLAY_H; r++) {
      const uint8_t *gbLine  = pixelBuf[rowLUT[r]];
      uint16_t      *lcdLine = frameBuf + r * LCD_W;
      for (int c = 0; c < LCD_W; c++) {
        lcdLine[c] = pal[gbLine[c >> 1] & 0x03];
      }
    }
    M5.Lcd.pushImage(0, 0, LCD_W, GB_DISPLAY_H, frameBuf);
  } else {
    /* フォールバック: 行バッファで 1 行ずつ転送 (Core1 低メモリ環境) */
    static uint16_t rowBuf[LCD_W];
    M5.Lcd.startWrite();
    for (int r = 0; r < GB_DISPLAY_H; r++) {
      const uint8_t *gbLine = pixelBuf[rowLUT[r]];
      for (int c = 0; c < LCD_W; c++) {
        rowBuf[c] = pal[gbLine[c >> 1] & 0x03];
      }
      M5.Lcd.setAddrWindow(0, r, LCD_W, 1);
      M5.Lcd.pushColors(rowBuf, LCD_W, true);
    }
    M5.Lcd.endWrite();
  }
}

/* ── メッセージ表示 ────────────────────────────────────────────────────── */
void displayShowMessage(const char *msg) {
  M5.Lcd.fillScreen(TFT_BLACK);
  M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Lcd.setTextSize(2);

  /* 文字列を '\n' で分割して中央揃え表示 */
  const int lineH   = 20;
  char buf[256];
  strncpy(buf, msg, sizeof(buf) - 1);
  buf[sizeof(buf) - 1] = '\0';

  /* 行数カウント */
  int lines = 1;
  for (int i = 0; buf[i]; i++) {
    if (buf[i] == '\n') lines++;
  }

  int y = (LCD_H - lines * lineH) / 2;
  char *line = strtok(buf, "\n");
  while (line) {
    int x = (LCD_W - (int)strlen(line) * 12) / 2;
    if (x < 0) x = 0;
    M5.Lcd.setCursor(x, y);
    M5.Lcd.print(line);
    y += lineH;
    line = strtok(nullptr, "\n");
  }
}
