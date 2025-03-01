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
#include "pp_bluetooth.h"

/* Macros */
#define BLUETOOTH_TAG "BLUETOOTH"
#define GATTS_TAG "GATTS"

/* Declarations */
static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);
static void gatts_profile_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);

/* Variables */
static const uint16_t GATTS_OTP_SRV                     = 0x1825;
static const uint16_t GATTS_CHAR_OTS_FEATURE            = 0x2ABD;
static const uint16_t GATTS_CHAR_OBJECT_NAME            = 0x2ABE;
static const uint16_t GATTS_CHAR_OBJECT_TYPE            = 0x2ABF;
static const uint16_t GATTS_CHAR_OBJECT_SIZE            = 0x2AC0;
static const uint16_t GATTS_CHAR_OBJECT_ID              = 0x2AC3;
static const uint16_t GATTS_CHAR_OBJECT_PROPERTIES      = 0x2AC4;
static const uint16_t GATTS_CHAR_OBJECT_OACP            = 0x2AC5;
static const uint16_t GATTS_CHAR_OBJECT_OLCP            = 0x2AC6;
static const uint16_t GATTS_CHAR_OBJECT_LIST_FILTER     = 0x2AC7;
static uint8_t GATTS_CHAR_ALARM_ACTION[16]              = {0x26, 0xab, 0x57, 0xe0, 0x57, 0xab, 0x45, 0x98, 0xaf, 0xf2, 0x06, 0xe5, 0x27, 0x3f, 0x91, 0x9e};
// static uint8_t GATTS_CHAR_RINGTONE_ACTION[16]           = {0x26, 0xab, 0x57, 0xe0, 0x57, 0xab, 0x45, 0x98, 0xaf, 0xf2, 0x06, 0xe5, 0x27, 0x5c, 0xe5, 0x40};      TBD
static uint8_t GATTS_CHAR_WIFI_ACTION[16]               = {0x26, 0xab, 0x57, 0xe0, 0x57, 0xab, 0x45, 0x98, 0xaf, 0xf2, 0x06, 0xe5, 0x27, 0x2a, 0x14, 0x80};

static const uint16_t primary_service_uuid          = ESP_GATT_UUID_PRI_SERVICE;
static const uint16_t char_declaration_uuid         = ESP_GATT_UUID_CHAR_DECLARE;
static const uint16_t character_client_config_uuid  = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;
static const uint8_t char_prop_read                 = ESP_GATT_CHAR_PROP_BIT_READ;
static const uint8_t char_prop_read_write           = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE;
static const uint8_t char_prop_write_indicate       = ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_INDICATE;
static const uint8_t char_prop_read_write_indicate  = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_INDICATE;

static const uint8_t OTS_Feature_value[8]      = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30};

