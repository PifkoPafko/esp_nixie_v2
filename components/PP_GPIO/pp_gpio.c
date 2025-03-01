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
#include "pp_gpio.h"

/* Functions */

/** @brief gpio_init: Initializes and sets gpio configuration for OE line, red, green and blue leds.
 *
 * @return
 */
void gpio_init(void)
{
    ESP_LOGI(GPIO_TAG, "Initializing gpio");
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_config(&io_conf));
    ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_set_level(GPIO_OUTPUT_OE, 0));
    ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_set_level(GPIO_OUTPUT_RED, 0));
    ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_set_level(GPIO_OUTPUT_BLUE, 0));
    ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_set_level(GPIO_OUTPUT_GREEN, 0));
}