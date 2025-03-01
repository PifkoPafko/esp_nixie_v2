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

#ifndef __SD_CARD_H__
#define __SD_CARD_H__

/* Headers */
#include "driver/sdmmc_host.h"
#include "esp_vfs_fat.h"

/* Macros */
#define SDCARD_TAG "SD CARD"

#define MOUNT_POINT "/sdcard"
#define FILE_LIST_NAME MOUNT_POINT "/file_id_list.txt"
#define ALARMS_PATH MOUNT_POINT "/alarms"
#define RIGNTONES_PATH MOUNT_POINT "/ringtones"
#define TEMP_FILE_PATH MOUNT_POINT "/temp"

// #define FORMAT_SD

/* Functions */

/** @brief sd_card_init: Initializes sd card and fat file system on it.
 *
 * @return
 */
void sd_card_init(void);

#endif