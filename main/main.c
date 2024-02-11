/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

/****************************************************************************
*
* This demo showcases creating a GATT database using a predefined attribute table.
* It acts as a GATT server and can send adv data, be connected by client.
* Run the gatt_client demo, the client demo will automatically connect to the gatt_server_service_table demo.
* Client demo will enable GATT server's notify after connection. The two devices will then exchange
* data.
*
****************************************************************************/
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"

#include <inttypes.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "ObjectTransfer_gatt_server.h"
#include "ObjectManager.h"
#include "project_defs.h"
#include "pp_wave_player.h"

#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_gatt_common_api.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"
#include "driver/gpio.h"
#include "driver/gptimer.h"
#include <sys/time.h>
#include <time.h>

#include "esp_wifi.h"
#include "wifi.h"

#include "mk_i2c.h"
#include "pp_rtc.h"
#include "pp_nixie_display.h"

#define MAIN_TAG    "MAIN"
#define ESP_APP_ID  0x55

static void button_functions(button_action_t action_handler);

static esp_err_t bt_init()
{
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    esp_err_t ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        ESP_LOGE(MAIN_TAG, "%s enable controller failed: %s", __func__, esp_err_to_name(ret));
        return ret;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret) {
        ESP_LOGE(MAIN_TAG, "%s enable controller failed: %s", __func__, esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(MAIN_TAG, "%s init bluetooth", __func__);
    ret = esp_bluedroid_init();
    if (ret) {
        ESP_LOGE(MAIN_TAG, "%s init bluetooth failed: %s", __func__, esp_err_to_name(ret));
        return ret;
    }

    ret = esp_bluedroid_enable();
    if (ret) {
        ESP_LOGE(MAIN_TAG, "%s enable bluetooth failed: %s", __func__, esp_err_to_name(ret));
        return ret;
    }

    ret = esp_ble_gatts_register_callback(gatts_event_handler);
    if (ret){
        ESP_LOGE(MAIN_TAG, "gatts register error, error code = %x", ret);
        return ret;
    }

    ret = esp_ble_gap_register_callback(gap_event_handler);
    if (ret){
        ESP_LOGE(MAIN_TAG, "gap register error, error code = %x", ret);
        return ret;
    }

    ret = esp_ble_gatts_app_register(ESP_APP_ID);
    if (ret){
        ESP_LOGE(MAIN_TAG, "gatts app register error, error code = %x", ret);
        return ret;
    }

    ret = esp_ble_gatt_set_local_mtu(512);
    if (ret){
        ESP_LOGE(MAIN_TAG, "set local  MTU failed, error code = %x", ret);
        return ret;
    }

    /* set the security iocap & auth_req & key size & init key response key parameters to the stack*/
    esp_ble_auth_req_t auth_req = ESP_LE_AUTH_REQ_SC_MITM_BOND;     //bonding with peer device after authentication
    esp_ble_io_cap_t iocap = ESP_IO_CAP_OUT;           //set the IO capability to No output No input
    uint8_t key_size = 16;      //the key size should be 7~16 bytes
    uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    uint8_t rsp_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    //set static passkey

    // uint32_t passkey = esp_random() / 4832 + 100000;    // /4295 to convert uint32 value to 0-999999 value
    uint8_t auth_option = ESP_BLE_ONLY_ACCEPT_SPECIFIED_AUTH_DISABLE;
    uint8_t oob_support = ESP_BLE_OOB_DISABLE;
    // esp_ble_gap_set_security_param(ESP_BLE_SM_SET_STATIC_PASSKEY, &passkey, sizeof(uint32_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth_req, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_ONLY_ACCEPT_SPECIFIED_SEC_AUTH, &auth_option, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_OOB_SUPPORT, &oob_support, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &init_key, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &rsp_key, sizeof(uint8_t));

    /* Just show how to clear all the bonded devices
     * Delay 30s, clear all the bonded devices
     *
     * vTaskDelay(30000 / portTICK_PERIOD_MS);
     * remove_all_bonded_devices();
     */

    return ESP_OK;
}

static esp_err_t sd_cad_init()
{
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
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

    ESP_LOGI(MAIN_TAG, "Mounting filesystem");
    esp_err_t ret = esp_vfs_fat_sdmmc_mount(mount_point, &host, &slot_config, &mount_config, &card);

    // ret = esp_vfs_fat_sdcard_format(mount_point, card);     //TODO remember to delete this

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(MAIN_TAG, "Failed to mount filesystem.");
        } else {
            ESP_LOGE(MAIN_TAG, "Failed to initialize the card (%s).", esp_err_to_name(ret));
        }
        return ret;
    }
    ESP_LOGI(MAIN_TAG, "Filesystem mounted");

    return ret;
}

static esp_err_t wifi_init()
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
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_sta_init();

    return ESP_OK;
}

static QueueHandle_t gpio_evt_queue = NULL;
gpio_sm_t button_sm[3];
TimerHandle_t btn_timer_h[3];
pairing_sm_t pairing_sm = WAIT_FOR_LEFT;
device_mode_t device_mode = DEFAULT_MODE;

static void IRAM_ATTR button_isr_handler(void* arg)
{
    button_queue_msg_t msg;
    msg.enable = gpio_get_level((uint32_t) arg);;
    msg.gpio_num = (uint32_t) arg;
    
    xQueueSendFromISR(gpio_evt_queue, &msg, NULL);
}

time_change_sm_t time_change_sm = IDLE_TIME_CHANGE;
nixie_time_t nixie_time;

alarm_add_sm_t alarm_add_sm = IDLE_ALARM_ADD;
alarm_mode_args_t alarm_add;
alarm_add_digits_t alatm_add_digits;
struct tm alarm_add_timeinfo;
extern uint8_t alarm_type_uuid[ESP_UUID_LEN_128];

