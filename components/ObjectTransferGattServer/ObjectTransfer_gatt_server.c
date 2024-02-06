#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_bt.h"
#include "esp_gatt_common_api.h"
#include "esp_log.h"
#include "esp_random.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "ObjectTransfer_gatt_server.h"
#include "ObjectTransfer_attr_ids.h"
#include "ObjectTransfer_metadata_read.h"
#include "ObjectTransfer_metadata_write.h"
#include "ObjectManager.h"
#include "pp_nixie_display.h"


#define GATTS_TAG "GATTS"

#define PROFILE_NUM                 1
#define PROFILE_APP_IDX             0
#define SAMPLE_DEVICE_NAME          "DEFINITELY_NOT_ESP"
#define SVC_INST_ID                 0

/* The max length of characteristic value. When the GATT client performs a write or prepare write operation,
*  the data length must be less than GATTS_DEMO_CHAR_VAL_LEN_MAX.
*/
#define GATTS_DEMO_CHAR_VAL_LEN_MAX 500
#define PREPARE_BUF_MAX_SIZE        1024
#define CHAR_DECLARATION_SIZE       (sizeof(uint8_t))

#define EXT_ADV_HANDLE              0
#define NUM_EXT_ADV_SET             1
#define EXT_ADV_DURATION            0
#define EXT_ADV_MAX_EVENTS          0

static uint8_t ext_adv_raw_data[] = {
        0x02, 0x01, 0x06,
        0x02, 0x0a, 0xeb, 0x03, 0x03, 0xab, 0xcd,
        0x11, 0X09, 'E', 'S', 'P', '_', 'B', 'L', 'E', '5', '0', '_', 'S', 'E', 'R', 'V', 'E', 'R',
};

uint16_t OPT_handle_table[OPT_IDX_NB];

