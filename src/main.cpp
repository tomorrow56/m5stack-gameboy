/**
 * M5Stack Game Boy Emulator
 * Core: peanut-gb (https://github.com/deltabeard/Peanut-GB)
 * APU : minigb_apu
 *
 * ビルド: platformio.ini 参照
 */

#ifdef M5CORE2
#  include <M5Core2.h>
#else
#  include <M5Stack.h>
#endif

#include <SD.h>
#include <string.h>

/* ── peanut-gb ビルドオプション フォールバック定義 (ビルドフラグが優先) ── */
#ifndef PEANUT_GB_HIGH_LCD_ACCURACY
#  define PEANUT_GB_HIGH_LCD_ACCURACY 0
#endif
#ifndef ENABLE_SOUND
#  define ENABLE_SOUND 0
#endif

/* モジュールヘッダ (この TU のみが peanut_gb.h をインクルードする) */
#include "display.h"
#include "input.h"
#include "audio.h"
#include "storage.h"

extern "C" {
#include "peanut_gb.h"
}

/* ── グローバル状態 ──────────────────────────────────────────────────── */
static struct gb_s gb;
static uint8_t   *romData     = nullptr;
static size_t     romSize     = 0;

static uint8_t    cartRam[128 * 1024];   /* 最大 128 KB カート RAM */
static size_t     cartRamSize = 0;
static bool       cartRamDirty = false;

static char       currentRomPath[256] = {0};
static char       currentSavPath[256] = {0};

/* ── フレームタイミング ──────────────────────────────────────────────── */
static uint32_t frameCount   = 0;
static uint32_t fpsTimer     = 0;
static uint32_t frameTimer   = 0;
static uint32_t sramTimer    = 0;        /* SRAM 自動保存タイマー */
static const uint32_t FRAME_US   = 16742;   /* 1/59.7275 秒 ≈ 16742 µs */
static const uint32_t SRAM_MS    = 30000;   /* 30 秒ごとに自動保存 */

/* ─────────────────────────────────────────────────────────────────────────
 * peanut-gb コールバック
 * ──────────────────────────────────────────────────────────────────────── */

static uint8_t gbRomRead(struct gb_s * /*gb*/, const uint_fast32_t addr) {
  return romData[addr];
}

static uint8_t gbCartRamRead(struct gb_s * /*gb*/, const uint_fast32_t addr) {
  if (addr < cartRamSize) return cartRam[addr];
  return 0xFF;
}

static void gbCartRamWrite(struct gb_s * /*gb*/,
                           const uint_fast32_t addr, const uint8_t val) {
  if (addr < sizeof(cartRam)) {
    cartRam[addr]  = val;
    cartRamDirty   = true;
  }
}

static void gbError(struct gb_s * /*gb*/,
                    const enum gb_error_e err, const uint16_t val) {
  Serial.printf("[gb] Error %d  val=0x%04X\n", err, val);
}

static void gbLcdDrawLine(struct gb_s * /*gb*/,
                          const uint8_t pixels[LCD_WIDTH],
                          const uint_fast8_t line) {
  displayDrawLine(pixels, line);
}

/* ─────────────────────────────────────────────────────────────────────────
 * ユーティリティ
 * ──────────────────────────────────────────────────────────────────────── */

static void getRomTitle(char *buf, size_t bufLen) {
  /* ROM ヘッダ 0x0134-0x0143 にタイトル (16 文字 ASCII, ゼロ終端あり) */
  size_t i = 0;
  for (; i < 16 && i < bufLen - 1; i++) {
    uint8_t c = romData[0x0134 + i];
    if (c == 0) break;
    buf[i] = (char)c;
  }
  buf[i] = '\0';
  if (i == 0) strncpy(buf, "UNKNOWN", bufLen);
}

static void autoSaveSRAM() {
  if (cartRamDirty && cartRamSize > 0) {
    storageSaveSRAM(currentSavPath, cartRam, cartRamSize);
    cartRamDirty = false;
  }
}

/* ─────────────────────────────────────────────────────────────────────────
 * ROM 選択メニュー
 * ──────────────────────────────────────────────────────────────────────── */