static void btn_timer_cb( TimerHandle_t xTimer )
{
    uint8_t button_id = 0xff;

    if (xTimer == btn_timer_h[0])
    {
        button_id = BUTTON_LEFT;
    }
    else if (xTimer == btn_timer_h[1])
    {
        button_id = BUTTON_CENTER;
    }
    else if (xTimer == btn_timer_h[2])
    {
        button_id = BUTTON_RIGHT;
    }
    else 
    {
        return;
    }

    bool action_happened = false;
    button_action_t action_handler;

    switch (button_sm[button_id].state)
    {
        case WAIT_ENABLE_CONTACT_VIBRATION:
        {
            if (button_sm[button_id].last_enable == false)
            {   
                if (xTimerChangePeriod(btn_timer_h[button_id], pdMS_TO_TICKS(3000), 1) == pdFAIL)
                {
                    ESP_LOGV(MAIN_TAG, "BTN %d WAIT_ENABLE_CONTACT_VIBRATION -> IDLE", button_id);
                    button_sm[button_id].state = IDLE;
                    return;
                }

                ESP_LOGV(MAIN_TAG, "BTN %d WAIT_ENABLE_CONTACT_VIBRATION -> WAIT_LONG_PRESS", button_id);
                button_sm[button_id].state = WAIT_LONG_PRESS;

                action_happened = true;
                action_handler.button = button_id;
                action_handler.action = HOLDING_SHORT;
            }
            else
            {
                action_happened = true;
                action_handler.button = button_id;
                action_handler.action = SHORT_PRESS;

                if (xTimerChangePeriod(btn_timer_h[button_id], pdMS_TO_TICKS(100), 1) == pdFAIL)
                {
                    ESP_LOGV(MAIN_TAG, "BTN %d WAIT_ENABLE_CONTACT_VIBRATION -> IDLE", button_id);
                    button_sm[button_id].state = IDLE;
                    return;
                }

                ESP_LOGV(MAIN_TAG, "BTN %d WAIT_ENABLE_CONTACT_VIBRATION -> WAIT_DISABLE_CONTACT_VIBRATION", button_id);
                button_sm[button_id].state = WAIT_DISABLE_CONTACT_VIBRATION;
            }
            break;
        }

        case WAIT_LONG_PRESS:
        {
            if (button_sm[button_id].last_enable == true)
            {
                action_happened = true;
                action_handler.button = button_id;
                action_handler.action = SHORT_PRESS;

                if (xTimerChangePeriod(btn_timer_h[button_id], pdMS_TO_TICKS(100), 1) == pdFAIL)
                {
                    ESP_LOGV(MAIN_TAG, "BTN %d WAIT_LONG_PRESS -> IDLE", button_id);
                    button_sm[button_id].state = IDLE;
                    break;
                }

                ESP_LOGV(MAIN_TAG, "BTN %d WAIT_LONG_PRESS -> WAIT_DISABLE_CONTACT_VIBRATION", button_id);
                button_sm[button_id].state = WAIT_DISABLE_CONTACT_VIBRATION;
            }
            else
            {
                action_happened = true;
                action_handler.button = button_id;
                action_handler.action = LONG_PRESS;

                ESP_LOGV(MAIN_TAG, "BTN %d WAIT_LONG_PRESS -> WAIT_DISABLE", button_id);
                button_sm[button_id].state = WAIT_DISABLE;
            }

            break;
        }

        case WAIT_DISABLE_CONTACT_VIBRATION:
        {
            ESP_LOGV(MAIN_TAG, "BTN %d WAIT_DISABLE_CONTACT_VIBRATION -> IDLE", button_id);
            button_sm[button_id].state = IDLE; 
            break;
        }

        default:
            break;
    }

    if (action_happened)
    {
        button_functions(action_handler);
    }
}

static void button_main(void* arg)
{
    button_queue_msg_t msg;
    uint8_t button_id;

    button_action_t action_handler;

    while(true) 
    {
        if (xQueueReceive(gpio_evt_queue, &msg, portMAX_DELAY) != pdTRUE)
        {
            continue;
        }
        
        switch (msg.gpio_num)
        {
            case GPIO_INPUT_IO_0:
                button_id = BUTTON_LEFT;
                break;
            
            case GPIO_INPUT_IO_1:
                button_id = BUTTON_CENTER;
                break;
            
            case GPIO_INPUT_IO_2:
                button_id = BUTTON_RIGHT;
                break;

            default:
                continue;
        }

        button_sm[button_id].last_enable = msg.enable;
        bool action_happened = false;

        switch (button_sm[button_id].state)
        {
            case IDLE:
            {
                if (button_sm[button_id].last_enable == false)
                {
                    if (xTimerChangePeriod(btn_timer_h[button_id], pdMS_TO_TICKS(100), 1) == pdFAIL)
                    {
                        return;
                    }

                    ESP_LOGV(MAIN_TAG, "BTN %d IDLE -> WAIT_ENABLE_CONTACT_VIBRATION", button_id);
                    button_sm[button_id].state = WAIT_ENABLE_CONTACT_VIBRATION;  
                }
                break;
            }

            case WAIT_LONG_PRESS:
            {
                if (button_sm[button_id].last_enable == true)
                {
                    action_happened = true;
                    action_handler.button = button_id;
                    action_handler.action = SHORT_PRESS;

                    if (xTimerChangePeriod(btn_timer_h[button_id], pdMS_TO_TICKS(100), 1) == pdFAIL)
                    {
                        ESP_LOGV(MAIN_TAG, "BTN %d WAIT_LONG_PRESS -> IDLE", button_id);
                        button_sm[button_id].state = IDLE;
                        break;
                    }

                    ESP_LOGV(MAIN_TAG, "BTN %d WAIT_LONG_PRESS -> WAIT_DISABLE_CONTACT_VIBRATION", button_id);
                    button_sm[button_id].state = WAIT_DISABLE_CONTACT_VIBRATION;
                }
                break;
            }

            case WAIT_DISABLE:
            {
                if (button_sm[button_id].last_enable == true)
                {
                    if (xTimerChangePeriod(btn_timer_h[button_id], pdMS_TO_TICKS(100), 1) == pdFAIL)
                    {
                        ESP_LOGV(MAIN_TAG, "BTN %d WAIT_DISABLE -> IDLE", button_id);
                        button_sm[button_id].state = IDLE;
                        break;
                    }

                    ESP_LOGV(MAIN_TAG, "BTN %d WAIT_DISABLE -> WAIT_DISABLE_CONTACT_VIBRATION", button_id);
                    button_sm[button_id].state = WAIT_DISABLE_CONTACT_VIBRATION;
                }
                break;
            }

            default:
            break;
        }

        if (action_happened)
        {
            button_functions(action_handler);
        }
    }
}

