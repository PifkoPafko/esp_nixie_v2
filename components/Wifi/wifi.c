#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "ObjectTransfer_metadata_write.h"

#include "pp_rtc.h"
#include "esp_sntp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wifi.h"

static const char* TAG = "WIFI";

static TaskFunction_t wifi_search_main_fun;
static TaskHandle_t wifi_main_hdl;

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static my_wifi_t my_wifi;
bool isConnected = false;

static void pp_sntp_init( char * sntp_srv );

void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) 
    {
        isConnected = false;
        ObjectTransfer_send_simple_wifi_ind(3);
        ESP_LOGI(TAG,"connect to the AP fail");
    } 
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) 
    {
        isConnected = true;
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        ObjectTransfer_send_simple_wifi_ind(2);
        pp_sntp_init(NULL);
    }
}

void search_wifi_task(void* arg)
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
        ObjectTransfer_send_found_wifi_ind(&record[i]);
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    ObjectTransfer_send_simple_wifi_ind(0);

    ESP_LOGI(TAG, "Wifi search task end");
    vTaskDelete(wifi_main_hdl);
}

esp_err_t start_search_task()
{
    wifi_search_main_fun = search_wifi_task;
    BaseType_t res = xTaskCreate(wifi_search_main_fun, "Wifi_Search", 4096, NULL, 1, &wifi_main_hdl);
    if(res != pdPASS)
    {
        ESP_ERROR_CHECK(ESP_FAIL);
        return res;
    }

    return ESP_OK;
}

esp_err_t connect_wifi(const uint8_t *ssid, const uint8_t ssid_len, const uint8_t *password, const uint8_t pass_len)
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

void wifi_sta_init()
{
    esp_wifi_get_config(WIFI_IF_STA, &my_wifi.wifi_config);
    my_wifi.my_ssid_len = strlen((char*)my_wifi.wifi_config.sta.ssid);
    my_wifi.my_password_len = strlen((char*)my_wifi.wifi_config.sta.password);

    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_connect());
}

bool get_wifi_connect_status()
{
    return isConnected;
}

my_wifi_t* get_current_wifi()
{
    return &my_wifi;
}

void sntp_cb(struct timeval *tv)
{
    ESP_LOGI(TAG, "SNTP Syncro Done, time: %lld", tv->tv_sec);

    time_t now;
    struct tm timeinfo;

    time(&now);
    localtime_r(&now, &timeinfo);

    ESP_LOGI(TAG, "New time: %02d:%02d:%02d, %02d.%02d.%04d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec, timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900);

    pp_rtc_set_time(timeinfo.tm_sec, timeinfo.tm_min, timeinfo.tm_hour, timeinfo.tm_wday + 1, timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year - 100);
}

static void pp_sntp_init( char * sntp_srv ) {

	esp_sntp_stop();

    ESP_LOGI(TAG, "Initializing SNTP");
    esp_sntp_setoperatingmode( SNTP_OPMODE_POLL );
    esp_sntp_set_time_sync_notification_cb(sntp_cb);

    if( sntp_srv ) esp_sntp_setservername(0, sntp_srv);
    else esp_sntp_setservername(0, DEFAULT_NTP_SERVER);

    esp_sntp_init();
}