static bool selectRom(char *selectedPath) {
  static char paths[32][256];
  int count = storageListROMs(paths, 32);

  if (count == 0) {
    displayShowMessage("SD カード内に\n.gb/.gbc ファイルが\nありません");
    delay(2000);
    return false;
  }

  if (count == 1) {
    strncpy(selectedPath, paths[0], 256);
    return true;
  }

  /* ── メニュー描画ループ ── */
  int selected = 0;
  int scroll   = 0;
  const int ITEMS = 7;
  const int ITEM_H = 28;

  while (true) {
    M5.Lcd.fillScreen(TFT_BLACK);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setTextColor(TFT_CYAN, TFT_BLACK);
    M5.Lcd.setCursor(6, 4);
    M5.Lcd.print("ROM SELECT");

    M5.Lcd.setTextSize(1);
    for (int i = 0; i < ITEMS && (scroll + i) < count; i++) {
      int idx = scroll + i;
      int y   = 30 + i * ITEM_H;

      if (idx == selected) {
        M5.Lcd.fillRect(0, y - 2, 320, ITEM_H, TFT_NAVY);
        M5.Lcd.setTextColor(TFT_WHITE, TFT_NAVY);
      } else {
        M5.Lcd.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
      }

      /* ファイル名 (/ を除去して表示) */
      const char *name = paths[idx];
      if (name[0] == '/') name++;
      char disp[42] = {0};
      strncpy(disp, name, 41);

      M5.Lcd.setCursor(8, y + 5);
      M5.Lcd.print(disp);
    }

    /* スクロールバー */
    if (count > ITEMS) {
      int barH  = LCD_H * ITEMS / count;
      int barY  = 30 + scroll * (LCD_H - 30) / count;
      M5.Lcd.fillRect(315, 30, 5, LCD_H - 30, TFT_DARKGREY);
      M5.Lcd.fillRect(315, barY, 5, barH, TFT_WHITE);
    }

    /* ボタンヒント */
    M5.Lcd.setTextColor(TFT_DARKGREY, TFT_BLACK);
    M5.Lcd.setCursor(2, 226);
#ifdef M5CORE2
    M5.Lcd.print("[A]Up  [C]Down  Touch=Select");
#else
    M5.Lcd.print("[A]Up  [B]Down  [C]Select");
#endif

    /* ── 入力待ち ── */
    while (true) {
      M5.update();

      bool moved = false;

      if (M5.BtnA.wasPressed()) {
        if (selected > 0) { selected--; if (selected < scroll) scroll = selected; }
        moved = true;
      }

#ifdef M5CORE2
      if (M5.BtnC.wasPressed()) {
        if (selected < count - 1) { selected++; if (selected >= scroll + ITEMS) scroll = selected - ITEMS + 1; }
        moved = true;
      }
      TouchPoint_t tp = M5.Touch.getPressPoint();
      if (tp.x >= 0 && tp.y >= 30 && tp.y < 30 + ITEMS * ITEM_H) {
        int idx = scroll + (tp.y - 30) / ITEM_H;
        if (idx >= 0 && idx < count) {
          strncpy(selectedPath, paths[idx], 256);
          return true;
        }
      }
#else
      if (M5.BtnB.wasPressed()) {
        if (selected < count - 1) { selected++; if (selected >= scroll + ITEMS) scroll = selected - ITEMS + 1; }
        moved = true;
      }
      if (M5.BtnC.wasPressed()) {
        strncpy(selectedPath, paths[selected], 256);
        return true;
      }
#endif
      if (moved) break;
      delay(10);
    }
  }
}

/* ─────────────────────────────────────────────────────────────────────────
 * エミュレータ初期化
 * ──────────────────────────────────────────────────────────────────────── */

static bool initEmulator(const char *romPath) {
  /* ROM ロード */
  romData = storageLoadROM(romPath, &romSize);
  if (!romData) {
    displayShowMessage("ROM 読み込み失敗");
    return false;
  }

  /* peanut-gb 初期化 */
  enum gb_init_error_e err = gb_init(&gb,
                                     gbRomRead,
                                     gbCartRamRead,
                                     gbCartRamWrite,
                                     gbError,
                                     nullptr);
  if (err != GB_INIT_NO_ERROR) {
    char msg[64];
    snprintf(msg, sizeof(msg), "GB 初期化エラー\nCode: %d", (int)err);
    displayShowMessage(msg);
    storageFreeROM();
    return false;
  }

  /* カート RAM サイズ取得 */
  size_t ramSz = 0;
  int sErr = gb_get_save_size_s(&gb, &ramSz);
  cartRamSize = (sErr == 0) ? ramSz : 0;
  memset(cartRam, 0xFF, sizeof(cartRam));

  /* セーブファイルロード */
  storageMakeSavePath(romPath, currentSavPath);
  if (cartRamSize > 0) {
    storageLoadSRAM(currentSavPath, cartRam, cartRamSize);
  }

  /* LCD コールバック設定 */
  gb_init_lcd(&gb, gbLcdDrawLine);

  /* タイトル表示 */
  char title[17];
  getRomTitle(title, sizeof(title));
  Serial.printf("[main] ROM: %s  cartRAM: %u bytes\n", title, (unsigned)cartRamSize);

  char msg[48];
  snprintf(msg, sizeof(msg), "%.16s\nLoading...", title);
  displayShowMessage(msg);
  delay(800);

  return true;
}