static void button_functions(button_action_t action_handler)
{
    switch (device_mode)
    {
        case DEFAULT_MODE:
        {
            switch (pairing_sm)
            {
                case WAIT_FOR_LEFT:
                {
                    if (action_handler.button == BUTTON_LEFT && action_handler.action == HOLDING_SHORT)
                    {
                        pairing_sm = WAIT_FOR_CENTER;
                    }
                    else
                    {
                        pairing_sm = WAIT_FOR_LEFT;
                    }
                    break;
                }

                case WAIT_FOR_CENTER:
                {
                    if (action_handler.button == BUTTON_CENTER && action_handler.action == HOLDING_SHORT)
                    {
                        pairing_sm = WAIT_FOR_LEFT_LONG;
                    }
                    else
                    {
                        pairing_sm = WAIT_FOR_LEFT;
                    }
                    break;
                }

                case WAIT_FOR_LEFT_LONG:
                {
                    if (action_handler.button == BUTTON_LEFT && action_handler.action == LONG_PRESS)
                    {
                        pairing_sm = WAIT_FOR_CENTER_LONG;
                    }
                    else
                    {
                        pairing_sm = WAIT_FOR_LEFT;
                    }
                    break;
                }

                case WAIT_FOR_CENTER_LONG:
                {
                    if (action_handler.button == BUTTON_CENTER && action_handler.action == LONG_PRESS)
                    {
                        ESP_LOGI(MAIN_TAG, "DEFAULT MODE -> PAIRING MODE");
                        pairing_sm = PAIRING;

                        device_mode = PAIRING_MODE;
                    }
                    else
                    {
                        pairing_sm = WAIT_FOR_LEFT;
                    }
                    break;
                }

                default:
                    break;
            }

            if (action_handler.button == BUTTON_LEFT && action_handler.action == LONG_PRESS && pairing_sm == WAIT_FOR_LEFT)
            {
                ESP_LOGI(MAIN_TAG, "DEFAULT MODE -> TIME CHANGE MODE");
                device_mode = TIME_CHANGE_MODE;
                time_change_mode(action_handler, true);
            }

            if (action_handler.button == BUTTON_CENTER && action_handler.action == LONG_PRESS && pairing_sm != PAIRING)
            {
                ESP_LOGI(MAIN_TAG, "DEFAULT MODE -> ALARM ADD MODE");
                device_mode = ALARM_ADD_MODE;
                alarm_add_mode(action_handler, true);
            }

            if (action_handler.button == BUTTON_RIGHT && action_handler.action == LONG_PRESS)
            {
                ESP_LOGI(MAIN_TAG, "DEFAULT MODE -> ALARM CHANGE MODE");
                device_mode = ALARM_CHANGE_MODE;
            }

            if (pairing_sm == PAIRING)
            {
                pairing_sm = WAIT_FOR_LEFT;
            }
            break;
        }

        case TIME_CHANGE_MODE:
        {
            time_change_mode(action_handler, false);
            break;
        }

        case ALARM_ADD_MODE:
        {
            alarm_add_mode(action_handler, false);
            break;
        }

        case ALARM_CHANGE_MODE:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                switch (action_handler.button)
                {
                    case BUTTON_LEFT:
                    {

                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        break;
                    }

                    case BUTTON_RIGHT:
                    {
                        ESP_LOGI(MAIN_TAG, "ALARM_CHANGE_MODE -> DEFAULT MODE");
                        device_mode = DEFAULT_MODE;
                        break;
                    }

                    default:
                        break;
                }
            }
            break;
        }

        case PAIRING_MODE:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                ESP_LOGI(MAIN_TAG, "PAIRING MODE -> DEFAULT MODE");
                device_mode = DEFAULT_MODE;
            }  
            break;
        }

        case ALARM_RING_MODE:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                ESP_LOGI(MAIN_TAG, "ALARM DISABLED");

                ESP_LOGI(MAIN_TAG, "ALARM RING MODE -> DEFAULT MODE");
                device_mode = DEFAULT_MODE;
                break;
            }
            break;
        }
    }
}

void set_device_mode(device_mode_t mode)
{
    device_mode = mode;
}

device_mode_t get_device_mode()
{
    return device_mode;
}