esp_ble_gap_ext_adv_params_t ext_adv_params = {
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

static esp_ble_gap_ext_adv_t ext_adv[1] = {
    [0] = {EXT_ADV_HANDLE, EXT_ADV_DURATION, EXT_ADV_MAX_EVENTS},
};

struct gatts_profile_inst {
    esp_gatts_cb_t gatts_cb;
    uint16_t gatts_if;
    uint16_t app_id;
    uint16_t conn_id;
    uint16_t service_handle;
    esp_gatt_srvc_id_t service_id;
    uint16_t char_handle;
    esp_bt_uuid_t char_uuid;
    esp_gatt_perm_t perm;
    esp_gatt_char_prop_t property;
    uint16_t descr_handle;
    esp_bt_uuid_t descr_uuid;
};

void gatts_profile_event_handler(esp_gatts_cb_event_t event,
					esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);

/* One gatt-based profile one app_id and one gatts_if, this array will store the gatts_if returned by ESP_GATTS_REG_EVT */
static struct gatts_profile_inst OPT_profile_tab[PROFILE_NUM] = {
    [PROFILE_APP_IDX] = {
        .gatts_cb = gatts_profile_event_handler,
        .gatts_if = ESP_GATT_IF_NONE,       /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
    },
};

/* Service */
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
// static uint8_t GATTS_CHAR_RINGTONE_ACTION[16]           = {0x26, 0xab, 0x57, 0xe0, 0x57, 0xab, 0x45, 0x98, 0xaf, 0xf2, 0x06, 0xe5, 0x27, 0x5c, 0xe5, 0x40};
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

static char *esp_key_type_to_str(esp_ble_key_type_t key_type)
{
   char *key_str = NULL;
   switch(key_type) {
    case ESP_LE_KEY_NONE:
        key_str = "ESP_LE_KEY_NONE";
        break;
    case ESP_LE_KEY_PENC:
        key_str = "ESP_LE_KEY_PENC";
        break;
    case ESP_LE_KEY_PID:
        key_str = "ESP_LE_KEY_PID";
        break;
    case ESP_LE_KEY_PCSRK:
        key_str = "ESP_LE_KEY_PCSRK";
        break;
    case ESP_LE_KEY_PLK:
        key_str = "ESP_LE_KEY_PLK";
        break;
    case ESP_LE_KEY_LLK:
        key_str = "ESP_LE_KEY_LLK";
        break;
    case ESP_LE_KEY_LENC:
        key_str = "ESP_LE_KEY_LENC";
        break;
    case ESP_LE_KEY_LID:
        key_str = "ESP_LE_KEY_LID";
        break;
    case ESP_LE_KEY_LCSRK:
        key_str = "ESP_LE_KEY_LCSRK";
        break;
    default:
        key_str = "INVALID BLE KEY TYPE";
        break;
   }

   return key_str;
}

static char *esp_auth_req_to_str(esp_ble_auth_req_t auth_req)
{
   char *auth_str = NULL;
   switch(auth_req) {
    case ESP_LE_AUTH_NO_BOND:
        auth_str = "ESP_LE_AUTH_NO_BOND";
        break;
    case ESP_LE_AUTH_BOND:
        auth_str = "ESP_LE_AUTH_BOND";
        break;
    case ESP_LE_AUTH_REQ_MITM:
        auth_str = "ESP_LE_AUTH_REQ_MITM";
        break;
    case ESP_LE_AUTH_REQ_BOND_MITM:
        auth_str = "ESP_LE_AUTH_REQ_BOND_MITM";
        break;
    case ESP_LE_AUTH_REQ_SC_ONLY:
        auth_str = "ESP_LE_AUTH_REQ_SC_ONLY";
        break;
    case ESP_LE_AUTH_REQ_SC_BOND:
        auth_str = "ESP_LE_AUTH_REQ_SC_BOND";
        break;
    case ESP_LE_AUTH_REQ_SC_MITM:
        auth_str = "ESP_LE_AUTH_REQ_SC_MITM";
        break;
    case ESP_LE_AUTH_REQ_SC_MITM_BOND:
        auth_str = "ESP_LE_AUTH_REQ_SC_MITM_BOND";
        break;
    default:
        auth_str = "INVALID BLE AUTH REQ";
        break;
   }

   return auth_str;
}

void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event) 
    {
        case ESP_GAP_BLE_EXT_ADV_SET_PARAMS_COMPLETE_EVT:
        {
            ESP_LOGI(GATTS_TAG, "ESP_GAP_BLE_EXT_ADV_SET_PARAMS_COMPLETE_EVT status %d",  param->ext_adv_set_params.status);
            esp_ble_gap_config_ext_adv_data_raw(EXT_ADV_HANDLE,  sizeof(ext_adv_raw_data), &ext_adv_raw_data[0]);
            break;
        }
        case ESP_GAP_BLE_EXT_ADV_DATA_SET_COMPLETE_EVT:
        {
            ESP_LOGI(GATTS_TAG,"ESP_GAP_BLE_EXT_ADV_DATA_SET_COMPLETE_EVT status %d",  param->ext_adv_data_set.status);
            esp_ble_gap_ext_adv_start(NUM_EXT_ADV_SET, &ext_adv[0]);
            break;
        }
        case ESP_GAP_BLE_EXT_ADV_START_COMPLETE_EVT:
        {
            ESP_LOGI(GATTS_TAG, "ESP_GAP_BLE_EXT_ADV_START_COMPLETE_EVT, status = %d", param->ext_adv_data_set.status);
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
                set_display_passkey(passkey);
            }
            break;
        }
        case ESP_GAP_BLE_PASSKEY_REQ_EVT:                           /* passkey request event */
        {
            ESP_LOGI(GATTS_TAG, "ESP_GAP_BLE_PASSKEY_REQ_EVT");
            /* Call the following function to input the passkey which is displayed on the remote device */
            //esp_ble_passkey_reply(heart_rate_profile_tab[HEART_PROFILE_APP_IDX].remote_bda, true, 0x00);
            break;
        }
        case ESP_GAP_BLE_OOB_REQ_EVT:
        {
            ESP_LOGI(GATTS_TAG, "ESP_GAP_BLE_OOB_REQ_EVT");
            // uint8_t tk[16] = {1}; //If you paired with OOB, both devices need to use the same tk
            // esp_ble_oob_req_reply(param->ble_security.ble_req.bd_addr, tk, sizeof(tk));
            break;
        }
        case ESP_GAP_BLE_LOCAL_IR_EVT:                             /* BLE local IR event */
        {
            ESP_LOGI(GATTS_TAG, "ESP_GAP_BLE_LOCAL_IR_EVT");
            break;
        }
        case ESP_GAP_BLE_LOCAL_ER_EVT:                               /* BLE local ER event */
        {
            ESP_LOGI(GATTS_TAG, "ESP_GAP_BLE_LOCAL_ER_EVT");
            break;
        }
        // case ESP_GAP_BLE_NC_REQ_EVT:
        // {
        //     ESP_LOGI(GATTS_TAG, "ESP_GAP_BLE_NC_REQ_EVT");
        //     /* The app will receive this evt when the IO has DisplayYesNO capability and the peer device IO also has DisplayYesNo capability.
        //     show the passkey number to the user to confirm it with the number displayed by peer device. */
        //     esp_ble_confirm_reply(param->ble_security.ble_req.bd_addr, true);
        //     ESP_LOGI(GATTS_TAG, "ESP_GAP_BLE_NC_REQ_EVT, the passkey Notify number:%" PRIu32, param->ble_security.key_notif.passkey);
        //     break;
        // }
        case ESP_GAP_BLE_SEC_REQ_EVT:
        {
            ESP_LOGI(GATTS_TAG, "ESP_GAP_BLE_NC_REQ_EVT");
            /* send the positive(true) security response to the peer device to accept the security request.
            If not accept the security request, should send the security response with negative(false) accept value*/
            esp_ble_gap_security_rsp(param->ble_security.ble_req.bd_addr, true);
            break;
        } 
        case ESP_GAP_BLE_PASSKEY_NOTIF_EVT:  ///the app will receive this evt when the IO  has Output capability and the peer device IO has Input capability.
        {
            if (get_device_mode() == PAIRING_MODE)
            {
                set_insert_passkey_flag(true);
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
            ESP_LOGI(GATTS_TAG, "key type = %s", esp_key_type_to_str(param->ble_security.ble_key.key_type));
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
                ESP_LOGI(GATTS_TAG, "auth mode = %s",esp_auth_req_to_str(param->ble_security.auth_cmpl.auth_mode));
            }

            if (param->ble_security.auth_cmpl.success && get_device_mode() == PAIRING_MODE)
            {
                set_insert_passkey_flag(false);
                set_device_mode(DEFAULT_MODE);
            }
            
            break;
        }
        case ESP_GAP_BLE_REMOVE_BOND_DEV_COMPLETE_EVT: 
        {
            ESP_LOGD(GATTS_TAG, "ESP_GAP_BLE_REMOVE_BOND_DEV_COMPLETE_EVT status = %d", param->remove_bond_dev_cmpl.status);
            ESP_LOGI(GATTS_TAG, "ESP_GAP_BLE_REMOVE_BOND_DEV");
            ESP_LOGI(GATTS_TAG, "-----ESP_GAP_BLE_REMOVE_BOND_DEV----");
            esp_log_buffer_hex(GATTS_TAG, (void *)param->remove_bond_dev_cmpl.bd_addr, sizeof(esp_bd_addr_t));
            ESP_LOGI(GATTS_TAG, "------------------------------------");
            break;
        }
        case ESP_GAP_BLE_SET_LOCAL_PRIVACY_COMPLETE_EVT:
        {
            ESP_LOGI(GATTS_TAG, "ESP_GAP_BLE_SET_LOCAL_PRIVACY_COMPLETE_EVT, tatus = %x", param->local_privacy_cmpl.status);
            esp_ble_gap_ext_adv_set_params(EXT_ADV_HANDLE, &ext_adv_params);
            break;
        } 
        case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
        {
            ESP_LOGI(GATTS_TAG, "update connection params status = %d, min_int = %d, max_int = %d,conn_int = %d,latency = %d, timeout = %d",
                     param->update_conn_params.status,
                     param->update_conn_params.min_int,
                     param->update_conn_params.max_int,
                     param->update_conn_params.conn_int,
                     param->update_conn_params.latency,
                     param->update_conn_params.timeout);
            break;
        }
        default:
            break;
    }
}