/* ─────────────────────────────────────────────────────────────────────────
 * Arduino setup
 * ──────────────────────────────────────────────────────────────────────── */

void setup() {
  Serial.begin(115200);

#ifdef M5CORE2
  M5.begin(true, true, true, true);
#else
  M5.begin(true, true, true);
#endif

  displayInit();
  displayShowMessage("M5Stack GameBoy\nInitializing...");

  /* SD カード確認 */
  if (!SD.begin()) {
    displayShowMessage("SD カードが\n見つかりません\n挿入してリセット");
    while (true) { M5.update(); delay(100); }
  }

  /* ROM 選択 */
  if (!selectRom(currentRomPath)) {
    while (true) { M5.update(); delay(100); }
  }

  /* エミュレータ初期化 */
  if (!initEmulator(currentRomPath)) {
    while (true) { M5.update(); delay(100); }
  }

  /* 入力・オーディオ初期化 */
  inputInit();
  audioInit();

  frameTimer = micros();
  fpsTimer   = millis();
  sramTimer  = millis();
  frameCount = 0;

  Serial.println("[main] Setup complete");
}

/* ─────────────────────────────────────────────────────────────────────────
 * Arduino loop
 * ──────────────────────────────────────────────────────────────────────── */

/* メニュー系の長押し管理 */
static uint32_t btnAHold = 0;
static uint32_t btnBHold = 0;
static uint32_t btnCHold = 0;

void loop() {
  M5.update();

  /* ── 長押しメニューアクション ── */
  if (M5.BtnA.isPressed()) {
    if (btnAHold == 0) btnAHold = millis();
    if (millis() - btnAHold > 2000) {
      /* A 長押し: ROM 選択に戻る */
      autoSaveSRAM();
      storageFreeROM();
      ESP.restart();
    }
  } else {
    btnAHold = 0;
  }

  if (M5.BtnB.isPressed()) {
    if (btnBHold == 0) btnBHold = millis();
    if (millis() - btnBHold > 1500) {
      /* B 長押し: パレット切替 */
      PaletteType next = (PaletteType)((displayGetPalette() + 1) % PALETTE_COUNT);
      displaySetPalette(next);
      btnBHold = millis() + 500;  /* チャタリング防止 */
    }
  } else {
    btnBHold = 0;
  }

  if (M5.BtnC.isPressed()) {
    if (btnCHold == 0) btnCHold = millis();
    if (millis() - btnCHold > 1500) {
      /* C 長押し: SRAM 強制保存 */
      if (cartRamSize > 0) {
        storageSaveSRAM(currentSavPath, cartRam, cartRamSize);
        cartRamDirty = false;
      }
      btnCHold = millis() + 500;
    }
  } else {
    btnCHold = 0;
  }

  /* ── フレームタイミング制御 (59.7275 fps) ── */
  uint32_t now = micros();
  if ((uint32_t)(now - frameTimer) < FRAME_US) {
    delayMicroseconds(FRAME_US - (uint32_t)(now - frameTimer));
  }
  frameTimer = micros();

  /* ── 入力更新 ── */
  inputUpdate(&gb.direct.joypad);

  /* ── エミュレータ 1 フレーム実行 ── */
  gb_run_frame(&gb);

  /* ── 画面転送 ── */
  displayPushFrame();

  /* ── オーディオ出力 ── */
  audioUpdate();

  /* ── SRAM 自動保存 (30 秒ごと、millis ベース) ── */
  if (cartRamDirty && cartRamSize > 0 && (millis() - sramTimer >= SRAM_MS)) {
    storageSaveSRAM(currentSavPath, cartRam, cartRamSize);
    cartRamDirty = false;
    sramTimer    = millis();
  }

  /* ── FPS デバッグ出力 (Serial, 1 秒ごと) ── */
  frameCount++;
  if (millis() - fpsTimer >= 1000) {
    Serial.printf("[fps] %u\n", frameCount);
    frameCount = 0;
    fpsTimer   = millis();
  }
}
