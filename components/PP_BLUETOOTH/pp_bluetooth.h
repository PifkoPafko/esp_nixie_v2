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

#ifndef __PP_BLUETOOTH_H__
#define __PP_BLUETOOTH_H__
 
/* Headers */
#include "esp_gatts_api.h"
#include "esp_gap_ble_api.h"

// #include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_bt.h"
#include "esp_gatt_common_api.h"
#include "esp_log.h"
#include "esp_random.h"

// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"

// #include "pp_object_transfer_gatt_server.h"
#include "pp_object_transfer_attr_ids.h"
// #include "pp_object_transfer_metadata_read.h"
// #include "pp_object_transfer_metadata_write.h"
// #include "pp_object_manager.h"
// #include "pp_nixie_display.h"

/* Macros */
#define ESP_APP_ID     0x55
#define MTU_SIZE       512

#define PROFILE_NUM                 1
#define PROFILE_APP_IDX             0
#define DEVICE_NAME                 "NIXIE B16"
#define SVC_INST_ID                 0

#define GATTS_DEMO_CHAR_VAL_LEN_MAX 500
#define PREPARE_BUF_MAX_SIZE        1024
#define CHAR_DECLARATION_SIZE       (sizeof(uint8_t))

#define EXT_ADV_HANDLE              0
#define NUM_EXT_ADV_SET             1
#define EXT_ADV_DURATION            0
#define EXT_ADV_MAX_EVENTS          0

/* Structures */
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

/* Functions */

/** @brief pp_bluetooth_init: Initializes and sets bluetooth functionality
 * 
 * @return
 */
void pp_bluetooth_init(void);

#endif