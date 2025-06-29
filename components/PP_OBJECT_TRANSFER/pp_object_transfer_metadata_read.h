#ifndef __OBJECT_TRANSFER_METADATA_READ_H__
#define __OBJECT_TRANSFER_METADATA_READ_H__

#include "esp_gatts_api.h"
#include "esp_err.h"

esp_gatt_status_t pp_object_transfer_read_event(uint16_t handle, uint16_t *handle_table, esp_gatt_rsp_t *rsp);

#endif