/* Full Database Description - Used to add attributes into the database */
static const esp_gatts_attr_db_t gatt_db[OPT_IDX_NB] =
{
    /* OPT Service Declaration */
    [OPT_IDX_SVC]        =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&primary_service_uuid, ESP_GATT_PERM_READ,
      sizeof(uint16_t), sizeof(GATTS_OTP_SRV), (uint8_t *)&GATTS_OTP_SRV}},


    /* OTS Feature Characteristic Declaration */
    [OPT_IDX_CHAR_OTS_FEATURE]     =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&char_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read}},

    /* OTS Feature Characteristic Value */
    [OPT_IDX_CHAR_OTS_FEATURE_VAL] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_OTS_FEATURE, ESP_GATT_PERM_READ,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, sizeof(OTS_Feature_value), (uint8_t *)OTS_Feature_value}},


    /* Object Name Characteristic Declaration */
    [OPT_IDX_CHAR_OBJECT_NAME]     =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&char_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_write}},

    /* Object Name Characteristic Value */
    [OPT_IDX_CHAR_OBJECT_NAME_VAL] =
    {{ESP_GATT_RSP_BY_APP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_OBJECT_NAME, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, 0, NULL}},


    /* Object Type Characteristic Declaration */
    [OPT_IDX_CHAR_OBJECT_TYPE]     =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&char_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read}},

    /* Object Type Characteristic Value */
    [OPT_IDX_CHAR_OBJECT_TYPE_VAL] =
    {{ESP_GATT_RSP_BY_APP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_OBJECT_TYPE, ESP_GATT_PERM_READ,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, 0, NULL}},


    /* Object Size Characteristic Declaration */
    [OPT_IDX_CHAR_OBJECT_SIZE]     =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&char_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read}},

    /* Object Size Characteristic Value */
    [OPT_IDX_CHAR_OBJECT_SIZE_VAL] =
    {{ESP_GATT_RSP_BY_APP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_OBJECT_SIZE, ESP_GATT_PERM_READ,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, 0, NULL}},


    /* Object ID Characteristic Declaration */
    [OPT_IDX_CHAR_OBJECT_ID]     =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&char_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read}},

    /* Object ID Characteristic Value */
    [OPT_IDX_CHAR_OBJECT_ID_VAL] =
    {{ESP_GATT_RSP_BY_APP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_OBJECT_ID, ESP_GATT_PERM_READ,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, 0, NULL}},


    /* Object Properties Characteristic Declaration */
    [OPT_IDX_CHAR_OBJECT_PROPERTIES]     =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&char_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_write}},

    /* Object Properties Characteristic Value */
    [OPT_IDX_CHAR_OBJECT_PROPERTIES_VAL] =
    {{ESP_GATT_RSP_BY_APP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_OBJECT_PROPERTIES, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, 0, NULL}},


    /* Object OACP Characteristic Declaration */
    [OPT_IDX_CHAR_OBJECT_OACP]     =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&char_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_write_indicate}},

    /* Object OACP Characteristic Value */
    [OPT_IDX_CHAR_OBJECT_OACP_VAL] =
    {{ESP_GATT_RSP_BY_APP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_OBJECT_OACP, ESP_GATT_PERM_WRITE,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, 0, NULL}},

    /* Object OACP Client Characteristic Configuration Descriptor */
    [OPT_IDX_CHAR_OBJECT_OACP_IND_CFG]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_client_config_uuid, ESP_GATT_PERM_WRITE,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, 0, NULL}},


    /* Object OLCP Characteristic Declaration */
    [OPT_IDX_CHAR_OBJECT_OLCP]     =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&char_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_write_indicate}},

    /* Object OLCP Characteristic Value */
    [OPT_IDX_CHAR_OBJECT_OLCP_VAL] =
    {{ESP_GATT_RSP_BY_APP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_OBJECT_OLCP, ESP_GATT_PERM_WRITE,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, 0, NULL}},

    /* Object OLCP Client Characteristic Configuration Descriptor */
    [OPT_IDX_CHAR_OBJECT_OLCP_IND_CFG]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_client_config_uuid, ESP_GATT_PERM_WRITE,
      sizeof(uint16_t),  0, NULL}},


    /* Object List Filter Characteristic Declaration */
    [OPT_IDX_CHAR_OBJECT_LIST_FILTER]     =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&char_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_write}},

    /* Object List Filter Characteristic Value */
    [OPT_IDX_CHAR_OBJECT_LIST_FILTER_VAL] =
    {{ESP_GATT_RSP_BY_APP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_OBJECT_LIST_FILTER, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, 0, NULL}},

    /* Object Alarm Action Characteristic Declaration */
    [OPT_IDX_CHAR_OBJECT_ALARM_ACTION]     =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&char_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_write}},

    /* Object Alarm Action Characteristic Value */
    [OPT_IDX_CHAR_OBJECT_ALARM_ACTION_VAL] =
    {{ESP_GATT_RSP_BY_APP}, {ESP_UUID_LEN_128, GATTS_CHAR_ALARM_ACTION, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, 0, NULL}},
    
    // TBD
    // /* Object Ringstone Action Characteristic Declaration */
    // [OPT_IDX_CHAR_OBJECT_RINGTONE_ACTION]     =
    // {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&char_declaration_uuid, ESP_GATT_PERM_READ,
    //   CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_write}},

    // /* Object Ringstone Action Characteristic Value */
    // [OPT_IDX_CHAR_OBJECT_RINGTONE_ACTION_VAL] =
    // {{ESP_GATT_RSP_BY_APP}, {ESP_UUID_LEN_128, GATTS_CHAR_RINGTONE_ACTION, ESP_GATT_PERM_WRITE,
    //   GATTS_DEMO_CHAR_VAL_LEN_MAX, 0, NULL}},

    // /* Object Ringstone Action Characteristic Configuration Descriptor */
    // [OPT_IDX_CHAR_OBJECT_RINGTONE_ACTION_CFG]  =
    // {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_client_config_uuid, ESP_GATT_PERM_WRITE,
    //   sizeof(uint16_t),  0, NULL}},

    /* Object Wifi Action Characteristic Declaration */
    [OPT_IDX_CHAR_OBJECT_WIFI_ACTION]     =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&char_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_write_indicate}},

    /* Object Wifi Action Characteristic Value */
    [OPT_IDX_CHAR_OBJECT_WIFI_ACTION_VAL] =
    {{ESP_GATT_RSP_BY_APP}, {ESP_UUID_LEN_128, GATTS_CHAR_WIFI_ACTION, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, 0, NULL}},

    /* Object Wifi Action Characteristic Configuration Descriptor */
    [OPT_IDX_CHAR_OBJECT_WIFI_ACTION_CFG]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_client_config_uuid, ESP_GATT_PERM_WRITE,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, 0, NULL}}
};

