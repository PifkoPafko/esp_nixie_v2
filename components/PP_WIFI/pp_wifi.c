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
#include "pp_wifi.h"

 /* Declarations */
static void pp_wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
static void pp_search_wifi_task(void* arg);
static void pp_sntp_cb(struct timeval *tv);

/* Macros */
#define TAG         "WIFI"
#define SETUP_DEFAULT_WIFI

#define WIFI_NAME   "Orange_Swiatlowod_5AF0"
#define WIFI_PASS   "maQD9qJkU44Fv3gVqS"

/* Variables */
static TaskFunction_t wifi_search_main_fun;
static my_wifi_t my_wifi;
static bool isConnected = false;

/* Functions */

/** @brief pp_wifi_init: Wifi features initialization function.
 *
 * @return
 */
void pp_wifi_init(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &pp_wifi_event_handler, NULL, &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &pp_wifi_event_handler, NULL, &instance_got_ip));

    esp_wifi_get_config(WIFI_IF_STA, &my_wifi.wifi_config);
    my_wifi.my_ssid_len = strlen((char*)my_wifi.wifi_config.sta.ssid);
    my_wifi.my_password_len = strlen((char*)my_wifi.wifi_config.sta.password);

    #ifdef SETUP_DEFAULT_WIFI
    strcpy((char*)my_wifi.wifi_config.sta.ssid, WIFI_NAME);
    strcpy((char*)my_wifi.wifi_config.sta.password, WIFI_PASS);
    my_wifi.my_ssid_len = strlen(WIFI_NAME);
    my_wifi.my_password_len = strlen(WIFI_PASS);
    esp_wifi_set_config(WIFI_IF_STA, &my_wifi.wifi_config);
    #endif

    



    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_connect());

    return;
}

/** @brief pp_wifi_event_handler: wifi event handler function
 *
 * @param[in]   arg         (void*) User data (unused)
 * @param[in]   event_base  (esp_event_base_t) Wifi event type
 * @param[in]   event_id    (int32_t) Event action type
 * @param[in]   event_data  (void*) Event data
 * 
 * @return
 */
static void pp_wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) 
    {
        gpio_set_level(GPIO_OUTPUT_GREEN, 0);
        isConnected = false;
        pp_object_transfer_send_simple_wifi_ind((uint8_t)WIFI_DISCONNECTED);
        ESP_LOGI(TAG,"connect to the AP fail");
    } 
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) 
    {
        gpio_set_level(GPIO_OUTPUT_GREEN, 1);
        isConnected = true;
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        pp_object_transfer_send_simple_wifi_ind((uint8_t)WIFI_CONNECTED);

        esp_sntp_stop();

        ESP_LOGI(TAG, "Initializing SNTP");
        esp_sntp_setoperatingmode( SNTP_OPMODE_POLL );
        esp_sntp_set_time_sync_notification_cb(pp_sntp_cb);

        esp_sntp_setservername(0, DEFAULT_NTP_SERVER_0);
        esp_sntp_setservername(1, DEFAULT_NTP_SERVER_1);
        esp_sntp_setservername(2, DEFAULT_NTP_SERVER_2);
        esp_sntp_setservername(3, DEFAULT_NTP_SERVER_3);
        esp_sntp_setservername(4, DEFAULT_NTP_SERVER_4);
        esp_sntp_setservername(5, DEFAULT_NTP_SERVER_5);

        esp_sntp_init();
    }
}

/** @brief pp_search_wifi_task: wifi search networks function
 *
 * @param[in]   arg         (void*) User data (unused)
 * 
 * @return
 */
static void pp_search_wifi_task(void* arg)
{
    ESP_ERROR_CHECK(esp_wifi_scan_start(NULL, true));

    uint16_t record_num = WIFI_RECORD_NUM;
    wifi_ap_record_t record[WIFI_RECORD_NUM];

    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&record_num, record));

    for (uint8_t i = 0; i < WIFI_RECORD_NUM; ++i)
    {
        ESP_LOGI(TAG, "Found Wi-Fi: %s", (char*)record[i].ssid);
    }

    for(uint8_t i = 0; i < WIFI_RECORD_NUM; ++i)
    {
        pp_object_transfer_send_found_wifi_ind(&record[i]);
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    pp_object_transfer_send_simple_wifi_ind((uint8_t)WIFI_SEARCH_END);

    ESP_LOGI(TAG, "Wifi search task end");
    vTaskDelete(wifi_main_h);
}

/** @brief pp_start_search_task: Creates wifi searching task
 * 
 * @return
 */
void pp_start_search_task(void)
{
#ifdef WIFI_ENABLE
    wifi_search_main_fun = pp_search_wifi_task;
    BaseType_t res = xTaskCreate(wifi_search_main_fun, "Wifi_Search", 4096, NULL, 1, &wifi_main_h);
    if(res != pdPASS)
    {
        ESP_ERROR_CHECK(ESP_FAIL);
    }
#endif
}

/** @brief pp_connect_wifi: Connects to specified wifi network
 *
 * @param[in]   ssid        (const uint8_t*) Wifi SSID
 * @param[in]   ssid_len    (const uint8_t) Wifi SSID length
 * @param[in]   password    (const uint8_t*) Wifi password
 * @param[in]   pass_len    (const uint8_t) Wifi password length
 * 
 * @return
 */
void pp_connect_wifi(const uint8_t *ssid, const uint8_t ssid_len, const uint8_t *password, const uint8_t pass_len)
{
#ifdef WIFI_ENABLE
    ESP_ERROR_CHECK(esp_wifi_stop());
    memcpy((uint8_t*)my_wifi.wifi_config.sta.ssid, ssid, ssid_len);
    my_wifi.wifi_config.sta.ssid[ssid_len] = '\0';
    my_wifi.my_ssid_len = ssid_len;

    memcpy((uint8_t*)my_wifi.wifi_config.sta.password, password, pass_len);
    my_wifi.wifi_config.sta.password[pass_len] = '\0';
    my_wifi.my_password_len = pass_len;

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &my_wifi.wifi_config) );    
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_connect());
#endif
}

/** @brief pp_get_wifi_connect_status: Returns WiFi connection status
 * 
 * @return  (bool) WiFi connection status
 */
bool pp_get_wifi_connect_status()
{
    return isConnected;
}

/** @brief pp_get_current_wifi: Returns WiFi descriptions structure pointer
 * 
 * @return  (my_wifi_t*) WiFi descriptions structure pointer
 */
my_wifi_t* pp_get_current_wifi()
{
    return &my_wifi;
}

/** @brief pp_sntp_cb: Server SNTP feature callback function
 * 
 * @param[in]   tv  (struct timeval*) Obtained timestamp
 * 
 * @return
 */
static void pp_sntp_cb(struct timeval *tv)
{
    ESP_LOGI(TAG, "SNTP Syncro Done, time: %lld", tv->tv_sec);

    time_t now;
    struct tm timeinfo;

    time(&now);
    localtime_r(&now, &timeinfo);

    ESP_LOGI(TAG, "New time: %02d:%02d:%02d, %02d.%02d.%04d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec, timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900);

    pp_rtc_set_time(&timeinfo);
    pp_set_next_alarm();
}
