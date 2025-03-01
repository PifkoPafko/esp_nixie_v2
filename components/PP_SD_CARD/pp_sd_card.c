/****************************************************************************
 * Copyright (C) 2012 by Matteo Franchin                                    *
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
#include "pp_sd_card.h"

/****************************************************************************/
/* Functions */

/** @brief sd_card_init: Initializes sd card and fat file system on it.
 *
 * @param[in]  param1_name  N/A
 * @param[out]  param2_name  N/A
 * @return N/A
 */
void sd_card_init(void)
{
    ESP_LOGI(SDCARD_TAG, "Initializing sd card");
    esp_vfs_fat_sdmmc_mount_config_t mount_config = 
    {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };

    sdmmc_card_t *card;
    const char mount_point[] = MOUNT_POINT;
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    host.max_freq_khz = 10000;          // TODO: Change in future to higher value
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();

    slot_config.width = 4;
    slot_config.cmd = 16;
    slot_config.clk = 15;
    slot_config.d0 = 17;
    slot_config.d1 = 5;
    slot_config.d2 = 6;
    slot_config.d3 = 7;
    slot_config.cd = 4;
    slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

#ifdef FORMAT_SD
    ESP_LOGI(SDCARD_TAG, "Formating sd card");
    ESP_ERROR_CHECK(esp_vfs_fat_sdcard_format(mount_point, card));
#endif

    ESP_LOGI(SDCARD_TAG, "Mounting filesystem");
    ESP_ERROR_CHECK(esp_vfs_fat_sdmmc_mount(mount_point, &host, &slot_config, &mount_config, &card));
}