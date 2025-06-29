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

/* Headers */
#include "pp_nixie_display_manager.h"

#include <stdbool.h>
#include <stdint.h>
#include <sys/time.h>
#include <time.h>
#include "driver/gpio.h"

/* Macros */
#define ALARM_FILE_TYPE ".txt"
#define RINGTONE_FILE_TYPE ".wav"

#define BUTTON_LEFT      0
#define BUTTON_CENTER    1
#define BUTTON_RIGHT     2

#define ESP_INTR_FLAG_DEFAULT ESP_INTR_FLAG_EDGE

#define ALARM_SINGLE_MODE   0
#define ALARM_WEEKLY_MODE   1
#define ALARM_MONTHLY_MODE  2
#define ALARM_YEARLY_MODE   3

typedef enum {
    WAIT_FOR_LEFT,
    WAIT_FOR_CENTER,
    WAIT_FOR_LEFT_LONG,
    WAIT_FOR_CENTER_LONG,
    PAIRING
}pairing_sm_t;

typedef enum {
    DEFAULT_MODE = 0,
    TIME_CHANGE_MODE,
    ALARM_ADD_MODE,
    ALARM_DELETE_MODE,
    PAIRING_MODE,
    PAIRING_PASSKEY_MODE,
    ALARM_RING_MODE
}device_mode_t;

typedef enum {
    IDLE_TIME_CHANGE,
    SET_HOUR_FIRST,
    SET_HOUR_SECOND,
    SET_MINUTE_FIRST,
    SET_MINUTE_SECOND,
    SET_SECOND_FIRST,
    SET_SECOND_SECOND,
    SET_DAY_FIRST,
    SET_DAY_SECOND,
    SET_MONTH_FIRST,
    SET_MONTH_SECOND,
    SET_YEAR_FIRST,
    SET_YEAR_SECOND
}time_change_sm_t;

typedef enum {
    IDLE_ALARM_ADD,
    SET_MODE,
    SET_ALARM_HOUR_FIRST,
    SET_ALARM_HOUR_SECOND,
    SET_ALARM_MINUTE_FIRST,
    SET_ALARM_MINUTE_SECOND,
    SET_SINGLE_DAY_FIRST,
    SET_SINGLE_DAY_SECOND,
    SET_SINGLE_MONTH_FIRST,
    SET_SINGLE_MONTH_SECOND,
    SET_SINGLE_YEAR_FIRST,
    SET_SINGLE_YEAR_SECOND,
    SET_WEEKLY_MONDAY,
    SET_WEEKLY_TUESDAY,
    SET_WEEKLY_WEDNESDAY,
    SET_WEEKLY_THURSDAY,
    SET_WEEKLY_FRIDAY,
    SET_WEEKLY_SATURDAY,
    SET_WEEKLY_SUNDAY,
    SET_MONTHLY_DAY_FIRST,
    SET_MONTHLY_DAY_SECOND,
    SET_YEARLY_DAY_FIRST,
    SET_YEARLY_DAY_SECOND,
    SET_YEARLY_MONTH_FIRST,
    SET_YEARLY_MONTH_SECOND,
    SET_VOLUME
}alarm_add_sm_t;

typedef struct 
{
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
    time_digits_t time;
    date_digits_t date;
} time_date_digits_t;

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
    uint8_t blink_tube;
    union {
        time_date_digits_t time_date_digits;
        alarm_digits_t alarm_digits;
    } display_mode;
}display_digits_t;

void pp_time_change_mode(button_action_t action_handler, bool start);
void pp_alarm_add_mode(button_action_t action_handler, bool start);
void pp_alarm_delete_mode(button_action_t action_handler);