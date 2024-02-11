#ifndef __PROJECT_DEFS_H__
#define __PROJECT_DEFS_H__

#include <stdbool.h>
#include <stdint.h>
#include <sys/time.h>
#include <time.h>
#include "driver/gpio.h"

#define MOUNT_POINT "/sdcard"
#define FILE_LIST_NAME MOUNT_POINT "/file_id_list.txt"
#define ALARMS_PATH MOUNT_POINT "/alarms"
#define RIGNTONES_PATH MOUNT_POINT "/ringtones"

#define TEMP_FILE_PATH MOUNT_POINT "/temp"

// #define FILE_PATH_FROM_ID(name) MOUNT_POINT "/" name

#define ALARM_FILE_TYPE ".txt"
#define RINGTONE_FILE_TYPE ".wav"

#define GPIO_OUTPUT_OE          GPIO_NUM_3
#define GPIO_OUTPUT_RED         GPIO_NUM_47
#define GPIO_OUTPUT_BLUE        GPIO_NUM_21
#define GPIO_OUTPUT_GREEN       GPIO_NUM_48
#define GPIO_OUTPUT_PIN_SEL     ((1ULL<<GPIO_OUTPUT_OE) | (1ULL<<GPIO_OUTPUT_RED) | (1ULL<<GPIO_OUTPUT_BLUE) | (1ULL<<GPIO_OUTPUT_GREEN))

#define GPIO_INPUT_IO_0     GPIO_NUM_12
#define GPIO_INPUT_IO_1     GPIO_NUM_13
#define GPIO_INPUT_IO_2     GPIO_NUM_14
#define GPIO_INPUT_PIN_SEL  ((1ULL<<GPIO_INPUT_IO_0) | (1ULL<<GPIO_INPUT_IO_1) | (1ULL<<GPIO_INPUT_IO_2))

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
    ALARM_CHANGE_MODE,
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

void set_device_mode(device_mode_t mode);
device_mode_t get_device_mode();
void time_change_mode(button_action_t action_handler, bool start);
void alarm_add_mode(button_action_t action_handler, bool start);



#endif