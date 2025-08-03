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

#ifndef __OBJECT_TRANSFER_METADATA_READ_H__
#define __OBJECT_TRANSFER_METADATA_READ_H__

/* Headers */
// #include "pp_object_transfer_attr_ids.h"
// #include "pp_object_manager.h"
// #include "pp_object_transfer_defs.h"
// #include "esp_gatts_api.h"
// #include "esp_log.h"
// #include "pp_wifi.h"
// #include "esp_err.h"

/** @brief pp_object_transfer_read_event: Object Transfer Read handler function
 * 
 * This function should be called when Bluetooth GATT Profile got a read request for Object Transfer profile.
 * This function, based on input argument, recognize which characteristic the request is for
 * and prepares a response data.
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
esp_gatt_status_t pp_object_transfer_read_event(uint16_t handle, uint16_t *handle_table, esp_gatt_rsp_t *rsp);

#endif