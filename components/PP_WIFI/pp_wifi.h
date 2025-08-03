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

#ifndef __WIFI_H__
#define __WIFI_H__

/* Headers */
// #include "esp_log.h"
// #include "esp_event.h"
// #include "esp_wifi.h"

// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "freertos/event_groups.h"
// #include "esp_wifi.h"
// #include "esp_log.h"
// #include "pp_object_transfer_metadata_write.h"

// #include "pp_rtc.h"
// #include "pp_alarm.h"
// #include "esp_sntp.h"
// #include "driver/gpio.h"

// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>

/* Macros */
#define DEFAULT_NTP_SERVER_0			"0.pl.pool.ntp.org"
#define DEFAULT_NTP_SERVER_1			"1.pl.pool.ntp.org"
#define DEFAULT_NTP_SERVER_2			"2.pl.pool.ntp.org"
#define DEFAULT_NTP_SERVER_3			"3.pl.pool.ntp.org"
#define DEFAULT_NTP_SERVER_4			"ntp1.tp.pl"
#define DEFAULT_NTP_SERVER_5			"ntp.certum.pl"

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

/* Structures */
typedef struct{
    uint8_t my_ssid_len;
    uint8_t my_password_len;
    wifi_config_t wifi_config;
} my_wifi_t;

typedef enum{
    WIFI_SEARCH_END,
    WIFI_RESERVED,
    WIFI_CONNECTED,
    WIFI_DISCONNECTED
} my_wifi_status_t;

/* Functions */

/** @brief pp_wifi_init: Wifi features initialization function.
 *
 * @return
 */
void pp_wifi_init(void);

/** @brief pp_start_search_task: Creates wifi searching task
 * 
 * @return
 */
void pp_start_search_task(void)

/** @brief pp_connect_wifi: Connects to specified wifi network
 *
 * @param[in]   ssid        (const uint8_t*) Wifi SSID
 * @param[in]   ssid_len    (const uint8_t) Wifi SSID length
 * @param[in]   password    (const uint8_t*) Wifi password
 * @param[in]   pass_len    (const uint8_t) Wifi password length
 * 
 * @return
 */
void pp_connect_wifi(const uint8_t *ssid, const uint8_t ssid_len, const uint8_t *password, const uint8_t pass_len);

/** @brief pp_get_wifi_connect_status: Returns WiFi connection status
 * 
 * @return  (bool) WiFi connection status
 */
bool pp_get_wifi_connect_status();

/** @brief pp_get_current_wifi: Returns WiFi descriptions structure pointer
 * 
 * @return  (my_wifi_t*) WiFi descriptions structure pointer
 */
my_wifi_t* pp_get_current_wifi();

#endif