/* One gatt-based profile one app_id and one gatts_if, this array will store the gatts_if returned by ESP_GATTS_REG_EVT */
static struct gatts_profile_inst OPT_profile_tab[PROFILE_NUM] = {
    [PROFILE_APP_IDX] = {
        .gatts_cb = gatts_profile_event_handler,
        .gatts_if = ESP_GATT_IF_NONE,       /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
    },
};

/* Extended Advertising Descriptor */
static esp_ble_gap_ext_adv_t ext_adv = {EXT_ADV_HANDLE, EXT_ADV_DURATION, EXT_ADV_MAX_EVENTS};

/* Extended Advertising Parameters */
static esp_ble_gap_ext_adv_params_t ext_adv_params = {
    .type = ESP_BLE_GAP_SET_EXT_ADV_PROP_CONNECTABLE,
    .interval_min = 0x20,
    .interval_max = 0x20,
    .channel_map = ADV_CHNL_ALL,
    .filter_policy = ADV_FILTER_ALLOW_SCAN_WLST_CON_ANY,
    .primary_phy = ESP_BLE_GAP_PHY_1M,
    .max_skip = 0,
    .secondary_phy = ESP_BLE_GAP_PHY_2M,
    .sid = 0,
    .scan_req_notif = false,
    .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
    .peer_addr_type = BLE_ADDR_TYPE_PUBLIC,
    .tx_power = EXT_ADV_TX_PWR_NO_PREFERENCE,
};

/* Extended Advertising Raw Data */
static uint8_t ext_adv_raw_data[] = {
    0x02, 0x01, 0x06,
    0x02, 0x0a, 0xeb, 0x03, 0x03, 0xab, 0xcd,
    0x0a, 0X09, 'N', 'I', 'X', 'I', 'E', ' ', 'B', '1', '6',
};

/* OTP handle table */
static uint16_t OPT_HANDLE_TABLE[OPT_IDX_NB];


/* Functions */

/** @brief pp_bluetooth_init: Initializes and sets bluetooth functionality
 * 
 * @return
 */
void pp_bluetooth_init(void)
{
    ESP_LOGI(BLUETOOTH_TAG, "Initializing bluetooth");

    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));
    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_BLE));
    ESP_ERROR_CHECK(esp_bluedroid_init());
    ESP_ERROR_CHECK(esp_bluedroid_enable());
    ESP_ERROR_CHECK(esp_ble_gatts_register_callback(gatts_event_handler));
    ESP_ERROR_CHECK(esp_ble_gap_register_callback(gap_event_handler));
    ESP_ERROR_CHECK(esp_ble_gatts_app_register(ESP_APP_ID));
    ESP_ERROR_CHECK(esp_ble_gatt_set_local_mtu(MTU_SIZE));

    /* set the security iocap & auth_req & key size & init key response key parameters to the stack*/
    esp_ble_auth_req_t auth_req = ESP_LE_AUTH_REQ_SC_MITM_BOND;     //bonding with peer device after authentication
    esp_ble_io_cap_t iocap = ESP_IO_CAP_OUT;                        //set the IO capability to No output No input
    uint8_t key_size = 16;                                          //the key size should be 7~16 bytes
    uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    uint8_t rsp_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    uint8_t auth_option = ESP_BLE_ONLY_ACCEPT_SPECIFIED_AUTH_DISABLE;
    uint8_t oob_support = ESP_BLE_OOB_DISABLE;

    esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth_req, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_ONLY_ACCEPT_SPECIFIED_SEC_AUTH, &auth_option, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_OOB_SUPPORT, &oob_support, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &init_key, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &rsp_key, sizeof(uint8_t));
}

