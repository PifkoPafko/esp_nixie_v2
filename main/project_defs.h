#ifndef __PROJECT_DEFS_H__
#define __PROJECT_DEFS_H__

#define MOUNT_POINT "/sdcard"
#define FILE_LIST_NAME MOUNT_POINT "/file_id_list.txt"
#define ALARMS_PATH MOUNT_POINT "/alarms"
#define RIGNTONES_PATH MOUNT_POINT "/ringtones"

#define TEMP_FILE_PATH MOUNT_POINT "/temp"

// #define FILE_PATH_FROM_ID(name) MOUNT_POINT "/" name

#define ALARM_FILE_TYPE ".txt"
#define RINGTONE_FILE_TYPE ".wav"

#define GPIO_OUTPUT_OE    GPIO_NUM_3
#define GPIO_OUTPUT_PIN_SEL  (1ULL<<GPIO_OUTPUT_OE)

#define GPIO_INPUT_IO_0     GPIO_NUM_12
#define GPIO_INPUT_IO_1     GPIO_NUM_13
#define GPIO_INPUT_IO_2     GPIO_NUM_14
#define GPIO_INPUT_PIN_SEL  ((1ULL<<GPIO_INPUT_IO_0) | (1ULL<<GPIO_INPUT_IO_1) | (1ULL<<GPIO_INPUT_IO_2))

#define BUTTON_LEFT      0
#define BUTTON_CENTER    1
#define BUTTON_RIGHT     2

#define ESP_INTR_FLAG_DEFAULT ESP_INTR_FLAG_EDGE

#include <stdbool.h>
#include <stdint.h>
#include <sys/time.h>
#include <time.h>

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
    int64_t stamp;
}gpio_sm_t;

typedef enum {
    EDGE,
    ALARM
}button_queue_msg_mode_t;

typedef struct
{
    button_queue_msg_mode_t mode;
    uint32_t gpio_num;
}button_queue_msg_t;



#endif