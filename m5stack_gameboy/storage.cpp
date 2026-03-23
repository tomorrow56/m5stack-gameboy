#include "config.h"
#include "storage.h"
#include <Arduino.h>
#include <SD.h>
#include <string.h>

static uint8_t *romBuffer = nullptr;

uint8_t *storageLoadROM(const char *path, size_t *size) {
  storageFreeROM();

  File f = SD.open(path, FILE_READ);
  if (!f) {
    Serial.printf("[storage] Cannot open: %s\n", path);
    return nullptr;
  }

  size_t fileSize = f.size();
  if (fileSize == 0) {
    f.close();
    return nullptr;
  }

#ifndef BOARD_HAS_PSRAM
  /* Core1 (PSRAM なし): 安全に使えるヒープを確保できる最大サイズをチェック */
  const size_t MAX_ROM_NO_PSRAM = 256 * 1024;  /* 256 KB */
  if (fileSize > MAX_ROM_NO_PSRAM) {
    Serial.printf("[storage] ROM too large for no-PSRAM build: %u bytes (max %u)\n",
                  (unsigned)fileSize, (unsigned)MAX_ROM_NO_PSRAM);
    f.close();
    return nullptr;
  }
#endif

#ifdef BOARD_HAS_PSRAM
  romBuffer = (uint8_t *)ps_malloc(fileSize);
#else
  romBuffer = (uint8_t *)malloc(fileSize);
#endif

  if (!romBuffer) {
    Serial.printf("[storage] Alloc failed for %u bytes\n", (unsigned)fileSize);
    f.close();
    return nullptr;
  }

  size_t bytesRead = 0;
  const size_t CHUNK = 4096;
  while (bytesRead < fileSize) {
    size_t toRead = min(CHUNK, fileSize - bytesRead);
    size_t n = f.read(romBuffer + bytesRead, toRead);
    if (n == 0) break;
    bytesRead += n;
  }
  f.close();

  if (bytesRead != fileSize) {
    Serial.printf("[storage] Read error: got %u / %u bytes\n",
                  (unsigned)bytesRead, (unsigned)fileSize);
    storageFreeROM();
    return nullptr;
  }

  Serial.printf("[storage] Loaded %u bytes from %s\n", (unsigned)fileSize, path);
  *size = fileSize;
  return romBuffer;
}

void storageFreeROM() {
  if (romBuffer) {
#ifdef BOARD_HAS_PSRAM
    ps_free(romBuffer);
#else
    free(romBuffer);
#endif
    romBuffer = nullptr;
  }
}

bool storageSaveSRAM(const char *path, const uint8_t *data, size_t size) {
  if (size == 0 || !data) return true;

  File f = SD.open(path, FILE_WRITE);
  if (!f) {
    Serial.printf("[storage] Cannot write: %s\n", path);
    return false;
  }

  size_t written = f.write(data, size);
  f.close();

  if (written != size) {
    Serial.printf("[storage] Save error: wrote %u / %u\n",
                  (unsigned)written, (unsigned)size);
    return false;
  }

  Serial.printf("[storage] SRAM saved (%u bytes) -> %s\n", (unsigned)size, path);
  return true;
}

bool storageLoadSRAM(const char *path, uint8_t *data, size_t size) {
  if (size == 0 || !data) return false;

  if (!SD.exists(path)) return false;

  File f = SD.open(path, FILE_READ);
  if (!f) return false;

  size_t toRead = min(size, (size_t)f.size());
  size_t n = f.read(data, toRead);
  f.close();

  Serial.printf("[storage] SRAM loaded (%u bytes) from %s\n", (unsigned)n, path);
  return (n > 0);
}

int storageListROMs(char paths[][256], int maxFiles) {
  int count = 0;
  File root = SD.open("/");
  if (!root) return 0;

  File entry = root.openNextFile();
  while (entry && count < maxFiles) {
    if (!entry.isDirectory()) {
      const char *name = entry.name();
      size_t len = strlen(name);
      bool isGB = false;
      if (len > 3 && strcasecmp(name + len - 3, ".gb") == 0) {
        isGB = true;
      } else if (len > 4 && strcasecmp(name + len - 4, ".gbc") == 0) {
        isGB = true;
      }
      if (isGB) {
        snprintf(paths[count], 256, "/%s", name);
        count++;
      }
    }
    entry = root.openNextFile();
  }
  root.close();
  return count;
}

void storageMakeSavePath(const char *romPath, char *savePath) {
  strncpy(savePath, romPath, 255);
  savePath[255] = '\0';
  char *dot = strrchr(savePath, '.');
  if (dot) {
    strncpy(dot, ".sav", 5);
  } else {
    strncat(savePath, ".sav", 255 - strlen(savePath));
  }
}
