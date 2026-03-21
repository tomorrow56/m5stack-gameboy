#pragma once
#include <stdint.h>
#include <stddef.h>

/**
 * @brief SD カードから ROM ファイルを PSRAM (または heap) にロードする
 * @param path  SD カード上のファイルパス (例: "/rom.gb")
 * @param size  ロードしたバイト数を返す
 * @return      ROM データへのポインタ（失敗時は nullptr）
 *              呼び出し元が free() する必要はない（static/PSRAM 管理）
 */
uint8_t *storageLoadROM(const char *path, size_t *size);

/**
 * @brief カート RAM (SRAM) を SD カードに保存する
 * @param path  保存先ファイルパス (例: "/rom.sav")
 * @param data  保存するデータ
 * @param size  データサイズ（0 の場合は何もしない）
 * @return      成功: true
 */
bool storageSaveSRAM(const char *path, const uint8_t *data, size_t size);

/**
 * @brief SD カードからカート RAM (SRAM) をロードする
 * @param path  ロードするファイルパス
 * @param data  書き込み先バッファ
 * @param size  バッファサイズ
 * @return      成功: true（ファイルが無い場合は false を返し data は変更しない）
 */
bool storageLoadSRAM(const char *path, uint8_t *data, size_t size);

/**
 * @brief SD カードのルートにある .gb / .gbc ファイルを列挙する
 * @param paths     見つかったパスを格納する char* 配列（呼び出し元が用意）
 * @param maxFiles  配列のサイズ
 * @return          見つかったファイル数
 */
int storageListROMs(char paths[][256], int maxFiles);

/**
 * @brief ROM パスから保存ファイルパスを生成する (.gb → .sav)
 * @param romPath  ROM のパス
 * @param savePath 出力先バッファ (256 バイト)
 */
void storageMakeSavePath(const char *romPath, char *savePath);

/** @brief ROM データを解放して nullptr に戻す */
void storageFreeROM();
