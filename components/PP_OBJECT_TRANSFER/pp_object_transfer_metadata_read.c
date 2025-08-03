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
#include "pp_object_transfer_metadata_read.h"

/* Macros */
#define TAG "READ_EVENT"

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
esp_gatt_status_t pp_object_transfer_read_event(uint16_t handle, uint16_t *handle_table, esp_gatt_rsp_t *rsp)
{
    otp_rsp_status_t rsp_status = STATUS_OK;

    if(handle == handle_table[OPT_IDX_CHAR_OBJECT_NAME_VAL])
    {
        do {
            ESP_LOGI(TAG, "Object Name READ EVENT");
            object_t* object = pp_object_manager_get_object();
            if(object == NULL)
            {
                ESP_LOGE(TAG, "Object not selected");
                rsp->handle = handle;
                rsp_status = ERROR_OBJECT_NOT_SELECTED;
                break;
            }

            memcpy(rsp->attr_value.value, object->name, object->name_len);
            rsp->attr_value.handle = handle;
            rsp->attr_value.offset = 0;
            rsp->attr_value.len = object->name_len;
            rsp->attr_value.auth_req = ESP_GATT_AUTH_REQ_NONE;
            rsp_status = STATUS_OK;

        } while(0);
    }
    else if(handle == handle_table[OPT_IDX_CHAR_OBJECT_TYPE_VAL])
    {
        do {
            ESP_LOGI(TAG, "Object Type READ EVENT");
            object_t *object = pp_object_manager_get_object();
            if(object == NULL)
            {
                ESP_LOGE(TAG, "Object not selected");
                rsp->handle = handle;
                rsp_status = ERROR_OBJECT_NOT_SELECTED;
                break;
            }

            memcpy(rsp->attr_value.value, &object->type.uuid.uuid16, object->type.len);
            rsp->attr_value.handle = handle;
            rsp->attr_value.offset = 0;
            rsp->attr_value.len = object->type.len;
            rsp->attr_value.auth_req = ESP_GATT_AUTH_REQ_NONE;
            rsp_status = STATUS_OK;

        } while(0);
    }
    else if(handle == handle_table[OPT_IDX_CHAR_OBJECT_SIZE_VAL])
    {
        do {
            ESP_LOGI(TAG, "Object Size READ EVENT");
            object_t *object = pp_object_manager_get_object();
            if(object == NULL)
            {
                ESP_LOGE(TAG, "Object not selected");
                rsp->handle = handle;
                rsp_status = ERROR_OBJECT_NOT_SELECTED;
                break;
            }

            uint8_t object_size[8];
            memcpy(object_size, &object->size, 4);
            memcpy(&object_size[4], &object->alloc_size, 4);
            memcpy(rsp->attr_value.value, object_size, 8);
            rsp->attr_value.handle = handle;
            rsp->attr_value.offset = 0;
            rsp->attr_value.len = 8;
            rsp->attr_value.auth_req = ESP_GATT_AUTH_REQ_NONE;
            rsp_status = STATUS_OK;

        } while(0);
    }
    else if(handle == handle_table[OPT_IDX_CHAR_OBJECT_ID_VAL])
    {
        do {
            ESP_LOGI(TAG, "Object ID READ EVENT");
            object_t* object = pp_object_manager_get_object();
            if(object == NULL)
            {
                ESP_LOGE(TAG, "Object not selected");
                rsp->handle = handle;
                rsp_status = ERROR_OBJECT_NOT_SELECTED;
                break;
            }

            ESP_LOGI(TAG, "ID: %llx", object->id);
            memcpy(rsp->attr_value.value, &object->id, 6);
            rsp->attr_value.handle = handle;
            rsp->attr_value.offset = 0;
            rsp->attr_value.len = 6;
            rsp->attr_value.auth_req = ESP_GATT_AUTH_REQ_NONE;
            rsp_status = STATUS_OK;

        } while(0);
    }
    else if(handle == handle_table[OPT_IDX_CHAR_OBJECT_PROPERTIES_VAL])
    {
        do {
            ESP_LOGI(TAG, "Object Properties READ EVENT");
            object_t* object = pp_object_manager_get_object();
            if(object == NULL)
            {
                ESP_LOGE(TAG, "Object not selected");
                rsp->handle = handle;
                rsp_status = ERROR_OBJECT_NOT_SELECTED;
                break;
            }

            memcpy(rsp->attr_value.value, &object->properties, 4);
            rsp->attr_value.handle = handle;
            rsp->attr_value.offset = 0;
            rsp->attr_value.len = 4;
            rsp->attr_value.auth_req = ESP_GATT_AUTH_REQ_NONE;
            rsp_status = STATUS_OK;

        } while(0);
    }
    else if(handle == handle_table[OPT_IDX_CHAR_OBJECT_LIST_FILTER_VAL])
    {
        do {
            ESP_LOGD(TAG, "Object List Filter READ EVENT");
            ListFilter_t *filter = pp_object_list_get_filter();
            rsp->attr_value.value[0] = filter->type;
            memcpy(&rsp->attr_value.value[1], filter->parameter, filter->par_length);
            rsp->attr_value.handle = handle;
            rsp->attr_value.offset = 0;
            rsp->attr_value.len = filter->par_length + 1;
            rsp->attr_value.auth_req = ESP_GATT_AUTH_REQ_NONE;
            rsp_status = STATUS_OK;

        } while(0);
    }
    else if(handle == handle_table[OPT_IDX_CHAR_OBJECT_ALARM_ACTION_VAL])
    {
        do {
            ESP_LOGI(TAG, "Object alarm data READ EVENT");

            object_t* object = pp_object_manager_get_object();
            if(object == NULL)
            {
                ESP_LOGE(TAG, "Object not selected");
                rsp->handle = handle;
                rsp_status = ERROR_OBJECT_NOT_SELECTED;
                break;
            }

            if(pp_object_manager_check_type(object->type.uuid.uuid128) != ALARM_TYPE)
            {
                ESP_LOGE(TAG, "Wrong type - required: Alarm");
                rsp->handle = handle;
                rsp_status = WRITE_REQUEST_REJECTED;
                break;
            }

            if(object->set_custom_object == false)
            {
                ESP_LOGE(TAG, "Alarm is not configured");
                rsp->handle = handle;
                rsp_status = ALARM_NOT_CONFIGURED;
                break;
            }

            uint8_t *payload = rsp->attr_value.value;

            memcpy(payload, &current_alarm.mode, ALARM_FIELD_SIZE);
            payload += ALARM_FIELD_SIZE;

            memcpy(payload, &current_alarm.enable, ALARM_FIELD_SIZE);
            payload += ALARM_FIELD_SIZE;

            memcpy(payload, &current_alarm.desc_len, ALARM_FIELD_SIZE);
            payload += ALARM_FIELD_SIZE;

            memcpy(payload, (uint8_t*)&current_alarm.desc, current_alarm.desc_len);
            payload += current_alarm.desc_len;

            memcpy(payload, &current_alarm.hour, ALARM_FIELD_SIZE);
            payload += ALARM_FIELD_SIZE;

            memcpy(payload, &current_alarm.minute, ALARM_FIELD_SIZE);
            payload += ALARM_FIELD_SIZE;

            switch(current_alarm.mode)
            {
                case ALARM_SINGLE_MODE:
                {
                    memcpy(payload, &current_alarm.args.single_alarm_args.day, ALARM_FIELD_SIZE);
                    payload += ALARM_FIELD_SIZE;

                    memcpy(payload, &current_alarm.args.single_alarm_args.month, ALARM_FIELD_SIZE);
                    payload += ALARM_FIELD_SIZE;

                    memcpy(payload, &current_alarm.args.single_alarm_args.year, ALARM_FIELD_SIZE);
                    payload += ALARM_FIELD_SIZE;

                    rsp->attr_value.len = ALARM_MODE_SINGLE_PAYLOAD_SIZE_MIN + current_alarm.desc_len;
                    break;
                }
                    

                case ALARM_WEEKLY_MODE:
                {
                    memcpy(payload, &current_alarm.args.days, ALARM_FIELD_SIZE);
                    payload += ALARM_FIELD_SIZE;

                    rsp->attr_value.len = ALARM_MODE_WEEKLY_PAYLOAD_SIZE_MIN + current_alarm.desc_len;
                    break;
                }
                    

                case ALARM_MONTHLY_MODE:
                {
                    memcpy(payload, &current_alarm.args.day, ALARM_FIELD_SIZE);
                    payload += ALARM_FIELD_SIZE;

                    rsp->attr_value.len = ALARM_MODE_MONTHLY_PAYLOAD_SIZE_MIN + current_alarm.desc_len;
                    break;
                }
                    

                case ALARM_YEARLY_MODE:
                {
                    memcpy(payload, &current_alarm.args.yearly_alarm_args.day, ALARM_FIELD_SIZE);
                    payload += ALARM_FIELD_SIZE;

                    memcpy(payload, &current_alarm.args.yearly_alarm_args.month, ALARM_FIELD_SIZE);
                    payload += ALARM_FIELD_SIZE;

                    rsp->attr_value.len = ALARM_MODE_YEARLY_PAYLOAD_SIZE_MIN + current_alarm.desc_len;
                    break;
                }
            }
            
            memcpy(payload, &current_alarm.volume, ALARM_FIELD_SIZE);
            payload += ALARM_FIELD_SIZE;

            rsp->attr_value.handle = handle;
            rsp->attr_value.offset = 0;
            rsp->attr_value.auth_req = ESP_GATT_AUTH_REQ_NONE;
            rsp_status = STATUS_OK;
        } while(0);
    }
    else if(handle == handle_table[OPT_IDX_CHAR_OBJECT_WIFI_ACTION_VAL])
    {
        do {
            ESP_LOGI(TAG, "Object wifi data READ EVENT");
            rsp->attr_value.handle = handle;

            my_wifi_t* my_wifi = pp_get_current_wifi();

            if(pp_get_wifi_connect_status())
            {
                rsp->attr_value.value[0] = 1;
            }
            else
            {
                rsp->attr_value.value[0] = 0;
            }

            rsp->attr_value.value[1] = my_wifi->my_ssid_len;
            memcpy(&rsp->attr_value.value[2], my_wifi->wifi_config.sta.ssid, my_wifi->my_ssid_len);
            rsp->attr_value.len = 2 + my_wifi->my_ssid_len;

            rsp->attr_value.offset = 0;
            rsp->attr_value.auth_req = ESP_GATT_AUTH_REQ_NONE;

            rsp_status = STATUS_OK;

        } while(0);
    }

    return (esp_gatt_status_t)rsp_status;
}