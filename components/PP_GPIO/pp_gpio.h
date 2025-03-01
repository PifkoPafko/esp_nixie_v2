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
#include "esp_log.h"
#include "driver/gpio.h"

/* Macros */
#define GPIO_TAG "GPIO"

#define GPIO_OUTPUT_OE          GPIO_NUM_3
#define GPIO_OUTPUT_RED         GPIO_NUM_47
#define GPIO_OUTPUT_BLUE        GPIO_NUM_21
#define GPIO_OUTPUT_GREEN       GPIO_NUM_48
#define GPIO_OUTPUT_PIN_SEL     ((1ULL<<GPIO_OUTPUT_OE) | (1ULL<<GPIO_OUTPUT_RED) | (1ULL<<GPIO_OUTPUT_BLUE) | (1ULL<<GPIO_OUTPUT_GREEN))

#define GPIO_INPUT_IO_0     GPIO_NUM_12
#define GPIO_INPUT_IO_1     GPIO_NUM_13
#define GPIO_INPUT_IO_2     GPIO_NUM_14
#define GPIO_INPUT_PIN_SEL  ((1ULL<<GPIO_INPUT_IO_0) | (1ULL<<GPIO_INPUT_IO_1) | (1ULL<<GPIO_INPUT_IO_2))

/* Functions */

/** @brief pp_gpio_init: Initializes and sets gpio configuration for OE line, red, green and blue leds.
 *
 * @return
 */
void pp_gpio_init(void);

#endif