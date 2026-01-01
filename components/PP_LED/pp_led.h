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

#include <stdio.h>
#include <string.h>

#include "esp_err.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/ledc.h"

#include "pp_global.h"


/* Macros */
#define LEDC_LS_MODE           LEDC_LOW_SPEED_MODE

/* First LED strip */
#define LEDC_LS_TIMER_CH0           LEDC_TIMER_1
#define LEDC_LS_CH0_RED_GPIO        (35)
#define LEDC_LS_CH0_RED_CHANNEL     LEDC_CHANNEL_0
#define LEDC_LS_CH0_GREEN_GPIO      (36)
#define LEDC_LS_CH0_GREEN_CHANNEL   LEDC_CHANNEL_1
#define LEDC_LS_CH0_BLUE_GPIO       (37)
#define LEDC_LS_CH0_BLUE_CHANNEL    LEDC_CHANNEL_2

/* Second LED strip */
#define LEDC_LS_TIMER_CH1           LEDC_TIMER_2
#define LEDC_LS_CH1_RED_GPIO        (38)
#define LEDC_LS_CH1_RED_CHANNEL     LEDC_CHANNEL_3
#define LEDC_LS_CH1_GREEN_GPIO      (39)
#define LEDC_LS_CH1_GREEN_CHANNEL   LEDC_CHANNEL_4
#define LEDC_LS_CH1_BLUE_GPIO       (40)
#define LEDC_LS_CH1_BLUE_CHANNEL    LEDC_CHANNEL_5

#define LEDC_SET_NUM                (2)
#define LEDC_CH_NUM                 (3)

#define LED_RED_INDEX               0
#define LED_GREEN_INDEX             1
#define LED_BLUE_INDEX              2

/* Functions */

/** @brief pp_led_init: Initializes LED
 *
 * @return
 */
void pp_led_init(void);