void time_change_mode(button_action_t action_handler, bool start)
{
    if (action_handler.action == SHORT_PRESS && action_handler.button == BUTTON_RIGHT)
    {
        ESP_LOGI(MAIN_TAG, "TIME CHANGE MODE -> DEFAULT MODE");
        time_change_sm = IDLE_TIME_CHANGE;
        device_mode = DEFAULT_MODE;
    }

    switch(time_change_sm)
    {
        case IDLE_TIME_CHANGE:
        {
            if (start)
            {
                time_t now;
                time(&now);
                struct tm timeinfo;
                localtime_r(&now, &timeinfo);

                nixie_time.hour_first = timeinfo.tm_hour / 10;
                nixie_time.hour_second = timeinfo.tm_hour % 10;
                nixie_time.minute_first = timeinfo.tm_min / 10;
                nixie_time.minute_second = timeinfo.tm_min % 10;
                nixie_time.second_first = timeinfo.tm_sec / 10;
                nixie_time.second_second = timeinfo.tm_sec % 10;
                nixie_time.day_first = timeinfo.tm_mday / 10;
                nixie_time.day_second = timeinfo.tm_mday % 10;
                nixie_time.month_first = (timeinfo.tm_mon + 1) / 10;
                nixie_time.month_second = (timeinfo.tm_mon + 1) % 10;
                nixie_time.year_first = (timeinfo.tm_year - 100) / 10;
                nixie_time.year_second = (timeinfo.tm_year - 100) % 10;

                time_change_sm = SET_HOUR_FIRST;
            }    
            break;
        }

        case SET_HOUR_FIRST:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                switch (action_handler.button)
                {
                    case BUTTON_LEFT:
                    {
                        nixie_time.hour_first++;
                        if (nixie_time.hour_first > 2) nixie_time.hour_first = 0;
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        time_change_sm = SET_HOUR_SECOND;
                        break;
                    }

                    default:
                        break;
                }
            }
            break;
        }

        case SET_HOUR_SECOND:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                switch (action_handler.button)
                {
                    case BUTTON_LEFT:
                    {
                        nixie_time.hour_second++;
                        if ( (nixie_time.hour_first < 2 && nixie_time.hour_second > 9) || (nixie_time.hour_first == 2 && nixie_time.hour_second > 3) )
                        {
                            nixie_time.hour_second = 0;
                        }
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        time_change_sm = SET_MINUTE_FIRST;
                        break;
                    }

                    default:
                        break;
                }
            }
            break;
        }

        case SET_MINUTE_FIRST:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                switch (action_handler.button)
                {
                    case BUTTON_LEFT:
                    {
                        nixie_time.minute_first++;
                        if (nixie_time.minute_first > 5) nixie_time.minute_first = 0;
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        time_change_sm = SET_MINUTE_SECOND;
                        break;
                    }

                    default:
                        break;
                }
            }
            break;
        }
        
        case SET_MINUTE_SECOND:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                switch (action_handler.button)
                {
                    case BUTTON_LEFT:
                    {
                        nixie_time.minute_second++;
                        if (nixie_time.minute_second > 9) nixie_time.minute_second = 0;
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        time_change_sm = SET_SECOND_FIRST;
                        break;
                    }

                    default:
                        break;
                }
            }
            break;
        }

        case SET_SECOND_FIRST:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                switch (action_handler.button)
                {
                    case BUTTON_LEFT:
                    {
                        nixie_time.second_first++;
                        if (nixie_time.second_first > 5) nixie_time.second_first = 0;
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        time_change_sm = SET_SECOND_SECOND;
                        break;
                    }

                    default:
                        break;
                }
            }
            break;
        }
        
        case SET_SECOND_SECOND:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                switch (action_handler.button)
                {
                    case BUTTON_LEFT:
                    {
                        nixie_time.second_second++;
                        if (nixie_time.second_second > 9) nixie_time.second_second = 0;
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        time_change_sm = SET_DAY_FIRST;
                        break;
                    }

                    default:
                        break;
                }
            }
            break;
        }

        case SET_DAY_FIRST:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                switch (action_handler.button)
                {
                    case BUTTON_LEFT:
                    {
                        nixie_time.day_first++;
                        if (nixie_time.day_first > 3) nixie_time.day_first = 0;
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        time_change_sm = SET_DAY_SECOND;
                        break;
                    }

                    default:
                        break;
                }
            }
            break;
        }

        case SET_DAY_SECOND:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                switch (action_handler.button)
                {
                    case BUTTON_LEFT:
                    {
                        nixie_time.day_second++;

                        if ( (nixie_time.day_first > 2 && nixie_time.day_second > 1) || (nixie_time.day_first < 2 && nixie_time.day_second > 9) )
                        {
                            nixie_time.day_second = 0;
                        }
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        time_change_sm = SET_MONTH_FIRST;
                        break;
                    }

                    default:
                        break;
                }
            }
            break;
        }

        case SET_MONTH_FIRST:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                switch (action_handler.button)
                {
                    case BUTTON_LEFT:
                    {
                        nixie_time.month_first++;
                        if (nixie_time.month_first > 1) nixie_time.month_first = 0;
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        time_change_sm = SET_MONTH_SECOND;
                        break;
                    }

                    default:
                        break;
                }
            }
            break;
        }

        case SET_MONTH_SECOND:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                switch (action_handler.button)
                {
                    case BUTTON_LEFT:
                    {
                        nixie_time.month_second++;
                        if ( (nixie_time.month_first > 0 && nixie_time.month_second > 2) || (nixie_time.month_first == 0 && nixie_time.month_second > 9) )
                        {
                            nixie_time.month_second = 0;
                        }
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        time_change_sm = SET_YEAR_FIRST;
                        break;
                    }

                    default:
                        break;
                }
            }
            break;
        }

        case SET_YEAR_FIRST:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                switch (action_handler.button)
                {
                    case BUTTON_LEFT:
                    {
                        nixie_time.year_first++;
                        if (nixie_time.year_first > 9) nixie_time.year_first = 0;
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        time_change_sm = SET_YEAR_SECOND;
                        break;
                    }

                    default:
                        break;
                }
            }
            break;
        }

        case SET_YEAR_SECOND:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                switch (action_handler.button)
                {
                    case BUTTON_LEFT:
                    {
                        nixie_time.year_second++;
                        if (nixie_time.year_second > 9) nixie_time.year_second = 0;
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        time_change_sm = IDLE_TIME_CHANGE;
                        device_mode = DEFAULT_MODE;

                        struct tm tm;
                        tm.tm_year 	= nixie_time.year_first * 10 + nixie_time.year_second + 100;
                        tm.tm_mon 	= nixie_time.month_first * 10 + nixie_time.month_second - 1;
                        tm.tm_mday 	= nixie_time.day_first * 10 + nixie_time.day_second;
                        tm.tm_hour 	= nixie_time.hour_first * 10 + nixie_time.hour_second;
                        tm.tm_min 	= nixie_time.minute_first * 10 + nixie_time.minute_second;
                        tm.tm_sec 	= nixie_time.second_first * 10 + nixie_time.second_second;
                        tm.tm_isdst = 0;

                        time_t t = mktime(&tm);

                        struct timeval tv;
                        tv.tv_sec = t;
                        settimeofday(&tv, NULL);

                        pp_rtc_set_time( tm.tm_sec, tm.tm_min, tm.tm_hour, 1, tm.tm_mday, tm.tm_mon + 1, tm.tm_year - 100 );
                        break;
                    }

                    default:
                        break;
                }
            }
            break;
        }    
    }
}

