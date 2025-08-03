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

#include "pp_global.h"

/* Global Variables */

/* Tasks Handles */
TaskHandle_t display_main_h;
TaskHandle_t button_main_h;
TaskHandle_t rtc_main_h;
TaskHandle_t alarm_main_h;
TaskHandle_t wifi_main_h;

/* Device Mode */
device_mode_t device_mode = DEFAULT_MODE;

/* Current Alarm */
alarm_mode_args_t current_alarm;

/* Custom UUIDS */
const uint8_t alarm_type_uuid[ESP_UUID_LEN_128] = {0x02, 0x00, 0x12, 0xAC, 0x42, 0x02, 0x61, 0xA2, 0xED, 0x11, 0xBA, 0x29, 0xB8, 0x13, 0x08, 0xCC};
const uint8_t ringtone_type_uuid[ESP_UUID_LEN_128] = {0x03, 0x00, 0x12, 0xAC, 0x42, 0x02, 0x61, 0xA2, 0xED, 0x11, 0xBA, 0x29, 0xB8, 0x13, 0x08, 0xCC};

/* Button action queue handle */
QueueHandle_t button_action_queue;

/* Display digits description */
display_digits_t display_digits;