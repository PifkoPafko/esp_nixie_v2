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

#ifndef __GPIO_H__
#define __GPIO_H__

/* Headers */
#include <stdint.h>
#include <stdbool.h>

#include "esp_log.h"

#include "freertos/FreeRTOS.h"

#include "driver/gpio.h"

#include "pp_global.h"

/* Macros */
// #define ENABLE_GPIO_DEBUG_LOGS   // Enable to show state transition logs of buttons

#define GPIO_OUTPUT_OE          GPIO_NUM_3
#define GPIO_OUTPUT_RED         GPIO_NUM_47
#define GPIO_OUTPUT_BLUE        GPIO_NUM_21
#define GPIO_OUTPUT_GREEN       GPIO_NUM_48
#define GPIO_OUTPUT_PIN_SEL     ((1ULL<<GPIO_OUTPUT_OE) | (1ULL<<GPIO_OUTPUT_RED) | (1ULL<<GPIO_OUTPUT_BLUE) | (1ULL<<GPIO_OUTPUT_GREEN))

#define GPIO_INPUT_IO_LEFT      GPIO_NUM_12
#define GPIO_INPUT_IO_CENTER    GPIO_NUM_13
#define GPIO_INPUT_IO_RIGHT     GPIO_NUM_14
#define GPIO_INPUT_PIN_SEL      ((1ULL<<GPIO_INPUT_IO_LEFT) | (1ULL<<GPIO_INPUT_IO_CENTER) | (1ULL<<GPIO_INPUT_IO_RIGHT))

/* STRUCTURES */
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

typedef enum 
{
    ISR,
    TIMER
}button_queue_msg_type_t;

typedef struct
{
    button_queue_msg_type_t type;
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

/* Functions */

/** @brief pp_gpio_init: Initializes and sets gpio configuration for OE line, red, green and blue leds.
 *
 * @return
 */
void pp_gpio_init(void);

/** @brief pp_led_enable: Enables or disables chosen LED
 *
 * @param[in]   led (uint8_t) LED to enable/disable
 * @param[in]   enable (uint8_t) true if enable, false if disable
 * 
 * @return
 */
void pp_led_enable(uint8_t led, bool enable);

#endif