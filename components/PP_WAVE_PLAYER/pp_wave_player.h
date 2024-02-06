#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#define AUDIO_BUFFER 2048           // buffer size for reading the wav file and sending to i2s
#define WAV_FILE "/sdcard/ringtone0.wav" // wav file to play

esp_err_t pp_wave_player_init();
void set_play_alarm_flag(bool new_val);