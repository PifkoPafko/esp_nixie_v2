#ifndef __OBJECT_TRANSFER_METADATA_WRITE_H__
#define __OBJECT_TRANSFER_METADATA_WRITE_H__

#include "esp_err.h"
#include "esp_gatts_api.h"
#include "esp_wifi.h"

esp_err_t ObjectTranfer_metadata_write_event(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param, uint16_t *handle_table);
esp_err_t ObjectTransfer_send_found_wifi_ind(wifi_ap_record_t *wifi_record);
esp_err_t ObjectTransfer_send_simple_wifi_ind(uint8_t val);

#endif