/** @brief gap_event_handler: GAP Event handler
 * 
 * @return
 */
static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event) 
    {
        case ESP_GAP_BLE_EXT_ADV_SET_PARAMS_COMPLETE_EVT:
        {
            ESP_LOGI(GATTS_TAG, "ESP_GAP_BLE_EXT_ADV_SET_PARAMS_COMPLETE_EVT status %d",  param->ext_adv_set_params.status);
            ESP_ERROR_CHECK(esp_ble_gap_config_ext_adv_data_raw(EXT_ADV_HANDLE,  sizeof(ext_adv_raw_data), &ext_adv_raw_data[0]));
            break;
        }
        
        case ESP_GAP_BLE_EXT_ADV_DATA_SET_COMPLETE_EVT:
        {
            ESP_LOGI(GATTS_TAG,"ESP_GAP_BLE_EXT_ADV_DATA_SET_COMPLETE_EVT status %d",  param->ext_adv_data_set.status);
            ESP_ERROR_CHECK(esp_ble_gap_ext_adv_start(NUM_EXT_ADV_SET, &ext_adv));
            break;
        }

        case ESP_GAP_BLE_ADV_TERMINATED_EVT:
        {
            ESP_LOGI(GATTS_TAG, "ESP_GAP_BLE_ADV_TERMINATED_EVT, status = %d", param->adv_terminate.status);
            if(param->adv_terminate.status == 0x00) 
            {
                ESP_LOGI(GATTS_TAG, "ADV successfully ended with a connection being created");
                uint32_t passkey = esp_random() / 4832 + 100000;    // /4295 to convert uint32 value to 0-999999 value
                esp_ble_gap_set_security_param(ESP_BLE_SM_SET_STATIC_PASSKEY, &passkey, sizeof(uint32_t));
                pp_set_display_passkey(passkey);
                device_mode = PAIRING_PASSKEY_MODE;

                display_update_type_t notif = NOTIFY_NORMAL_VAL;
                xQueueSend(display_update_queue, &notif, 10);
            }
            break;
        }

        case ESP_GAP_BLE_SEC_REQ_EVT:
        {
            ESP_LOGI(GATTS_TAG, "ESP_GAP_BLE_NC_REQ_EVT");
            /* send the positive(true) security response to the peer device to accept the security request.
            If not accept the security request, should send the security response with negative(false) accept value*/
            esp_ble_gap_security_rsp(param->ble_security.ble_req.bd_addr, true);
            break;
        }

        case ESP_GAP_BLE_PASSKEY_NOTIF_EVT:  ///the app will receive this evt when the IO has Output capability and the peer device IO has Input capability.
        {
            if (device_mode == PAIRING_MODE || device_mode == PAIRING_PASSKEY_MODE)
            {
                ESP_LOGI(GATTS_TAG, "The passkey Notify number: %06" PRIu32, param->ble_security.key_notif.passkey);
            }
            else
            {
                esp_ble_gap_disconnect(param->ble_security.key_notif.bd_addr);
            }
            
            break;
        }

        case ESP_GAP_BLE_KEY_EVT:
        {
            ESP_LOGI(GATTS_TAG, "ESP_GAP_BLE_KEY_EVT");
            //shows the ble key info share with peer device to the user.
            ESP_LOGI(GATTS_TAG, "key type = %d", (uint8_t)param->ble_security.ble_key.key_type);
            break;
        }

        case ESP_GAP_BLE_AUTH_CMPL_EVT: 
        {
            ESP_LOGI(GATTS_TAG, "ESP_GAP_BLE_AUTH_CMPL_EVT");
            esp_bd_addr_t bd_addr;
            memcpy(bd_addr, param->ble_security.auth_cmpl.bd_addr, sizeof(esp_bd_addr_t));
            ESP_LOGI(GATTS_TAG, "remote BD_ADDR: %08x%04x",\
                    (bd_addr[0] << 24) + (bd_addr[1] << 16) + (bd_addr[2] << 8) + bd_addr[3],
                    (bd_addr[4] << 8) + bd_addr[5]);
            ESP_LOGI(GATTS_TAG, "address type = %d", param->ble_security.auth_cmpl.addr_type);
            ESP_LOGI(GATTS_TAG, "pair status = %s",param->ble_security.auth_cmpl.success ? "success" : "fail");

            if(!param->ble_security.auth_cmpl.success) 
            {
                ESP_LOGI(GATTS_TAG, "fail reason = 0x%x",param->ble_security.auth_cmpl.fail_reason);
            } 
            else 
            {
                ESP_LOGI(GATTS_TAG, "auth mode = %d",(uint8_t)param->ble_security.auth_cmpl.auth_mode);
            }

            if (param->ble_security.auth_cmpl.success && (device_mode == PAIRING_MODE || device_mode == PAIRING_PASSKEY_MODE))
            {
                device_mode = DEFAULT_MODE;
                display_update_type_t notif = NOTIFY_NORMAL_VAL;
                xQueueSend(display_update_queue, &notif, 10);
            }
            
            break;
        }

        case ESP_GAP_BLE_REMOVE_BOND_DEV_COMPLETE_EVT: 
        {
            ESP_LOGD(GATTS_TAG, "ESP_GAP_BLE_REMOVE_BOND_DEV_COMPLETE_EVT status = %d", param->remove_bond_dev_cmpl.status);
            ESP_LOGI(GATTS_TAG, "ESP_GAP_BLE_REMOVE_BOND_DEV");
            ESP_LOGI(GATTS_TAG, "-----ESP_GAP_BLE_REMOVE_BOND_DEV----");
            ESP_LOG_BUFFER_HEX(GATTS_TAG, (void *)param->remove_bond_dev_cmpl.bd_addr, sizeof(esp_bd_addr_t));
            ESP_LOGI(GATTS_TAG, "------------------------------------");
            break;
        }

        case ESP_GAP_BLE_SET_LOCAL_PRIVACY_COMPLETE_EVT:
        {
            ESP_LOGI(GATTS_TAG, "ESP_GAP_BLE_SET_LOCAL_PRIVACY_COMPLETE_EVT, tatus = %x", param->local_privacy_cmpl.status);
            esp_ble_gap_ext_adv_set_params(EXT_ADV_HANDLE, &ext_adv_params);
            break;
        } 

        default:
            break;
    }
}

