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

#include "esp_log.h"
#include "esp_event.h"
#include "esp_wifi.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "pp_object_transfer_metadata_write.h"

#include "pp_rtc.h"
#include "pp_alarm.h"
#include "esp_sntp.h"
#include "driver/gpio.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define DEFAULT_NTP_SERVER_0			"0.pl.pool.ntp.org"
#define DEFAULT_NTP_SERVER_1			"1.pl.pool.ntp.org"
#define DEFAULT_NTP_SERVER_2			"2.pl.pool.ntp.org"
#define DEFAULT_NTP_SERVER_3			"3.pl.pool.ntp.org"
#define DEFAULT_NTP_SERVER_4			"ntp1.tp.pl"
#define DEFAULT_NTP_SERVER_5			"ntp.certum.pl"

typedef struct{
    uint8_t my_ssid_len;
    uint8_t my_password_len;
    wifi_config_t wifi_config;
} my_wifi_t;

esp_err_t pp_start_search_task();
void pp_wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
esp_err_t pp_connect_wifi(const uint8_t *ssid, const uint8_t ssid_len, const uint8_t *password, const uint8_t pass_len);
void pp_wifi_sta_init();
bool pp_get_wifi_connect_status();
my_wifi_t* pp_get_current_wifi();

#endif