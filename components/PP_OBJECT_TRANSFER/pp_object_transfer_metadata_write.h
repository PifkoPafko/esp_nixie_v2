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

#ifndef __OBJECT_TRANSFER_METADATA_WRITE_H__
#define __OBJECT_TRANSFER_METADATA_WRITE_H__

/* Headers */
// #include "pp_object_manager.h"
// #include "pp_object_transfer_attr_ids.h"
// #include "pp_object_transfer_defs.h"
// #include "esp_err.h"
// #include "esp_gatts_api.h"
// #include "esp_log.h"
// #include "esp_wifi.h"
// #include "pp_wifi.h"
// #include "pp_alarm.h"

/* Structures */
typedef struct{
    bool need_attr_set = false,
    uint16_t length = 0,
    uint8_t value[32],
    bool need_ind = false
}otp_write_attr_t;

/* Function declarations */

/** @brief pp_object_transfer_write_event: Object Transfer Write handler function
 * 
 * This function should be called when Bluetooth GATT Profile got a write request for Object Transfer profile.
 * This function, based on input argument, recognize which characteristic the request is for
 * and prepares, takes action based on the type and prepares a response data.
 * 
 * Support OTP characteristics:
 *  -   Object Name             - name of the current object
 *  -   Object Type             - type of the current object
 *  -   Object Size             - size of the current object
 *  -   Object ID               - ID of the current object
 *  -   Object Properties       - properties of the current object
 *  -   Object List Filter      - Current filter option
 *  -   Object Alarm Action     - Alarm properties of the current object
 *  -   Object Wifi Action      - Wifi properties of the device
 *
 * @param[in]   handle        (uint16_t) Characteristic handle
 * @param[in]   handle_table  (uint16_t*) Characteristics handle table
 * @param[out]  rsp           (esp_gatt_rsp_t*) Pointer to the GATT response data
 * 
 * @return  (esp_gatt_status_t) GATT status of the operation.
 */
esp_gatt_status_t pp_object_transfer_write_event(esp_ble_gatts_cb_param_t *param, uint16_t *handle_table, otp_write_attr_t *write_params, esp_gatt_rsp_t *rsp)

/** @brief pp_object_transfer_write_event_indication: Object Transfer Write Indication function
 * 
 * This function should be called when Bluetooth GATT Profile got a write request for OTP that needs additional
 * indication to be send. This function, based on input arguments, recognize which characteristic
 * the request is for, takes action based on the operation type and prepares a response data for indication.
 * 
 * Support operation types:
 *  -   Object OACP
 *      -   OACP_OP_CODE_CREATE - Create new object and set it as current
 *      -   OACP_OP_CODE_DELETE - Delete current object
 * 
 *  -   Object OLCP
 *      -   OLCP_OP_CODE_FIRST          - Sets the first object as current
 *      -   OLCP_OP_CODE_LAST           - Sets the last object as current
 *      -   OLCP_OP_CODE_PREVIOUS       - Sets the previous object as current
 *      -   OLCP_OP_CODE_NEXT           - Sets the next object as current
 *      -   OLCP_OP_CODE_GOTO           - Sets the requested by ID object as current
 *      -   OLCP_OP_CODE_ORDER          - Sets order of the objects
 *      -   OLCP_OP_CODE_REQ_NUM_OF_OBJ - Provides number of objects
 *      -   OLCP_OP_CODE_CLEAR_MARING   - Clear markings on objects
 *
 * @param[in]   param           (esp_ble_gatts_cb_param_t *) GATT handler parameter
 * @param[in]   handle_table    (uint16_t) Characteristics handle table
 * @param[out]  write_params    (otp_write_attr_t*) Reponse data
 * 
 * @return
 */
void pp_object_transfer_write_event_indication(esp_ble_gatts_cb_param_t *param, uint16_t *handle_table, otp_write_attr_t *write_params);

/** @brief pp_object_transfer_send_found_wifi_ind: Sends found WiFi networks by indication
 * 
 * This function should sends indication to connected Bluetooth device with discovered WiFi
 * network information.
 *
 * @param[in]   wifi_record     (wifi_ap_record_t*) Pointer to discovered WiFi network record
 * 
 * @return
 */
void pp_object_transfer_send_found_wifi_ind(wifi_ap_record_t *wifi_record);

/** @brief pp_object_transfer_send_found_wifi_ind: Sends found WiFi networks by indication
 * 
 * This function should sends indication to connected Bluetooth device with information
 * about current WIFI state.
 * 
 *  WiFi status description:
 *      - WIFI_SEARCH_END:      Discovering of WiFi network ended
 *      - WIFI_RESERVED:        RESERVED
 *      - WIFI_CONNECTED:       Got IP/Connected
 *      - WIFI_DISCONNECTED:    Disconnected
 * @param[in]   type    (uint8_t*) Pointer to discovered WiFi network record
 * 
 * @return
 */
void pp_object_transfer_send_simple_wifi_ind(my_wifi_status_t type);

#endif