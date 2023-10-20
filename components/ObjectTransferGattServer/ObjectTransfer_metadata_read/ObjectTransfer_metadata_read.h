#ifndef __OBJECT_TRANSFER_METADATA_READ_H__
#define __OBJECT_TRANSFER_METADATA_READ_H__

#include "esp_gatts_api.h"
#include "esp_err.h"

esp_err_t ObjectTranfer_metadata_read_event(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param, uint16_t *handle_table);

#endif