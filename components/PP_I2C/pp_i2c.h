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

#ifndef __PP_I2C_H__
#define __PP_I2C_H__

/* Headers */
#include <stdint.h>
#include <stddef.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/i2c_master.h"


/* Macros */
#define I2C_MASTER_NUM  0

#define I2C_SDA_IO      GPIO_NUM_8
#define I2C_SCL_IO      GPIO_NUM_18

#define I2C_FREQ        100000
#define I2C_FREQ_MAX    1000000
#define I2C_FREQ_MIN    10000

#define I2CDEV_TIMEOUT	1000
#define MUTEX_ON		1

/* Functions */

/** @brief pp_i2c_init: Initializes I2C
 *
 * @return
 */
void pp_i2c_init(void);

#endif
