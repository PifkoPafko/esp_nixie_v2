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
#include "pp_nvs.h"

/* Macros */
#define NVS_TAG "NVS"

#define LED_KEYS_COUNT 4

/* Constant values */
const char* nvs_keys_led_0[LED_KEYS_COUNT] = {
    "LED0_ENABLE_KEY",
    "LED0_RED_KEY",
    "LED0_GREEN_KEY",
    "LED0_BLUE_KEY",
};

const char* nvs_keys_led_1[LED_KEYS_COUNT] = {
    "LED1_ENABLE_KEY",
    "LED1_RED_KEY",
    "LED1_GREEN_KEY",
    "LED1_BLUE_KEY"
};

/* Variable */
static nvs_handle_t my_handle;

/* Functions */

/** @brief nvs_init: Initializes nvs flash memory.
 *
 * @return
 */
void pp_nvs_init(void)
{
    ESP_LOGI(NVS_TAG, "Initializing nvs");
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) 
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    ESP_ERROR_CHECK(nvs_open("storage", NVS_READWRITE, &my_handle));

    uint32_t read_nvs[LED_KEYS_COUNT];
    bool to_commit = false;

    for (uint8_t i = 0; i < LED_KEYS_COUNT; ++i)
    {
        ret = nvs_get_u32(my_handle, nvs_keys_led_0[i], &read_nvs[i]);

        if (ret != ESP_OK)
        {
            read_nvs[i] = 0;
            ESP_ERROR_CHECK(nvs_set_u32(my_handle, nvs_keys_led_0[i], read_nvs[i]));
            to_commit = true;
        }
    }

    current_led[0].enable = (bool)read_nvs[0];
    memcpy(&current_led[0].red, &read_nvs[1], sizeof(read_nvs[0]) * 3);

    for (uint8_t i = 0; i < LED_KEYS_COUNT; ++i)
    {
        ret = nvs_get_u32(my_handle, nvs_keys_led_1[i], &read_nvs[i]);

        if (ret != ESP_OK)
        {
            read_nvs[i] = 0;
            ESP_ERROR_CHECK(nvs_set_u32(my_handle, nvs_keys_led_1[i], read_nvs[i]));
            to_commit = true;
        }
    }

    current_led[1].enable = (bool)read_nvs[0];
    memcpy(&current_led[1].red, &read_nvs[1], sizeof(read_nvs[0]) * 3);

    if(to_commit)
    {
        ESP_ERROR_CHECK(nvs_commit(my_handle));
    }
}

/** @brief pp_nvs_led_store: Stores current LEDs settings into NVS.
 *
 * @return
 */
void pp_nvs_led_store(uint8_t channel_bitmask)
{
    uint32_t write_nvs[LED_KEYS_COUNT];

    if (channel_bitmask & 0x1)
    {
        ESP_LOGI(NVS_TAG, "Storing channel 0");
        write_nvs[0] = current_led[0].enable ? 1 : 0;
        memcpy(&write_nvs[1], &current_led[0].red, sizeof(write_nvs[0]) * 3);

        for (uint8_t i = 0; i < LED_KEYS_COUNT; ++i)
        {
            ESP_ERROR_CHECK(nvs_set_u32(my_handle, nvs_keys_led_0[i], write_nvs[i]));
        }
    }

    if (channel_bitmask & 0x2)
    {
        ESP_LOGI(NVS_TAG, "Storing channel 1");
        write_nvs[0] = current_led[1].enable ? 1 : 0;
        memcpy(&write_nvs[1], &current_led[1].red, sizeof(write_nvs[0]) * 3);

        for (uint8_t i = 0; i < LED_KEYS_COUNT; ++i)
        {
            ESP_ERROR_CHECK(nvs_set_u32(my_handle, nvs_keys_led_1[i], write_nvs[i]));
        }
    }

    ESP_ERROR_CHECK(nvs_commit(my_handle));
}