void alarm_add_mode(button_action_t action_handler, bool start)
{
    if (action_handler.action == SHORT_PRESS && action_handler.button == BUTTON_RIGHT)
    {
        ESP_LOGI(MAIN_TAG, "ALARM_ADD_MODE -> DEFAULT MODE");
        alarm_add_sm = IDLE_ALARM_ADD;
        device_mode = DEFAULT_MODE;
    }

    switch(alarm_add_sm)
    {
        case IDLE_ALARM_ADD:
        {
            if (start)
            {
                time_t now;
                time(&now);
                localtime_r(&now, &alarm_add_timeinfo);

                alatm_add_digits.mode = ALARM_SINGLE_MODE;

                alatm_add_digits.time.hour_first = 1;
                alatm_add_digits.time.hour_second = 2;
                alatm_add_digits.time.minute_first = 0;
                alatm_add_digits.time.minute_second = 0;

                alatm_add_digits.time.day_first = alarm_add_timeinfo.tm_mday / 10;
                alatm_add_digits.time.day_second = alarm_add_timeinfo.tm_mday % 10;
                alatm_add_digits.time.month_first = (alarm_add_timeinfo.tm_mon + 1) / 10;
                alatm_add_digits.time.month_second = (alarm_add_timeinfo.tm_mon + 1) % 10;
                alatm_add_digits.time.year_first = (alarm_add_timeinfo.tm_year - 100) / 10;
                alatm_add_digits.time.year_second = (alarm_add_timeinfo.tm_year - 100) % 10;

                alatm_add_digits.monday = 0;
                alatm_add_digits.tuesday = 0;
                alatm_add_digits.wednesday = 0;
                alatm_add_digits.thursday = 0;
                alatm_add_digits.friday = 0;
                alatm_add_digits.saturday = 0;
                alatm_add_digits.sunday = 0;

                alatm_add_digits.volume = 9;

                memset(&alarm_add, 0, sizeof(alarm_add));
                alarm_add_sm = SET_MODE;
            }    
            break;
        }

        case SET_MODE:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                switch (action_handler.button)
                {
                    case BUTTON_LEFT:
                    {
                        alatm_add_digits.mode++;
                        if (alatm_add_digits.mode > 3) alatm_add_digits.mode = 0;
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        alarm_add_sm = SET_ALARM_HOUR_FIRST;
                        break;
                    }

                    default:
                        break;
                }
            }
            break;
        }

        case SET_ALARM_HOUR_FIRST:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                switch (action_handler.button)
                {
                    case BUTTON_LEFT:
                    {
                        alatm_add_digits.time.hour_first++;
                        if (alatm_add_digits.time.hour_first > 2) alatm_add_digits.time.hour_first = 0;
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        alarm_add_sm = SET_ALARM_HOUR_SECOND;
                        break;
                    }

                    default:
                        break;
                }
            }
            break;
        }

        case SET_ALARM_HOUR_SECOND:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                switch (action_handler.button)
                {
                    case BUTTON_LEFT:
                    {
                        alatm_add_digits.time.hour_second++;
                        if ( (alatm_add_digits.time.hour_first < 2 && alatm_add_digits.time.hour_second > 9) || (alatm_add_digits.time.hour_first == 2 && alatm_add_digits.time.hour_second > 3) )
                        {
                            alatm_add_digits.time.hour_second = 0;
                        }
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        alarm_add_sm = SET_ALARM_MINUTE_FIRST;
                        break;
                    }

                    default:
                        break;
                }
            }
            break;
        }

        case SET_ALARM_MINUTE_FIRST:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                switch (action_handler.button)
                {
                    case BUTTON_LEFT:
                    {
                        alatm_add_digits.time.minute_first++;
                        if (alatm_add_digits.time.minute_first > 5) alatm_add_digits.time.minute_first = 0;
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        alarm_add_sm = SET_ALARM_MINUTE_SECOND;
                        break;
                    }

                    default:
                        break;
                }
            }
            break;
        }
        
        case SET_ALARM_MINUTE_SECOND:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                switch (action_handler.button)
                {
                    case BUTTON_LEFT:
                    {
                        alatm_add_digits.time.minute_second++;
                        if (alatm_add_digits.time.minute_second > 9) alatm_add_digits.time.minute_second = 0;
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        switch(alatm_add_digits.mode)
                        {
                            case ALARM_SINGLE_MODE:
                                alarm_add_sm = SET_SINGLE_DAY_FIRST;
                                break;

                            case ALARM_WEEKLY_MODE:
                                alarm_add_sm = SET_WEEKLY_MONDAY;
                                break;

                            case ALARM_MONTHLY_MODE:
                                alarm_add_sm = SET_MONTHLY_DAY_FIRST;
                                break;

                            case ALARM_YEARLY_MODE:
                                alarm_add_sm = SET_YEARLY_DAY_FIRST;
                                break;

                            default:
                                alarm_add_sm = SET_SINGLE_DAY_FIRST;
                                break;
                        }
                        
                        break;
                    }

                    default:
                        break;
                }
            }
            break;
        }

        case SET_SINGLE_DAY_FIRST:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                switch (action_handler.button)
                {
                    case BUTTON_LEFT:
                    {
                        alatm_add_digits.time.day_first++;
                        if (alatm_add_digits.time.day_first > 3) alatm_add_digits.time.day_first = 0;
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        alarm_add_sm = SET_SINGLE_DAY_SECOND;
                        break;
                    }

                    default:
                        break;
                }
            }
            break;
        }

        case SET_SINGLE_DAY_SECOND:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                switch (action_handler.button)
                {
                    case BUTTON_LEFT:
                    {
                        alatm_add_digits.time.day_second++;

                        if ( (alatm_add_digits.time.day_first > 2 && alatm_add_digits.time.day_second > 1) || (alatm_add_digits.time.day_first < 2 && alatm_add_digits.time.day_second > 9) )
                        {
                            alatm_add_digits.time.day_second = 0;
                        }
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        alarm_add_sm = SET_SINGLE_MONTH_FIRST;
                        break;
                    }

                    default:
                        break;
                }
            }
            break;
        }

        case SET_SINGLE_MONTH_FIRST:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                switch (action_handler.button)
                {
                    case BUTTON_LEFT:
                    {
                        alatm_add_digits.time.month_first++;
                        if (alatm_add_digits.time.month_first > 1) alatm_add_digits.time.month_first = 0;
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        alarm_add_sm = SET_SINGLE_MONTH_SECOND;
                        break;
                    }

                    default:
                        break;
                }
            }
            break;
        }

        case SET_SINGLE_MONTH_SECOND:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                switch (action_handler.button)
                {
                    case BUTTON_LEFT:
                    {
                        alatm_add_digits.time.month_second++;
                        if ( (alatm_add_digits.time.month_first > 0 && alatm_add_digits.time.month_second > 2) || (alatm_add_digits.time.month_first == 0 && alatm_add_digits.time.month_second > 9) )
                        {
                            alatm_add_digits.time.month_second = 0;
                        }
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        alarm_add_sm = SET_SINGLE_YEAR_FIRST;
                        break;
                    }

                    default:
                        break;
                }
            }
            break;
        }

        case SET_SINGLE_YEAR_FIRST:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                switch (action_handler.button)
                {
                    case BUTTON_LEFT:
                    {
                        alatm_add_digits.time.year_first++;
                        if (alatm_add_digits.time.year_first > 9) alatm_add_digits.time.year_first = 0;
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        alarm_add_sm = SET_SINGLE_YEAR_SECOND;
                        break;
                    }

                    default:
                        break;
                }
            }
            break;
        }

        case SET_SINGLE_YEAR_SECOND:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                switch (action_handler.button)
                {
                    case BUTTON_LEFT:
                    {
                        alatm_add_digits.time.year_second++;
                        if (alatm_add_digits.time.year_second > 9) alatm_add_digits.time.year_second = 0;
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        alarm_add_sm = SET_VOLUME;
                        break;
                    }

                    default:
                        break;
                }
            }
            break;
        }

        case SET_WEEKLY_MONDAY:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                switch (action_handler.button)
                {
                    case BUTTON_LEFT:
                    {
                        alatm_add_digits.monday++;
                        if (alatm_add_digits.monday > 1) alatm_add_digits.monday = 0;
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        alarm_add_sm = SET_WEEKLY_TUESDAY;
                        break;
                    }

                    default:
                        break;
                }
            }
            break;
        }

        case SET_WEEKLY_TUESDAY:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                switch (action_handler.button)
                {
                    case BUTTON_LEFT:
                    {
                        alatm_add_digits.tuesday++;
                        if (alatm_add_digits.tuesday > 1) alatm_add_digits.tuesday = 0;
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        alarm_add_sm = SET_WEEKLY_WEDNESDAY;
                        break;
                    }

                    default:
                        break;
                }
            }
            break;
        }

        case SET_WEEKLY_WEDNESDAY:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                switch (action_handler.button)
                {
                    case BUTTON_LEFT:
                    {
                        alatm_add_digits.wednesday++;
                        if (alatm_add_digits.wednesday > 1) alatm_add_digits.wednesday = 0;
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        alarm_add_sm = SET_WEEKLY_THURSDAY;
                        break;
                    }

                    default:
                        break;
                }
            }
            break;
        }

        case SET_WEEKLY_THURSDAY:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                switch (action_handler.button)
                {
                    case BUTTON_LEFT:
                    {
                        alatm_add_digits.thursday++;
                        if (alatm_add_digits.thursday > 1) alatm_add_digits.thursday = 0;
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        alarm_add_sm = SET_WEEKLY_FRIDAY;
                        break;
                    }

                    default:
                        break;
                }
            }
            break;
        }

        case SET_WEEKLY_FRIDAY:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                switch (action_handler.button)
                {
                    case BUTTON_LEFT:
                    {
                        alatm_add_digits.friday++;
                        if (alatm_add_digits.friday > 1) alatm_add_digits.friday = 0;
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        alarm_add_sm = SET_WEEKLY_SATURDAY;
                        break;
                    }

                    default:
                        break;
                }
            }
            break;
        }

        case SET_WEEKLY_SATURDAY:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                switch (action_handler.button)
                {
                    case BUTTON_LEFT:
                    {
                        alatm_add_digits.saturday++;
                        if (alatm_add_digits.saturday > 1) alatm_add_digits.saturday = 0;
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        alarm_add_sm = SET_WEEKLY_SUNDAY;
                        break;
                    }

                    default:
                        break;
                }
            }
            break;
        }

        case SET_WEEKLY_SUNDAY:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                switch (action_handler.button)
                {
                    case BUTTON_LEFT:
                    {
                        alatm_add_digits.sunday++;
                        if (alatm_add_digits.sunday > 1) alatm_add_digits.sunday = 0;
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        alarm_add_sm = SET_VOLUME;
                        break;
                    }

                    default:
                        break;
                }
            }
            break;
        }

        case SET_MONTHLY_DAY_FIRST:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                switch (action_handler.button)
                {
                    case BUTTON_LEFT:
                    {
                        alatm_add_digits.time.day_first++;
                        if (alatm_add_digits.time.day_first > 3) alatm_add_digits.time.day_first = 0;
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        alarm_add_sm = SET_MONTHLY_DAY_SECOND;
                        break;
                    }

                    default:
                        break;
                }
            }
            break;
        }

        case SET_MONTHLY_DAY_SECOND:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                switch (action_handler.button)
                {
                    case BUTTON_LEFT:
                    {
                        alatm_add_digits.time.day_second++;

                        if ( (alatm_add_digits.time.day_first > 2 && alatm_add_digits.time.day_second > 1) || (alatm_add_digits.time.day_first < 2 && alatm_add_digits.time.day_second > 9) )
                        {
                            alatm_add_digits.time.day_second = 0;
                        }
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        alarm_add_sm = SET_VOLUME;
                        break;
                    }

                    default:
                        break;
                }
            }
            break;
        }

        case SET_YEARLY_DAY_FIRST:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                switch (action_handler.button)
                {
                    case BUTTON_LEFT:
                    {
                        alatm_add_digits.time.day_first++;
                        if (alatm_add_digits.time.day_first > 3) alatm_add_digits.time.day_first = 0;
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        alarm_add_sm = SET_YEARLY_DAY_SECOND;
                        break;
                    }

                    default:
                        break;
                }
            }
            break;
        }

        case SET_YEARLY_DAY_SECOND:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                switch (action_handler.button)
                {
                    case BUTTON_LEFT:
                    {
                        alatm_add_digits.time.day_second++;

                        if ( (alatm_add_digits.time.day_first > 2 && alatm_add_digits.time.day_second > 1) || (alatm_add_digits.time.day_first < 2 && alatm_add_digits.time.day_second > 9) )
                        {
                            alatm_add_digits.time.day_second = 0;
                        }
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        alarm_add_sm = SET_YEARLY_MONTH_FIRST;
                        break;
                    }

                    default:
                        break;
                }
            }
            break;
        }

        case SET_YEARLY_MONTH_FIRST:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                switch (action_handler.button)
                {
                    case BUTTON_LEFT:
                    {
                        alatm_add_digits.time.month_first++;
                        if (alatm_add_digits.time.month_first > 1) alatm_add_digits.time.month_first = 0;
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        alarm_add_sm = SET_YEARLY_MONTH_SECOND;
                        break;
                    }

                    default:
                        break;
                }
            }
            break;
        }

        case SET_YEARLY_MONTH_SECOND:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                switch (action_handler.button)
                {
                    case BUTTON_LEFT:
                    {
                        alatm_add_digits.time.month_second++;
                        if ( (alatm_add_digits.time.month_first > 0 && alatm_add_digits.time.month_second > 2) || (alatm_add_digits.time.month_first == 0 && alatm_add_digits.time.month_second > 9) )
                        {
                            alatm_add_digits.time.month_second = 0;
                        }
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        alarm_add_sm = SET_VOLUME;
                        break;
                    }

                    default:
                        break;
                }
            }
            break;
        }

        case SET_VOLUME:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                switch (action_handler.button)
                {
                    case BUTTON_LEFT:
                    {
                        alatm_add_digits.volume++;
                        if (alatm_add_digits.volume > 9) alatm_add_digits.volume = 0;
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        alarm_add_sm = IDLE_ALARM_ADD;
                        device_mode = DEFAULT_MODE;

                        alarm_add.mode = alatm_add_digits.mode;
                        alarm_add.enable = true;
                        alarm_add.desc_len = 0;
                        alarm_add.desc[0] = '\0';
                        alarm_add.hour = alatm_add_digits.time.hour_first * 10 + alatm_add_digits.time.hour_second;
                        alarm_add.minute = alatm_add_digits.time.minute_first * 10 + alatm_add_digits.time.minute_second;

                        switch(alatm_add_digits.mode)
                        {
                            case ALARM_SINGLE_MODE:
                                alarm_add.args.single_alarm_args.day = alatm_add_digits.time.day_first * 10 + alatm_add_digits.time.day_second;
                                alarm_add.args.single_alarm_args.month = alatm_add_digits.time.month_first * 10 + alatm_add_digits.time.month_second;
                                alarm_add.args.single_alarm_args.year = alatm_add_digits.time.year_first * 10 + alatm_add_digits.time.year_second;
                                break;

                            case ALARM_WEEKLY_MODE:
                                if(alatm_add_digits.monday) alarm_add.args.days |= (1 << 0);
                                if(alatm_add_digits.tuesday) alarm_add.args.days |= (1 << 1);
                                if(alatm_add_digits.wednesday) alarm_add.args.days |= (1 << 2);
                                if(alatm_add_digits.thursday) alarm_add.args.days |= (1 << 3);
                                if(alatm_add_digits.friday) alarm_add.args.days |= (1 << 4);
                                if(alatm_add_digits.saturday) alarm_add.args.days |= (1 << 5);
                                if(alatm_add_digits.sunday) alarm_add.args.days |= (1 << 6);
                                break;

                            case ALARM_MONTHLY_MODE:
                                alarm_add.args.day = alatm_add_digits.time.day_first * 10 + alatm_add_digits.time.day_second;
                                break;

                            case ALARM_YEARLY_MODE:
                                alarm_add.args.yearly_alarm_args.day = alatm_add_digits.time.day_first * 10 + alatm_add_digits.time.day_second;
                                alarm_add.args.yearly_alarm_args.month = alatm_add_digits.time.month_first * 10 + alatm_add_digits.time.month_second;
                                break;

                            default:
                                break;
                        }

                        if (alatm_add_digits.volume == 0)
                        {
                            alarm_add.volume = 100;
                        }
                        else
                        {
                            alarm_add.volume = alatm_add_digits.volume * 11;
                        }

                        esp_bt_uuid_t type;
                        type.len = ESP_UUID_LEN_128;
                        oacp_op_code_result_t result;
                        memcpy(type.uuid.uuid128, alarm_type_uuid, ESP_UUID_LEN_128);

                        esp_err_t ret = ObjectManager_create_object(0, type, &result);
                        if (ret) 
                        {
                            ESP_LOGI(MAIN_TAG, "ObjectManager_create_object: %x", ret);
                            break;
                        }

                        ret = ObjectManager_change_alarm_data_in_file(alarm_add);
                        if (ret) 
                        {
                            ESP_LOGI(MAIN_TAG, "ObjectManager_change_alarm_data_in_file: %x", ret);
                            break;
                        }

                        set_next_alarm();
                        ESP_LOGI(MAIN_TAG, "ALARM_ADD_MODE -> DEFAULT MODE");
                        
                        break;
                    }

                    default:
                        break;
                }
            }
            break;
        }    
    }
}

