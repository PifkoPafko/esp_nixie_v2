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
#include "pp_i2c.h"

/* Macros */
#define I2C_TAG "I2C"

/* Functions */

/** @brief pp_i2c_init: Initializes I2C
 *
 * @return
 */
void pp_i2c_init(void) 
{
    ESP_LOGI(I2C_TAG, "Initializing I2C");
    uint32_t freq = I2C_FREQ;
	if(freq < I2C_FREQ_MIN) freq = I2C_FREQ_MIN;
	if(freq > I2C_FREQ_MAX) freq = I2C_FREQ_MAX;

    i2c_master_bus_config_t i2c_bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = 0,
        .scl_io_num = I2C_SCL_IO,
        .sda_io_num = I2C_SDA_IO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    i2c_master_bus_handle_t bus_handle;
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_config, &bus_handle));
}