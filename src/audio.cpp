#include "audio.h"
#include <Arduino.h>

#if ENABLE_SOUND

extern "C" {
#include "minigb_apu.h"
}

#ifdef M5CORE2
#  include <M5Core2.h>
#  include <driver/i2s.h>
#else
#  include <M5Stack.h>
#  include <driver/i2s.h>
#endif

/* ── I2S 設定 ─────────────────────────────────────────────────────────── */
#ifdef M5CORE2
/* M5Stack Core2: NS4168 DAC (I2S_NUM_0) */
#  define I2S_PORT        I2S_NUM_0
#  define I2S_BCK_PIN     12
#  define I2S_LRCK_PIN     0
#  define I2S_DATA_PIN     2
#else
/* M5Stack Core1: 内蔵 DAC を使用 (I2S_NUM_0, GPIO25) */
#  define I2S_PORT        I2S_NUM_0
#  define I2S_BCK_PIN     -1
#  define I2S_LRCK_PIN    -1
#  define I2S_DATA_PIN    25
#endif

/* ── APU コンテキスト ────────────────────────────────────────────────── */
static struct minigb_apu_ctx apuCtx;
static bool muteFlag = false;

/* peanut-gb が直接呼び出す APU I/O (minigb_apu に委譲) */
extern "C" uint8_t audio_read(uint16_t addr) {
  return minigb_apu_audio_read(&apuCtx, addr);
}

extern "C" void audio_write(uint16_t addr, uint8_t val) {
  minigb_apu_audio_write(&apuCtx, addr, val);
}

/* ── 初期化 ────────────────────────────────────────────────────────────── */
void audioInit() {
  minigb_apu_audio_init(&apuCtx);

#ifdef M5CORE2
  /* I2S ドライバ設定 */
  i2s_config_t i2sCfg = {
    .mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate          = AUDIO_SAMPLE_RATE,
    .bits_per_sample      = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format       = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count        = 4,
    .dma_buf_len          = AUDIO_SAMPLES_TOTAL,
    .use_apll             = false,
    .tx_desc_auto_clear   = true,
    .fixed_mclk           = 0
  };

  i2s_pin_config_t pinCfg = {
    .bck_io_num   = I2S_BCK_PIN,
    .ws_io_num    = I2S_LRCK_PIN,
    .data_out_num = I2S_DATA_PIN,
    .data_in_num  = I2S_PIN_NO_CHANGE
  };

  i2s_driver_install(I2S_PORT, &i2sCfg, 0, nullptr);
  i2s_set_pin(I2S_PORT, &pinCfg);
  i2s_zero_dma_buffer(I2S_PORT);

  /* AW88298 アンプを有効化 */
  M5.Axp.SetSpkEnable(true);

#else
  /* Core1: 内蔵 DAC モード */
  i2s_config_t i2sCfg = {
    .mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN),
    .sample_rate          = AUDIO_SAMPLE_RATE,
    .bits_per_sample      = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format       = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_MSB,
    .intr_alloc_flags     = 0,
    .dma_buf_count        = 4,
    .dma_buf_len          = AUDIO_SAMPLES_TOTAL,
    .use_apll             = false,
    .tx_desc_auto_clear   = true,
    .fixed_mclk           = 0
  };

  i2s_driver_install(I2S_PORT, &i2sCfg, 0, nullptr);
  i2s_set_dac_mode(I2S_DAC_CHANNEL_RIGHT_EN);
#endif

  Serial.printf("[audio] Init OK  sample_rate=%d  buf=%d\n",
                AUDIO_SAMPLE_RATE, AUDIO_SAMPLES_TOTAL);
}

/* ── フレームごとのオーディオ更新 ─────────────────────────────────────── */
void audioUpdate() {
  static audio_sample_t samples[AUDIO_SAMPLES_TOTAL];

  minigb_apu_audio_callback(&apuCtx, samples);

  if (muteFlag) return;

  size_t bytesWritten = 0;
  i2s_write(I2S_PORT, samples,
            AUDIO_SAMPLES_TOTAL * sizeof(audio_sample_t),
            &bytesWritten, portMAX_DELAY);
}

/* ── ミュート制御 ──────────────────────────────────────────────────────── */
void audioSetMute(bool mute) {
  muteFlag = mute;
  if (mute) {
    i2s_zero_dma_buffer(I2S_PORT);
  }
}

#else  /* ENABLE_SOUND == 0 */

void audioInit()              {}
void audioUpdate()            {}
void audioSetMute(bool /*m*/) {}

#endif /* ENABLE_SOUND */
