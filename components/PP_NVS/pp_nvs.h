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

#ifndef __NVS_H__
#define __NVS_H__

/* Headers */
#include <string.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "pp_global.h"

/* Functions */

/** @brief nvs_init: Initializes nvs flash memory.
 *
 * @return
 */
void pp_nvs_init(void);

/** @brief pp_nvs_led_store: Stores current LEDs settings into NVS.
 *
 * @return
 */
void pp_nvs_led_store(void);

#endif