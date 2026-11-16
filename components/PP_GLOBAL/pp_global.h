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
#define DISPLAY_ENABLE
#define WIFI_ENABLE

/* I2C Devices */
#define I2C_DEVICE_NUM 7

/* Alarm mode */
#define ALARM_SINGLE_MODE   0
#define ALARM_WEEKLY_MODE   1
#define ALARM_MONTHLY_MODE  2
#define ALARM_YEARLY_MODE   3

/* Bluetooth */
#define ESP_UUID_LEN_128    16
#define MAX_BONDED_DEVICES  5


/* Structures */
typedef enum {
    DEFAULT_MODE = 0,
    TIME_CHANGE_MODE,
    ALARM_ADD_MODE,
    ALARM_DELETE_MODE,
    PAIRING_MODE,
    PAIRING_PASSKEY_MODE,
    ALARM_RING_MODE,
    RINGTONE_LIST_MODE,
    LED_CHANGE_MODE
}device_mode_t;

typedef enum {
    NOTIFY_NORMAL_VAL = 0,
    NOTIFY_TIMER_VAL,
    NOTIFY_TIMER_BLINK_VAL,
    NOTIFY_TIMER_ANTI_POISONING_VAL
}display_update_type_t;

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
    uint8_t number_first;
    uint8_t number_second;
    uint8_t number_third;
} ringtone_digits_t;

typedef struct {
    uint8_t led_lamp_enable;
    uint8_t led_lamp_red;
    uint8_t led_lamp_green;
    uint8_t led_lamp_blue;
    uint8_t led_bottom_enable;
    uint8_t led_bottom_red;
    uint8_t led_bottom_green;
    uint8_t led_bottom_blue;
} led_digits_t;

typedef struct {
    uint8_t blink_tube;
    bool alarm_add_ringtone;
    union {
        time_date_digits_t time_date_digits;
        alarm_digits_t alarm_digits;
        ringtone_digits_t ringtone_digits;
        led_digits_t led_change_digits;
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
    uint8_t ringtone_default;
    uint64_t ringtone_id;
}alarm_mode_args_t;

typedef enum {
    ALARM_TIMER_NOTIFICATION,
    ALARM_START_NOTIFICATION,
    ALARM_STOP_NOTIFICATION
}alarm_notification_type_t;

typedef struct {
    bool enable;
    uint8_t channel;
    uint32_t red;
    uint32_t green;
    uint32_t blue;
}led_update_t;

/* Variables declarations */
extern TaskHandle_t display_main_h;
extern TaskHandle_t button_main_h;
extern TaskHandle_t rtc_main_h;
extern TaskHandle_t alarm_main_h;
extern TaskHandle_t wifi_main_h;
extern TaskHandle_t led_main_h;
extern device_mode_t device_mode;
extern alarm_mode_args_t current_alarm;
extern const uint8_t alarm_type_uuid[ESP_UUID_LEN_128];
extern const uint8_t ringtone_type_uuid[ESP_UUID_LEN_128];
extern QueueHandle_t button_action_queue;
extern QueueHandle_t display_update_queue;
extern QueueHandle_t alarm_queue;
extern QueueHandle_t led_update_queue;
extern display_digits_t display_digits;
extern led_update_t current_led[2];
extern uint8_t gatts_if_curr;
extern uint8_t bt_connection_id_curr;
extern uint8_t bt_wifi_handle;

#endif