/** @brief gatts_profile_event_handler: GATTS Profile Event handler
 * 
 * @return
 */
static void gatts_profile_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    switch (event) 
    {
        case ESP_GATTS_REG_EVT:
        {
            ESP_ERROR_CHECK(esp_ble_gap_set_device_name(DEVICE_NAME));
            ESP_ERROR_CHECK(esp_ble_gap_config_local_privacy(true));
            ESP_ERROR_CHECK(esp_ble_gatts_create_attr_tab(gatt_db, gatts_if, OPT_IDX_NB, SVC_INST_ID));
            break;
        }
        case ESP_GATTS_READ_EVT:
        {
            ESP_LOGI(GATTS_TAG, "ESP_GATTS_READ_EVT");

            esp_gatt_rsp_t rsp;
            esp_gatt_status_t status = ESP_GATT_OK;

            if(param->read.handle < OPT_IDX_NB)
            {
                ESP_LOGI(GATTS_TAG, "ESP_GATTS_READ_EVT: OTP");
                status = pp_object_transfer_read_event(param->read.handle, OPT_HANDLE_TABLE, &rsp);

                if(param->read.need_rsp)
                {
                    esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id, status, &rsp);
                }
            }

       	    break;
        }
        case ESP_GATTS_WRITE_EVT:
        {
            ESP_LOGI(GATTS_TAG, "ESP_GATTS_WRITE_EVT");

            esp_gatt_rsp_t otp_rsp;
            esp_gatt_status_t status = ESP_GATT_OK;

            if(param->write.handle < OPT_IDX_NB)
            {
                ESP_LOGI(GATTS_TAG, "ESP_GATTS_WRITE_EVT: OTP");
                otp_write_attr_t write_params;
                write_params.need_attr_set = false;
                write_params.length = 0;
                write_params.need_ind = false;

                pp_object_transfer_write_event(param, OPT_HANDLE_TABLE, &write_params, &otp_rsp);

                if(write_params.need_attr_set)
                {
                    ESP_ERROR_CHECK(esp_ble_gatts_set_attr_value(param->write.handle, write_params.length, write_params.value));
                }
                
                if(param->write.need_rsp)
                {
                    esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, status, &otp_rsp);
                }

                if(write_params.need_ind)
                {
                    pp_object_transfer_write_event_indication(param, OPT_HANDLE_TABLE, &write_params);
                    esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, param->write.handle, write_params.length, write_params.value, true);
                }
            }
      	    break;
        }
        case ESP_GATTS_CONNECT_EVT:
        {
            ESP_LOGI(GATTS_TAG, "ESP_GATTS_CONNECT_EVT, conn_id = %d", param->connect.conn_id);
            esp_ble_set_encryption(param->connect.remote_bda, ESP_BLE_SEC_ENCRYPT_MITM);
            gpio_set_level(GPIO_OUTPUT_BLUE, 1);
            break;
        }
        case ESP_GATTS_DISCONNECT_EVT:
        {
            ESP_LOGI(GATTS_TAG, "ESP_GATTS_DISCONNECT_EVT, reason = 0x%x", param->disconnect.reason);
            esp_ble_gap_ext_adv_start(NUM_EXT_ADV_SET, &ext_adv);
            gpio_set_level(GPIO_OUTPUT_BLUE, 0);
            break;
        }
        case ESP_GATTS_CREAT_ATTR_TAB_EVT:
        {
            if (param->add_attr_tab.status != ESP_GATT_OK)
            {
                ESP_LOGE(GATTS_TAG, "create attribute table failed, error code=0x%x", param->add_attr_tab.status);
            }
            else if (param->add_attr_tab.num_handle != OPT_IDX_NB)
            {
                ESP_LOGE(GATTS_TAG, "create attribute table abnormally, num_handle (%d) \
                        doesn't equal to HRS_IDX_NB(%d)", param->add_attr_tab.num_handle, OPT_IDX_NB);
            }
            else 
            {
                ESP_LOGI(GATTS_TAG, "create attribute table successfully, the number handle = %d\n",param->add_attr_tab.num_handle);
                memcpy(OPT_HANDLE_TABLE, param->add_attr_tab.handles, sizeof(OPT_HANDLE_TABLE));
                esp_ble_gatts_start_service(OPT_HANDLE_TABLE[OPT_IDX_SVC]);
            }
            break;
        }

        default:
            break;
    }
}

/** @brief gatts_event_handler: GATTS Event handler
 * 
 * @return
 */
static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    /* If event is register event, store the gatts_if for each profile */
    if(event == ESP_GATTS_REG_EVT)
    {
        if (param->reg.status == ESP_GATT_OK) 
        {
            OPT_profile_tab[PROFILE_APP_IDX].gatts_if = gatts_if;
        } 
        else 
        {
            ESP_LOGE(GATTS_TAG, "reg app failed, app_id %04x, status %d", param->reg.app_id, param->reg.status);
            return;
        }
    }

    if (gatts_if == ESP_GATT_IF_NONE || gatts_if == OPT_profile_tab[PROFILE_APP_IDX].gatts_if) 
    {
        if (OPT_profile_tab[PROFILE_APP_IDX].gatts_cb) 
        {
            OPT_profile_tab[PROFILE_APP_IDX].gatts_cb(event, gatts_if, param);
        }
    }
}