static esp_err_t button_init()
{
    /* BUTTONS GPIO CONFIG */

    //zero-initialize the config structure.
    gpio_config_t io_conf = {};

    //interrupt of any edge
    io_conf.intr_type = GPIO_INTR_ANYEDGE;
    //bit mask of the pins, use GPIO4/5 here
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    //set as input mode
    io_conf.mode = GPIO_MODE_INPUT;
    //enable pull-up mode
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);

    for (uint i=0; i<3; i++)
    {
        button_sm[i].state = IDLE;
        button_sm[i].last_enable = false;
    }

    //create a queue to handle gpio event from isr
    gpio_evt_queue = xQueueCreate(20, sizeof(button_queue_msg_t));
    //start gpio task
    xTaskCreate(button_main, "BUTTON_MAIN", 3072, NULL, 1, NULL);

    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    gpio_isr_handler_add(GPIO_INPUT_IO_0, button_isr_handler, (void*) GPIO_INPUT_IO_0);
    gpio_isr_handler_add(GPIO_INPUT_IO_1, button_isr_handler, (void*) GPIO_INPUT_IO_1);
    gpio_isr_handler_add(GPIO_INPUT_IO_2, button_isr_handler, (void*) GPIO_INPUT_IO_2);

    btn_timer_h[0] = xTimerCreate(NULL, pdMS_TO_TICKS(100), pdFALSE, NULL, btn_timer_cb);
    btn_timer_h[1] = xTimerCreate(NULL, pdMS_TO_TICKS(100), pdFALSE, NULL, btn_timer_cb);
    btn_timer_h[2] = xTimerCreate(NULL, pdMS_TO_TICKS(100), pdFALSE, NULL, btn_timer_cb);
    
    return ESP_OK;
}

