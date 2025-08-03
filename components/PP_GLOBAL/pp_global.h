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

#ifndef __GLOBAL_H__
#define __GLOBAL_H__

/* Headers */
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* Macros */
#define DEVICE_NAME     "NIXIE B16"

#define NOTIFY_TASK(x, y) (xTaskNotify(x, y, eSetValueWithOverwrite))
#define NOTIFY_TASK_FROM_ISR(x, y) (xTaskNotifyFromISR(x, y, eSetValueWithOverwrite, NULL))

/* Alarm mode */
#define ALARM_SINGLE_MODE   0
#define ALARM_WEEKLY_MODE   1
#define ALARM_MONTHLY_MODE  2
#define ALARM_YEARLY_MODE   3

#define ESP_UUID_LEN_128    16

/* Structures */
typedef enum {
    DEFAULT_MODE = 0,
    TIME_CHANGE_MODE,
    ALARM_ADD_MODE,
    ALARM_DELETE_MODE,
    PAIRING_MODE,
    PAIRING_PASSKEY_MODE,
    ALARM_RING_MODE
}device_mode_t;

typedef struct {
    uint8_t hour_first;
    uint8_t hour_second;
    uint8_t minute_first;
    uint8_t minute_second;
    uint8_t second_first;
    uint8_t second_second;
} time_digits_t;

typedef struct {
    uint8_t day_first;
    uint8_t day_second;
    uint8_t month_first;
    uint8_t month_second;
    uint8_t year_first;
    uint8_t year_second;
} date_digits_t;

typedef struct {
    uint8_t alarm_mode;
    time_digits_t time;

    union {
        date_digits_t date;
        uint8_t days;   // It codes week days in 1-byte: [0 | SUN | SAT | FRI | THU | WED | TUE | MON]
    } arg;
    
    uint8_t volume;
} alarm_digits_t;

typedef struct {
    time_digits_t time;
    date_digits_t date;
} time_date_digits_t;

typedef struct {
    uint8_t blink_tube;
    union {
        time_date_digits_t time_date_digits;
        alarm_digits_t alarm_digits;
    } display_mode;
}display_digits_t;

typedef struct
{
    uint8_t day;
    uint8_t month;
    uint8_t year;
}alarm_single_args_t;

typedef struct
{
    uint8_t day;
    uint8_t month;
}alarm_yearly_args_t;

typedef struct 
{
    bool is_set;
    uint8_t mode;
    uint8_t enable;
    uint8_t desc_len;
    char desc[40];
    uint8_t hour;
    uint8_t minute;

    union {
        alarm_single_args_t single_alarm_args;
        uint8_t days;
        uint8_t day;
        alarm_yearly_args_t yearly_alarm_args;
    } args;

    uint8_t volume;
}alarm_mode_args_t;

/* Variables declarations */
extern TaskHandle_t display_main_h;
extern TaskHandle_t button_main_h;
extern TaskHandle_t rtc_main_h;
extern TaskHandle_t alarm_main_h;
extern TaskHandle_t wifi_main_h;
extern device_mode_t device_mode;
extern alarm_mode_args_t current_alarm;
extern const uint8_t alarm_type_uuid[ESP_UUID_LEN_128];
extern const uint8_t ringtone_type_uuid[ESP_UUID_LEN_128];
extern QueueHandle_t button_action_queue;
extern display_digits_t display_digits;


#endif