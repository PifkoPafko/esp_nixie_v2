/****************************************************************************
 * Copyright (C) 2025 by Paweł Smarkucki                                    *
 *                                                                          *
 *   This file is part of NIXIE B16.                                        *
 *                                                                          *
 *   NIXIE B16 is free software: you can redistribute it, modify it,        *
 *   sell it and do whatever you want under no terms or conditions.         *
 *                                                                          *
 *   NIXIE B16 is distributed in the hope that it will be useful,           *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                   *
 ****************************************************************************/

#ifndef __ALARM_H__
#define __ALARM_H__

/* Headers */
// #include "project_defs.h"
// #include "ctype.h"
// #include "stdlib.h"
// #include "string.h"
// #include  <stdbool.h>

// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "freertos/semphr.h"

// #include "driver/gptimer.h"
// #include "driver/gpio.h"
// #include "driver/i2s_std.h" // i2s setup

// #include "esp_err.h"
// #include "esp_log.h"
// #include "stdlib.h"
// #include <sys/time.h>
// #include <time.h>

// #include "pp_object_manager.h"
// #include "pp_object_transfer_attr_ids.h"
#include "pp_object_transfer_defs.h"
// #include "pp_wave_player.h"

/* Macros */
// #define ALARM_LOG

#define ALARM_SINGLE_MODE   0
#define ALARM_WEEKLY_MODE   1
#define ALARM_MONTHLY_MODE  2
#define ALARM_YEARLY_MODE   3

#define ALARM_FIELD_SIZE        1
#define ALARM_ENABLE_SIZE       1
#define ALARM_MODE_SIZE         1
#define ALARM_DESC_LEN_SIZE     1
#define ALARM_VOLUME_SIZE  1

#define ALARM_MODES_NUM         4

#define ALARM_MODE_SINGLE_PAYLOAD_SIZE_MIN         9
#define ALARM_MODE_SINGLE_PAYLOAD_SIZE_MAX         49

#define ALARM_MODE_WEEKLY_PAYLOAD_SIZE_MIN         7
#define ALARM_MODE_WEEKLY_PAYLOAD_SIZE_MAX         47

#define ALARM_MODE_MONTHLY_PAYLOAD_SIZE_MIN        7
#define ALARM_MODE_MONTHLY_PAYLOAD_SIZE_MAX        47

#define ALARM_MODE_YEARLY_PAYLOAD_SIZE_MIN         8
#define ALARM_MODE_YEARLY_PAYLOAD_SIZE_MAX         48

#define ALARM_MODE_PAYLOAD_SIZE_MIN                ALARM_MODE_WEEKLY_PAYLOAD_SIZE_MIN
#define ALARM_MODE_PAYLOAD_SIZE_MAX                ALARM_MODE_SINGLE_PAYLOAD_SIZE_MAX

#define ALARM_DESC_LEN_MAX                         40
#define ALARM_VOLUME_MAX                      100


#define ONE_WEEK_IN_SEC     604800
#define DAYS_TO_SEC(x)     ( (x) * 24ll * 60ll * 60ll )

#define AUDIO_BUFFER 2048           // buffer size for reading the wav file and sending to i2s
#define WAV_FILE "/sdcard/ringtone0.wav" // wav file to play

#define ALARM_TIMER_NOTIFICATION    0
#define ALARM_START_NOTIFICATION    1
#define ALARM_STOP_NOTIFICATION     2

/* Structures */
typedef enum {
    WAIT_FOR_START_PLAY,
    SET_RINGTONE,
    SET_DATA_POSITION,
    ALARM_PLAY,
    SET_NEXT_ALARM,
}alarm_play_sm_t;

/** @brief pp_alarm_init: Alarm player initialization function
 * 
 * This functions initializes i2s functionality and creates a task that manages alarm playing. It also initializes
 * the timer used to measure interval to next alarm or current playing alarm remaining playing time.
 * The timer is set to notify the alarm task when the next alarm should be played.
 * 
 * @return
 */
void pp_alarm_init(void);

/** @brief pp_set_alarm_values: Set the current alarm values.
 * 
 * This functions sets the values of the current alarm global variable with the bluetooth payload data stream.
 * 
 * @param[in]   payload      (uint8_t *) Write bluetooth payload.
 * @param[in]   payload_len  (uint16_t)  Payload length.
 * 
 * @return (otp_rsp_status_t) Operation result code.
 */
otp_rsp_status_t pp_set_alarm_values(uint8_t *payload, uint16_t payload_len);

/** @brief pp_set_next_alarm: Set the timer for the next alarm.
 * 
 * This function checks all the alarm objects and calculate the time to the nearest alarm. This function
 * takes into consideratation the mode of the alarm (single, weekly, monthly and yearly). This function sets the Timer
 * with the interval to the next alarm. The Timer is disabled if there is no alarm enabled.
 * 
 * @return (void)
 */
void pp_set_next_alarm(void);

#endif
