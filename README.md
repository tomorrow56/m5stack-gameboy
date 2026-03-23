# M5Stack Game Boy Emulator

> **開発中**

M5Stack (ESP32) 向け Game Boy / Game Boy Color エミュレータ。
エミュレータコアに [peanut-gb](https://github.com/deltabeard/Peanut-GB) を使用。

## 対応ハードウェア

| ハードウェア | 推奨度 | 備考 |
| --- | --- | --- |
| M5Stack Core2 | ★★★ | PSRAM 8MB、スピーカー内蔵 |
| M5Stack Fire | ★★☆ | PSRAM 4MB |
| M5Stack Basic/Gray | ★☆☆ | PSRAM なし・ROM サイズ制限あり |

## 機能

- Game Boy (DMG) / Game Boy Color (GBC) ROM 対応
- SD カードから ROM 自動検索・選択メニュー
- SRAM 自動保存（30 秒ごと）
- 320×240 フル画面スケーリング（最近傍補間）
- 4 色パレット（クラシックグリーン / グレー / ウォーム切替）
- I2S オーディオ出力（`ENABLE_SOUND=1` 時）
- タッチ仮想ゲームパッド (Core2)
- 物理 3 ボタン操作

## セットアップ

### 必要なもの

- M5Stack Core2（推奨）
- micro SD カード（FAT32 フォーマット）
- `.gb` / `.gbc` ROM ファイル（著作権に注意）
- [`peanut_gb.h`](https://github.com/deltabeard/Peanut-GB/raw/master/peanut_gb.h) を `m5stack_gameboy/` に配置

### peanut_gb.h の取得

```bash
curl -L https://raw.githubusercontent.com/deltabeard/Peanut-GB/master/peanut_gb.h \
     -o m5stack_gameboy/peanut_gb.h
curl -L https://raw.githubusercontent.com/deltabeard/minigb_apu/master/minigb_apu.h \
     -o m5stack_gameboy/minigb_apu.h
```

### Arduino IDE でビルドする

1. `m5stack_gameboy/config.h` を開き、ハードウェアに合わせて設定する
   - Core2 → `#define M5CORE2`（デフォルト）
   - Core/Basic/Gray/Fire → `#define M5CORE2` をコメントアウト
2. Arduino IDE で以下のライブラリをインストール
   - Core2: **M5Core2** (`m5stack/M5Core2`)
   - Core:  **M5Stack** (`m5stack/M5Stack`)
3. `m5stack_gameboy/m5stack_gameboy.ino` を Arduino IDE で開く
4. ボードを選択してアップロード

### PlatformIO でビルドする

```bash
# Core2 の場合
pio run -e m5stack-core2 -t upload

# Core の場合
pio run -e m5stack-core -t upload
```

## SD カードのフォルダ構成

```text
SD カード/
├── game1.gb      ← ROM ファイル（ルートに配置）
├── game2.gbc
├── game1.sav     ← SRAM 保存（自動生成）
└── game2.sav
```

## 操作方法

### M5Stack Core2（タッチ仮想ボタン）

画面下部に仮想ゲームパッドが表示されます。

| タッチ領域 | GB ボタン |
| --- | --- |
| 左下エリア（十字キー） | 上下左右 |
| 右下 A | A ボタン |
| 右下 B | B ボタン |
| 中央下 SELECT | SELECT |
| 中央下 START | START |

### 物理ボタン（全機種共通）

| M5Stack ボタン | 機能 |
| --- | --- |
| BtnA 長押し | メニューに戻る |
| BtnB 長押し | パレット切替 |
| BtnC 長押し | SRAM 強制保存 |

## アーキテクチャ

```text
m5stack_gameboy/
 ├── m5stack_gameboy.ino  ← メインスケッチ (Arduino IDE / PlatformIO)
 ├── config.h             ← ユーザー設定（ハードウェア・オーディオ）
 ├── peanut_gb.h          ← Game Boy エミュレータコア（C 実装）
 ├── minigb_apu.h         ← APU エミュレータ
 ├── storage.cpp/h        ← ROM/SRAM の SD カード読み書き
 ├── display.cpp/h        ← LCD スケーリング・フレームバッファ
 ├── input.cpp/h          ← ボタン・タッチ入力
 └── audio.cpp/h          ← I2S オーディオ出力
```

## 注意事項

- ROM の使用は著作権法に従ってください
- 自分で所有するソフトのバックアップのみ使用可
- GBC ゲームは DMG 互換モードで動作します
