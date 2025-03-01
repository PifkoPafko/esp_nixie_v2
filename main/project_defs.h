#ifndef __PROJECT_DEFS_H__
#define __PROJECT_DEFS_H__

#include <stdbool.h>
#include <stdint.h>
#include <sys/time.h>
#include <time.h>
#include "driver/gpio.h"

// #define FILE_PATH_FROM_ID(name) MOUNT_POINT "/" name

#define ALARM_FILE_TYPE ".txt"
#define RINGTONE_FILE_TYPE ".wav"

#define BUTTON_LEFT      0
#define BUTTON_CENTER    1
#define BUTTON_RIGHT     2

#define ESP_INTR_FLAG_DEFAULT ESP_INTR_FLAG_EDGE

typedef enum {
    IDLE,
    WAIT_ENABLE_CONTACT_VIBRATION,
    WAIT_LONG_PRESS,
    WAIT_DISABLE,
    WAIT_DISABLE_CONTACT_VIBRATION
}gpio_state_t;

typedef struct 
{
    gpio_state_t state;
    bool last_enable;
}gpio_sm_t;

typedef struct
{
    bool enable;
    uint32_t gpio_num;
}button_queue_msg_t;

typedef enum {
    SHORT_PRESS,
    HOLDING_SHORT,
    LONG_PRESS,
}action_mode_t;

typedef struct
{
    uint8_t button;
    action_mode_t action;
}button_action_t;

typedef enum {
    WAIT_FOR_LEFT,
    WAIT_FOR_CENTER,
    WAIT_FOR_LEFT_LONG,
    WAIT_FOR_CENTER_LONG,
    PAIRING
}pairing_sm_t;

typedef enum {
    DEFAULT_MODE,
    TIME_CHANGE_MODE,
    ALARM_ADD_MODE,
    ALARM_DELETE_MODE,
    PAIRING_MODE,
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

typedef struct {
    uint8_t hour_first;
    uint8_t hour_second;
    uint8_t minute_first;
    uint8_t minute_second;
    uint8_t second_first;
    uint8_t second_second;
    uint8_t day_first;
    uint8_t day_second;
    uint8_t month_first;
    uint8_t month_second;
    uint8_t year_first;
    uint8_t year_second;
} nixie_time_t;

typedef struct {
    uint8_t mode;
    nixie_time_t time;
    uint8_t monday;
    uint8_t tuesday;
    uint8_t wednesday;
    uint8_t thursday;
    uint8_t friday;
    uint8_t saturday;
    uint8_t sunday;
    uint8_t volume;
} alarm_add_digits_t;

void pp_set_device_mode(device_mode_t mode);
device_mode_t pp_get_device_mode();
void pp_time_change_mode(button_action_t action_handler, bool start);
void pp_alarm_add_mode(button_action_t action_handler, bool start);
void pp_alarm_delete_mode(button_action_t action_handler);

#endif