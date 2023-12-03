#ifndef __ALARM_H__
#define __ALARM_H__

#include "project_defs.h"
#include "ctype.h"
#include "stdlib.h"
#include "string.h"
#include  <stdbool.h>

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

uint8_t set_alarm_values(uint8_t *payload, uint16_t payload_len);
alarm_mode_args_t get_alarm_values();
alarm_mode_args_t* get_alarm_pointer();

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

#endif
