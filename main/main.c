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

static void IRAM_ATTR button_isr_handler(void* arg)
{
    button_queue_msg_t msg;
    msg.mode = EDGE;
    msg.gpio_num = (uint32_t) arg;
    
    xQueueSendFromISR(gpio_evt_queue, &msg, NULL);
}

static void button_main(void* arg)
{
    button_queue_msg_t msg;
    struct timeval tv_now;
    int64_t next_stamp;
    bool got_from_queue = false;

    while(true) 
    {
        if(xQueueReceive(gpio_evt_queue, &msg, 1)) 
        {
            got_from_queue = true;
        }
        else
        {
            got_from_queue = false;
        }

        uint8_t button_id = 0xff;
        
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
        }

        if (button_id >= 3)
        {
            continue;
        }

        if (got_from_queue)
        {
            button_sm[button_id].last_enable = !gpio_get_level(msg.gpio_num);
        }

        switch (button_sm[button_id].state)
        {
            case IDLE:
            {
                if (got_from_queue && msg.mode == EDGE && button_sm[button_id].last_enable)
                {
                    // ESP_LOGI(MAIN_TAG, "IDLE -> WAIT_ENABLE_CONTACT_VIBRATION");
                    button_sm[button_id].state = WAIT_ENABLE_CONTACT_VIBRATION;

                    gettimeofday(&tv_now, NULL);
                    button_sm[button_id].stamp = (int64_t)tv_now.tv_sec * 1000000L + (int64_t)tv_now.tv_usec;
                }
                break;
            }

            case WAIT_ENABLE_CONTACT_VIBRATION:
            {
                gettimeofday(&tv_now, NULL);
                next_stamp = (int64_t)tv_now.tv_sec * 1000000L + (int64_t)tv_now.tv_usec;

                if (next_stamp - button_sm[button_id].stamp >= 100000)
                {
                    gettimeofday(&tv_now, NULL);
                    button_sm[button_id].stamp = (int64_t)tv_now.tv_sec * 1000000L + (int64_t)tv_now.tv_usec;

                    if (button_sm[button_id].last_enable)
                    {
                        // ESP_LOGI(MAIN_TAG, "WAIT_ENABLE_CONTACT_VIBRATION -> WAIT_LONG_PRESS");
                        button_sm[button_id].state = WAIT_LONG_PRESS;
                    }
                    else
                    {
                        // ESP_LOGI(MAIN_TAG, "WAIT_ENABLE_CONTACT_VIBRATION -> WAIT_DISABLE_CONTACT_VIBRATION");
                        ESP_LOGI(MAIN_TAG, "SHORT PRESS");
                        button_sm[button_id].state = WAIT_DISABLE_CONTACT_VIBRATION;
                    }
                }
                break;
            }

            case WAIT_LONG_PRESS:
            {
                if (got_from_queue && msg.mode == EDGE && !button_sm[button_id].last_enable)
                {
                    gettimeofday(&tv_now, NULL);
                    button_sm[button_id].stamp = (int64_t)tv_now.tv_sec * 1000000L + (int64_t)tv_now.tv_usec;

                    // ESP_LOGI(MAIN_TAG, "WAIT_LONG_PRESS -> WAIT_DISABLE_CONTACT_VIBRATION");
                    ESP_LOGI(MAIN_TAG, "SHORT PRESS");
                    button_sm[button_id].state = WAIT_DISABLE_CONTACT_VIBRATION;
                }
                else
                {
                    gettimeofday(&tv_now, NULL);
                    next_stamp = (int64_t)tv_now.tv_sec * 1000000L + (int64_t)tv_now.tv_usec;

                    if (next_stamp - button_sm[button_id].stamp >= 3000000)
                    {
                        ESP_LOGI(MAIN_TAG, "LONG PRESS");
                        gettimeofday(&tv_now, NULL);
                        button_sm[button_id].stamp = (int64_t)tv_now.tv_sec * 1000000L + (int64_t)tv_now.tv_usec;

                        // ESP_LOGI(MAIN_TAG, "WAIT_LONG_PRESS -> WAIT_DISABLE");
                        button_sm[button_id].state = WAIT_DISABLE;
                    }
                }

                break;
            }

            case WAIT_DISABLE:
            {
                if (got_from_queue && msg.mode == EDGE && !button_sm[button_id].last_enable)
                {
                    gettimeofday(&tv_now, NULL);
                    button_sm[button_id].stamp = (int64_t)tv_now.tv_sec * 1000000L + (int64_t)tv_now.tv_usec;

                    // ESP_LOGI(MAIN_TAG, "WAIT_DISABLE -> WAIT_DISABLE_CONTACT_VIBRATION");
                    button_sm[button_id].state = WAIT_DISABLE_CONTACT_VIBRATION;
                }

                break;
            }

            case WAIT_DISABLE_CONTACT_VIBRATION:
            {
                gettimeofday(&tv_now, NULL);
                next_stamp = (int64_t)tv_now.tv_sec * 1000000L + (int64_t)tv_now.tv_usec;

                if (next_stamp - button_sm[button_id].stamp >= 100000)
                {
                    button_sm[button_id].state = IDLE;
                    // ESP_LOGI(MAIN_TAG, "WAIT_DISABLE_CONTACT_VIBRATION -> IDLE");
                }
                break;
            }
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
    return ESP_OK;
}

void app_main(void)
{
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
