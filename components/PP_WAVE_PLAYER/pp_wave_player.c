#include "pp_wave_player.h"

#include <string.h>
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "driver/i2s_std.h" // i2s setup
#include "alarm.h"
#include "driver/gpio.h"

static const char *TAG = "WAV PLAYER";

i2s_chan_handle_t tx_handle;

volatile bool play_alarm_flag = false;
volatile bool is_alarm_playing = false;

void set_play_alarm_flag(bool new_val)
{
  play_alarm_flag = new_val;
}

bool get_is_alarm_playing()
{
  return is_alarm_playing;
}

static esp_err_t i2s_setup()
{
  // setup a standard config and the channel
  i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
  ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &tx_handle, NULL));

  // setup the i2s config
  i2s_std_config_t std_cfg = {
      .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(44100),                                                    // the wav file sample rate
      .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO), // the wav faile bit and channel config
      .gpio_cfg = {
          // refer to configuration.h for pin setup
          .mclk = I2S_GPIO_UNUSED,
          .bclk = GPIO_NUM_10,
          .ws = GPIO_NUM_11,
          .dout = GPIO_NUM_9,
          .din = I2S_GPIO_UNUSED,
          .invert_flags = {
              .mclk_inv = false,
              .bclk_inv = false,
              .ws_inv = false,
          },
      },
  };
  return i2s_channel_init_std_mode(tx_handle, &std_cfg);
}

static esp_err_t play_wave()
{
  FILE *fh = fopen("/sdcard/ringtone0.wav", "r");
  if (fh == NULL)
  {
    ESP_LOGE(TAG, "Failed to open file");
    return ESP_ERR_INVALID_ARG;
  }

  // skip the header...
  fseek(fh, 44, SEEK_SET);

  // create a writer buffer
  int16_t *buf = calloc(AUDIO_BUFFER, sizeof(int16_t));
  size_t bytes_read = 0;
  size_t bytes_written = 0;

  if (is_alarm_playing)
  {
    bytes_read = fread(buf, sizeof(int16_t), AUDIO_BUFFER, fh);
    for (int i=0; i<bytes_read; i++)
    {
        buf[i] = buf[i]>>1;
    }

    i2s_channel_enable(tx_handle);

    while (bytes_read > 0)
    {
      if (is_alarm_playing)
      {
        // write the buffer to the i2s
        i2s_channel_write(tx_handle, buf, bytes_read * sizeof(int16_t), &bytes_written, portMAX_DELAY);
        bytes_read = fread(buf, sizeof(int16_t), AUDIO_BUFFER, fh);
        for (int i=0; i<bytes_read; i++)
        {
            buf[i] = buf[i]>>1;
        }
        ESP_LOGV(TAG, "Bytes read: %d", bytes_read);
      }
      else
      {
        break;
      }
    }
  }

  ESP_LOGI(TAG, "End of ringtone");
  fclose(fh);

  i2s_channel_disable(tx_handle);
  free(buf);

  return ESP_OK;
}

void pp_wav_player_main(void* arg)
{
  while (true)
  {
    if (play_alarm_flag)
    {
      play_alarm_flag = false;
      is_alarm_playing = true;
      ESP_LOGI(TAG, "Playing wav file");
      ESP_ERROR_CHECK(play_wave(WAV_FILE));
      is_alarm_playing = false;
      gpio_set_level(GPIO_OUTPUT_OE, 0);
      set_next_alarm();
    }

    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

esp_err_t pp_wave_player_init()
{
  ESP_LOGI(TAG, "Initializing wave player");

  ESP_LOGI(TAG, "Initializing i2s");
  esp_err_t res = i2s_setup();
  if (res) 
  {
      ESP_LOGE(TAG, "I2S initialize failed, err: %x", res);
      return res;
  }

  BaseType_t resTask = xTaskCreate(pp_wav_player_main, "WAV PLAYER", 4096, NULL, 1, NULL);
  if(resTask != pdPASS)
  {
      ESP_LOGE(TAG, "Creating wave player task failed, err: %x", resTask);
      return resTask;
  }

  return ESP_OK;
}