void gatts_profile_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    switch (event) {
        case ESP_GATTS_REG_EVT:
        {
            esp_err_t set_dev_name_ret = esp_ble_gap_set_device_name(SAMPLE_DEVICE_NAME);

            if (set_dev_name_ret)
            {
                ESP_LOGE(GATTS_TAG, "set device name failed, error code = %x", set_dev_name_ret);
            }

            esp_ble_gap_config_local_privacy(true);

            esp_err_t create_attr_ret = esp_ble_gatts_create_attr_tab(gatt_db, gatts_if, OPT_IDX_NB, SVC_INST_ID);
            if (create_attr_ret){
                ESP_LOGE(GATTS_TAG, "create attr table failed, error code = %x", create_attr_ret);
            }
             break;
        }
        case ESP_GATTS_READ_EVT:
        {
            ESP_LOGD(GATTS_TAG, "ESP_GATTS_READ_EVT");
            ObjectTranfer_metadata_read_event(gatts_if, param, OPT_handle_table);
       	    break;
        }
        case ESP_GATTS_WRITE_EVT:
        {
            ESP_LOGD(GATTS_TAG, "ESP_GATTS_WRITE_EVT");
            ObjectTranfer_metadata_write_event(gatts_if, param, OPT_handle_table);
      	    break;
        }
        case ESP_GATTS_EXEC_WRITE_EVT:
        {
            // the length of gattc prepare write data must be less than GATTS_DEMO_CHAR_VAL_LEN_MAX.
            ESP_LOGD(GATTS_TAG, "ESP_GATTS_EXEC_WRITE_EVT");
            break;
        }
        case ESP_GATTS_MTU_EVT:
        {
            ESP_LOGD(GATTS_TAG, "ESP_GATTS_MTU_EVT, MTU %d", param->mtu.mtu);
            break;
        } 
        case ESP_GATTS_CONF_EVT:
        {
            ESP_LOGD(GATTS_TAG, "ESP_GATTS_CONF_EVT, status = %d, attr_handle %d", param->conf.status, param->conf.handle);
            break;
        }
        case ESP_GATTS_START_EVT:
        {
            ESP_LOGD(GATTS_TAG, "SERVICE_START_EVT, status %d, service_handle %d", param->start.status, param->start.service_handle);
            break;
        }
        case ESP_GATTS_CONNECT_EVT:
        {
            ESP_LOGD(GATTS_TAG, "ESP_GATTS_CONNECT_EVT, conn_id = %d", param->connect.conn_id);
            esp_ble_set_encryption(param->connect.remote_bda, ESP_BLE_SEC_ENCRYPT_MITM);
            break;
        }
        case ESP_GATTS_DISCONNECT_EVT:
        {
            ESP_LOGD(GATTS_TAG, "ESP_GATTS_DISCONNECT_EVT, reason = 0x%x", param->disconnect.reason);
            esp_ble_gap_ext_adv_start(NUM_EXT_ADV_SET, &ext_adv[0]);
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
                memcpy(OPT_handle_table, param->add_attr_tab.handles, sizeof(OPT_handle_table));
                esp_ble_gatts_start_service(OPT_handle_table[OPT_IDX_SVC]);
            }
            break;
        }
        case ESP_GATTS_STOP_EVT:
        case ESP_GATTS_OPEN_EVT:
        case ESP_GATTS_CANCEL_OPEN_EVT:
        case ESP_GATTS_CLOSE_EVT:
        case ESP_GATTS_LISTEN_EVT:
        case ESP_GATTS_CONGEST_EVT:
        case ESP_GATTS_UNREG_EVT:
        case ESP_GATTS_DELETE_EVT:
        default:
            break;
    }
}

void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{

    /* If event is register event, store the gatts_if for each profile */
    if (event == ESP_GATTS_REG_EVT) {
        if (param->reg.status == ESP_GATT_OK) {
            OPT_profile_tab[PROFILE_APP_IDX].gatts_if = gatts_if;
        } else {
            ESP_LOGE(GATTS_TAG, "reg app failed, app_id %04x, status %d",
                    param->reg.app_id,
                    param->reg.status);
            return;
        }
    }
    do {
        int idx;
        for (idx = 0; idx < PROFILE_NUM; idx++) {
            /* ESP_GATT_IF_NONE, not specify a certain gatt_if, need to call every profile cb function */
            if (gatts_if == ESP_GATT_IF_NONE || gatts_if == OPT_profile_tab[idx].gatts_if) {
                if (OPT_profile_tab[idx].gatts_cb) {
                    OPT_profile_tab[idx].gatts_cb(event, gatts_if, param);
                }
            }
        }
    } while (0);
}