void app_main(void)
{
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);
    gpio_set_level(GPIO_OUTPUT_OE, 0);
    gpio_set_level(GPIO_OUTPUT_RED, 1);

    ESP_LOGI(MAIN_TAG, "Initializing sd card");
    esp_err_t ret = sd_cad_init();
    if (ret) {
        ESP_LOGE(MAIN_TAG, "sd card initialize failed, err: %x", ret);
        return;
    }

    ESP_LOGI(MAIN_TAG, "Initializing NVS");
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
        if (ret)
        {
            ESP_LOGE(MAIN_TAG, "Failed to initialize nvs.");
            return;
        }
    }

    ret = i2c_init(I2C_MASTER_NUM, GPIO_NUM_18, GPIO_NUM_8, 100);
    if (ret) {
        ESP_LOGE(MAIN_TAG, "i2c init failed, err: %x", ret);
        return;
    }

    ret = pp_rtc_init();
    if (ret) {
        ESP_LOGE(MAIN_TAG, "rtc init failed, err: %x", ret);
        return;
    }

    ret = pp_nixie_diplay_init();
    if (ret) {
        ESP_LOGE(MAIN_TAG, "nixie display failed, err: %x", ret);
        return;
    }

    ret = ObjectManager_init();
    if (ret) {
        ESP_LOGE(MAIN_TAG, "Object Manager failed, err: %x", ret);
        return;
    }

    ret = alarm_init();
    if (ret) {
        ESP_LOGE(MAIN_TAG, "Alarm init failed, err: %x", ret);
        return;
    }

    ret = pp_wave_player_init();
    if (ret) {
        ESP_LOGE(MAIN_TAG, "Wave player init failed, err: %x", ret);
        return;
    }

    set_next_alarm();

    ret = button_init();
    if (ret) {
        ESP_LOGE(MAIN_TAG, "Button init failed, err: %x", ret);
        return;
    }

    ESP_LOGI(MAIN_TAG, "Initializing wifi");
    ret = wifi_init();
    if (ret) {
        ESP_LOGE(MAIN_TAG, "wifi init failed, err: %x", ret);
        return;
    }

    ESP_LOGI(MAIN_TAG, "Initializing bluetooth");
    ret = bt_init();
    if (ret) {
        ESP_LOGE(MAIN_TAG, "bt init failed, err: %x", ret);
        return;
    }


}
