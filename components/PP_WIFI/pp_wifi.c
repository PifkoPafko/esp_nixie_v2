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

#include "pp_wifi.h"

#define TAG = "WIFI"

static TaskFunction_t wifi_search_main_fun;

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static my_wifi_t my_wifi;
bool isConnected = false;

static void pp_sntp_init( char * sntp_srv );

static esp_err_t pp_wifi_init()
{
    esp_err_t ret = esp_netif_init();
    if (ret){
        ESP_LOGE(MAIN_TAG, "esp_netif_init, error code = %x", ret);
        return ret;
    }

    esp_event_loop_create_default();
    if (ret){
        ESP_LOGE(MAIN_TAG, "esp_event_loop_create_default, error code = %x", ret);
        return ret;
    }

    esp_netif_create_default_wifi_sta();
    if (ret){
        ESP_LOGE(MAIN_TAG, "esp_netif_create_default_wifi_sta, error code = %x", ret);
        return ret;
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    if (ret){
        ESP_LOGE(MAIN_TAG, "esp_wifi_init, error code = %x", ret);
        return ret;
    }

    esp_wifi_set_mode(WIFI_MODE_STA);
    if (ret){
        ESP_LOGE(MAIN_TAG, "esp_wifi_set_mode, error code = %x", ret);
        return ret;
    }

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &pp_wifi_event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &pp_wifi_event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    pp_wifi_sta_init();

    return ESP_OK;
}

void pp_wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) 
    {
        // TODO: gpio_set_level(GPIO_OUTPUT_GREEN, 0);
        isConnected = false;
        pp_object_transfer_send_simple_wifi_ind(3);
        ESP_LOGI(TAG,"connect to the AP fail");
    } 
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) 
    {
        // TODO: gpio_set_level(GPIO_OUTPUT_GREEN, 1);
        isConnected = true;
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        pp_object_transfer_send_simple_wifi_ind(2);
        pp_sntp_init(NULL);
    }
}

void pp_search_wifi_task(void* arg)
{
    ESP_ERROR_CHECK(esp_wifi_scan_start(NULL, true));

    uint16_t record_num = 20;
    wifi_ap_record_t record[20];

    esp_wifi_scan_get_ap_records(&record_num, record);

    for (int i = 0; i < record_num; i++)
    {
        ESP_LOGI(TAG, "Found Wi-Fi: %s", (char*)record[i].ssid);
    }

    for(uint8_t i=0; i<record_num; i++)
    {
        pp_object_transfer_send_found_wifi_ind(&record[i]);
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    pp_object_transfer_send_simple_wifi_ind(0);

    ESP_LOGI(TAG, "Wifi search task end");
    vTaskDelete(wifi_main_h);
}

esp_err_t pp_start_search_task()
{
    wifi_search_main_fun = pp_search_wifi_task;
    BaseType_t res = xTaskCreate(wifi_search_main_fun, "Wifi_Search", 4096, NULL, 1, &wifi_main_h);
    if(res != pdPASS)
    {
        ESP_ERROR_CHECK(ESP_FAIL);
        return res;
    }

    return ESP_OK;
}

esp_err_t pp_connect_wifi(const uint8_t *ssid, const uint8_t ssid_len, const uint8_t *password, const uint8_t pass_len)
{
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

    return ESP_OK;
}

void pp_wifi_sta_init()
{
    esp_wifi_get_config(WIFI_IF_STA, &my_wifi.wifi_config);
    my_wifi.my_ssid_len = strlen((char*)my_wifi.wifi_config.sta.ssid);
    my_wifi.my_password_len = strlen((char*)my_wifi.wifi_config.sta.password);

    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_connect());
}

bool pp_get_wifi_connect_status()
{
    return isConnected;
}

my_wifi_t* pp_get_current_wifi()
{
    return &my_wifi;
}

void pp_sntp_cb(struct timeval *tv)
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

static void pp_sntp_init( char * sntp_srv ) {

	esp_sntp_stop();

    ESP_LOGI(TAG, "Initializing SNTP");
    esp_sntp_setoperatingmode( SNTP_OPMODE_POLL );
    esp_sntp_set_time_sync_notification_cb(pp_sntp_cb);

    if( sntp_srv ) esp_sntp_setservername(0, sntp_srv);
    else 
    {
        esp_sntp_setservername(0, DEFAULT_NTP_SERVER_0);
        esp_sntp_setservername(1, DEFAULT_NTP_SERVER_1);
        esp_sntp_setservername(2, DEFAULT_NTP_SERVER_2);
        esp_sntp_setservername(3, DEFAULT_NTP_SERVER_3);
        esp_sntp_setservername(4, DEFAULT_NTP_SERVER_4);
        esp_sntp_setservername(5, DEFAULT_NTP_SERVER_5);
    }

    esp_sntp_init();
}
