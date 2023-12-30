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
    uint32_t passkey = 123456;
    uint8_t auth_option = ESP_BLE_ONLY_ACCEPT_SPECIFIED_AUTH_DISABLE;
    uint8_t oob_support = ESP_BLE_OOB_DISABLE;
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_STATIC_PASSKEY, &passkey, sizeof(uint32_t));
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
