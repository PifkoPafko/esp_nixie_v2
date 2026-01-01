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
#include "pp_object_transfer_metadata_write.h"

/* Macros */
#define TAG "WRITE_EVENT"

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
{
    uint16_t handle = param->write.handle;
    otp_rsp_status_t rsp_status = STATUS_OK;

    if(handle == handle_table[OPT_IDX_CHAR_OBJECT_NAME_VAL])
    {
        do {
            ESP_LOGI(TAG, "Object Name WRITE EVENT");
            if(pp_object_manager_is_object_empty())
            {
                ESP_LOGE(TAG, "Object not selected");
                rsp->handle = handle;
                rsp_status = ERROR_OBJECT_NOT_SELECTED;
                break;
            }

            object_t *object = pp_object_manager_get_object();

            if(param->write.len == 0)
            {
                ESP_LOGE(TAG, "The new name cannot be empty");
                rsp->handle = handle;
                rsp_status = INVALID_ATTR_VAL_LENGTH;
                break;
            }

            if(param->write.len > NAME_LEN_MAX || param->write.len == 0)
            {
                ESP_LOGE(TAG, "Wrong name length. Should be 1-%d", NAME_LEN_MAX);
                rsp->handle = handle;
                rsp_status = INVALID_ATTR_VAL_LENGTH;
                break;
            }

            strncpy(object->name, (const char*)param->write.value, NAME_LEN_MAX + 1);
            object->name_len = strlen(object->name);
            ESP_LOGI("WRITE", "Object name changed to '%s'", object->name);

            pp_object_manager_change_name_in_file();

            write_params->need_attr_set = true;
            write_params->length = object->name_len;
            memcpy(write_params->value, (uint8_t*)object->name, object->name_len);

            rsp->handle = handle;
            rsp_status = STATUS_OK;

        } while (0);
    }
    else if(handle == handle_table[OPT_IDX_CHAR_OBJECT_PROPERTIES_VAL])
    {
        do {
            ESP_LOGI(TAG, "Object Properties WRITE EVENT");
            rsp->handle = handle;

            uint32_t new_properties;

            if(param->write.len != 4)
            {
                rsp_status = INVALID_ATTR_VAL_LENGTH;
                break;
            }

            memcpy(&new_properties, param->write.value, 4);
            if(new_properties > 0xFF)
            {
                rsp_status = WRITE_REQUEST_REJECTED;
                break;
            }

            if(pp_object_manager_is_object_empty())
            {
                ESP_LOGE(TAG, "Object not selected");
                rsp_status = ERROR_OBJECT_NOT_SELECTED;
                break;
            }

            object_t *object = pp_object_manager_get_object();
            
            object->properties = new_properties;
            pp_object_manager_change_properties_in_file();

            ESP_LOGI("WRITE", "Object properties changed to: 0x%" PRIx32, object->properties);

            write_params->need_attr_set = true;
            write_params->length = 4;
            memcpy(write_params->value, (uint8_t*)&object->properties, 4);

            rsp_status = STATUS_OK;

        } while(0);
    }
    else if(handle == handle_table[OPT_IDX_CHAR_OBJECT_OACP_VAL])
    {
        do {
            ESP_LOGE(TAG, "Object OACP WRITE EVENT");
            rsp->handle = handle;

            if(param->write.len == 0)
            {
                ESP_LOGE(TAG, "INVALID ATTR VAL LENGTH = %d", param->write.len);
                rsp_status = INVALID_ATTR_VAL_LENGTH;
                break;
            }

            switch(param->write.value[0])
            {
                case OACP_OP_CODE_CREATE:
                {
                    if(param->write.len != DATA_LEN_UUID16 && param->write.len != DATA_LEN_UUID128)
                    {
                        ESP_LOGE(TAG, "INVALID ATTR VAL LENGTH = %d", param->write.len);
                        rsp_status= INVALID_ATTR_VAL_LENGTH;
                        break;
                    }

                    rsp_status = STATUS_OK;
                    break;
                }

                case OACP_OP_CODE_DELETE:
                {
                    if(param->write.len != 1)
                    {
                        ESP_LOGE(TAG, "INVALID ATTR VAL LENGTH = %d", param->write.len);
                        rsp_status = INVALID_ATTR_VAL_LENGTH;
                        break;
                    }

                    rsp_status = STATUS_OK;
                    break;
                }

                default:
                {
                    rsp_status = STATUS_OK;
                    break;
                }
            }

            if(rsp_status == STATUS_OK)
            {
                write_params->need_ind = true;
            }

        } while(0);
    }
    else if(handle == handle_table[OPT_IDX_CHAR_OBJECT_OACP_IND_CFG])
    {
        do {
            if(param->write.len == 2)
            {
                uint16_t descr_value = param->write.value[1]<<8 | param->write.value[0];
                if (descr_value == 0x0002)
                {
                    ESP_LOGI(TAG, "OACP indicate enable");
                }
                else if (descr_value == 0x0000)
                {
                    ESP_LOGI(TAG, "OACP indicate disable");
                }
                else
                {
                    ESP_LOGE(TAG, "unknown descr value");;
                }
            }

        } while(0);
    }
    else if(handle == handle_table[OPT_IDX_CHAR_OBJECT_OLCP_VAL])
    {
        do {
            ESP_LOGI(TAG, "Object OLCP WRITE EVENT");
            rsp->handle = handle;

            if(param->write.len == 0)
            {
                ESP_LOGE(TAG, "INVALID ATTR VAL LENGTH = %d", param->write.len);
                rsp_status = INVALID_ATTR_VAL_LENGTH;
                break;
            }

            switch(param->write.value[0])
            {
                case OLCP_OP_CODE_FIRST:
                case OLCP_OP_CODE_LAST:
                case OLCP_OP_CODE_PREVIOUS:
                case OLCP_OP_CODE_NEXT:
                case OLCP_OP_CODE_REQ_NUM_OF_OBJ:
                case OLCP_OP_CODE_CLEAR_MARING:
                {
                    if(param->write.len != 1)
                    {
                        ESP_LOGE(TAG, "INVALID ATTR VAL LENGTH = %d", param->write.len);
                        rsp_status = INVALID_ATTR_VAL_LENGTH;
                    }

                    rsp_status = STATUS_OK;
                    break;
                }

                case OLCP_OP_CODE_GOTO:
                {
                    if(param->write.len != 7)
                    {
                        ESP_LOGE(TAG, "INVALID ATTR VAL LENGTH = %d", param->write.len);
                        rsp_status = INVALID_ATTR_VAL_LENGTH;
                    }

                    rsp_status = STATUS_OK;
                    break;
                }

                case OLCP_OP_CODE_ORDER:
                {
                    if(param->write.len != 2)
                    {
                        ESP_LOGE(TAG, "INVALID ATTR VAL LENGTH = %d", param->write.len);
                        rsp_status = INVALID_ATTR_VAL_LENGTH;
                    }

                    rsp_status = STATUS_OK;
                    break;
                }

                default:
                {
                    rsp_status = WRITE_REQUEST_REJECTED;
                    break;
                }
            }

            if(rsp_status == STATUS_OK)
            {
                write_params->need_ind = true;
            }

        } while(0);
    }

    else if(handle == handle_table[OPT_IDX_CHAR_OBJECT_OLCP_IND_CFG])
    {
        do {
            if(param->write.len == 2)
            {
                uint16_t descr_value = param->write.value[1]<<8 | param->write.value[0];
                if (descr_value == 0x0002)
                {
                    ESP_LOGI(TAG, "OLCP indicate enable");
                }
                else if (descr_value == 0x0000)
                {
                    ESP_LOGI(TAG, "OLCP indicate disable");
                }
                else
                {
                    ESP_LOGE(TAG, "unknown descr value");;
                }
            }

        } while(0);
    }
    else if(handle == handle_table[OPT_IDX_CHAR_OBJECT_LIST_FILTER_VAL])
    {
        do {
            ESP_LOGI(TAG, "Object List Filter WRITE EVENT");
            rsp->handle = handle;

            if(param->write.len == 0)
            {
                rsp_status = INVALID_ATTR_VAL_LENGTH;
                break;
            }

            switch(param->write.value[0])
            {
                case NO_FILTER:
                case MARKED_OBJECTS:
                {
                    if(param->write.len != 1)
                    {
                        rsp_status = INVALID_ATTR_VAL_LENGTH;
                        break;
                    }
                    rsp_status = STATUS_OK;
                    break;
                }
                    
                case NAME_STARTS_WITH:
                case NAME_ENDS_WITH:
                case NAME_CONTAINS:
                case NAME_IS_EXACTLY:
                {
                    if(param->write.len <= 1 || param->write.len > NAME_LEN_MAX)
                    {
                        rsp_status = INVALID_ATTR_VAL_LENGTH;
                        break;
                    }
                    rsp_status = STATUS_OK;
                    break;
                }

                case OBJECT_TYPE:
                {
                    if(param->write.len != 17)
                    {
                        rsp_status = INVALID_ATTR_VAL_LENGTH;
                        break;
                    }
                    rsp_status = STATUS_OK;
                    break;
                }

                case CURRENT_SIZE_BETWEEN:
                {
                    if(param->write.len != 9)
                    {
                        rsp_status = INVALID_ATTR_VAL_LENGTH;
                        break;
                    }

                    uint32_t left_size, right_size;
                    memcpy(&left_size, param->write.value, 4);
                    memcpy(&right_size, &param->write.value[4], 4);
                    
                    if(right_size<left_size)
                    {
                        rsp_status = WRITE_REQUEST_REJECTED;
                        break;
                    }

                    rsp_status = STATUS_OK;
                    break;
                }

                case ALLOC_SIZE_BETWEEN:
                {
                    if(param->write.len != 9)
                    {
                        rsp_status = INVALID_ATTR_VAL_LENGTH;
                        break;
                    }
                    rsp_status = STATUS_OK;
                    break;
                }

                default:
                {
                    rsp_status = WRITE_REQUEST_REJECTED;
                    break;
                } 
            }

            ListFilter_t *filter = pp_object_list_get_filter();
            filter->type = param->write.value[0];

            if(param->write.value[0] == OBJECT_TYPE)
            {
                filter->parameter[0] = pp_object_manager_check_type(&param->write.value[1]);
                filter->par_length = 1;
            }
            else
            {
                memcpy(filter->parameter, &param->write.value[1], param->write.len-1);
                filter->par_length = param->write.len-1;
            }
            
            pp_object_list_make_list();

            rsp_status = STATUS_OK;

        } while(0);
    }
    else if(handle == handle_table[OPT_IDX_CHAR_OBJECT_ALARM_ACTION_VAL])
    {
        do {
            ESP_LOGI(TAG, "Object Alarm Action WRITE EVENT, payload length: %u", param->write.len);
            rsp->handle = handle;
            
            if(pp_object_manager_is_object_empty())
            {
                ESP_LOGE(TAG, "Object not selected");
                rsp_status = ERROR_OBJECT_NOT_SELECTED;
                break;
            }

            object_t *object = pp_object_manager_get_object();

            if(pp_object_manager_check_type(object->type.uuid.uuid128) != ALARM_TYPE)
            {
                ESP_LOGE(TAG, "Wrong type - required: Alarm");
                rsp_status = WRITE_REQUEST_REJECTED;
                break;
            }

            rsp_status = pp_set_alarm_values(param->write.value, param->write.len);
            if(rsp_status != STATUS_OK)
            {
                ESP_LOGI(TAG, "Wrong Alarm payload");
                break;
            }

            object->set_custom_object = true;
            pp_object_manager_change_alarm_data_in_file(&current_alarm);
            ESP_LOGI("WRITE", "Object alarm data changed");
            pp_set_next_alarm();
            rsp_status = STATUS_OK;

        } while(0);

    }
    else if(handle == handle_table[OPT_IDX_CHAR_OBJECT_WIFI_ACTION_VAL])
    {
        do {
            ESP_LOGI(TAG, "Object Wifi Action WRITE EVENT");
            rsp->handle = handle;

            if(param->write.len == 0)
            {
                ESP_LOGE(TAG, "INVALID ATTR VAL LENGTH = %d", param->write.len);
                rsp_status = INVALID_ATTR_VAL_LENGTH;
                break;
            }

            switch(param->write.value[0])
            {
                case 0x01:
                {
                    if(param->write.len != 1)
                    {
                        ESP_LOGE(TAG, "INVALID ATTR VAL LENGTH = %d", param->write.len);
                        rsp_status = INVALID_ATTR_VAL_LENGTH;
                        break;
                    }

                    rsp_status = STATUS_OK;
                    pp_start_search_task();
                    break;
                }
                
                case 0x02:
                {
                    uint8_t offset = 1;

                    uint8_t *ssid;
                    uint8_t *password;

                    if(param->write.len < 3)
                    {
                        ESP_LOGE(TAG, "INVALID ATTR VAL LENGTH = %d", param->write.len);
                        rsp_status = INVALID_ATTR_VAL_LENGTH;
                        break;
                    }

                    uint8_t ssid_len = param->write.value[offset];
                    if(param->write.len < 3 + ssid_len)
                    {
                        ESP_LOGE(TAG, "INVALID ATTR VAL LENGTH = %d", param->write.len);
                        rsp_status = INVALID_ATTR_VAL_LENGTH;
                        break;
                    }
                    offset++;

                    ssid = &param->write.value[offset];
                    offset += ssid_len;

                    uint8_t password_len = param->write.value[offset];
                    if(param->write.len < 3 + ssid_len + password_len)
                    {
                        ESP_LOGE(TAG, "INVALID ATTR VAL LENGTH = %d", param->write.len);
                        rsp_status = INVALID_ATTR_VAL_LENGTH;
                        break;
                    }
                    offset++;

                    password = &param->write.value[offset];
                    pp_connect_wifi(ssid, ssid_len, password, password_len);

                    rsp_status = STATUS_OK;
                    break;
                }

                default:
                {
                    rsp_status = WRITE_REQUEST_REJECTED;
                    break;
                }
            }

        } while(0);
    }
    else if(handle == handle_table[OPT_IDX_CHAR_OBJECT_WIFI_ACTION_CFG])
    {
        do {
            if(param->write.len == 2)
            {
                uint16_t descr_value = param->write.value[1]<<8 | param->write.value[0];
                if (descr_value == 0x0002)
                {
                    ESP_LOGI(TAG, "Wifi indicate enable");
                }
                else if(descr_value == 0x0000)
                {
                    ESP_LOGI(TAG, "Wifi indicate disable");
                }
                else
                {
                    ESP_LOGE(TAG, "unknown descr value");;
                }
            }
        } while(0);
    }
    else if(handle == handle_table[OPT_IDX_CHAR_OBJECT_LED_ACTION_VAL])
    {
        do {
            ESP_LOGI(TAG, "Object LED Action WRITE EVENT");
            rsp->handle = handle;

            if(param->write.len != 13)
            {
                ESP_LOGE(TAG, "INVALID ATTR VAL LENGTH = %d", param->write.len);
                rsp_status = INVALID_ATTR_VAL_LENGTH;
                break;
            }

            led_update_t led_params;
            memcpy(&led_params.channel, &param->write.value[0], sizeof(led_params.channel));
            memcpy(&led_params.red, &param->write.value[1],     sizeof(led_params.red));
            memcpy(&led_params.green, &param->write.value[5],   sizeof(led_params.green));
            memcpy(&led_params.blue, &param->write.value[9],    sizeof(led_params.blue));
            xQueueSend(led_update_queue, &led_params, 10);

            rsp_status = STATUS_OK;

        } while(0);
    }

    return (esp_gatt_status_t)rsp_status;
}

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
void pp_object_transfer_write_event_indication(esp_ble_gatts_cb_param_t *param, uint16_t *handle_table, otp_write_attr_t *write_params)
{
    uint16_t handle = param->write.handle;

    if(handle == handle_table[OPT_IDX_CHAR_OBJECT_OACP_VAL])
    {
        switch(param->write.value[0])
        {
            case OACP_OP_CODE_CREATE:
            {
                uint32_t size;
                memcpy(&size, &param->write.value[1], 4);

                esp_bt_uuid_t type;

                if(param->write.len == DATA_LEN_UUID16)
                {
                    type.len = ESP_UUID_LEN_16;
                    memcpy(&type.uuid.uuid16, &param->write.value[5], ESP_UUID_LEN_16);
                }
                else if(param->write.len == DATA_LEN_UUID128)
                {
                    type.len = ESP_UUID_LEN_128;
                    memcpy(type.uuid.uuid128, &param->write.value[5], ESP_UUID_LEN_128);
                }

                oacp_op_code_result_t result = pp_object_manager_create_object(size, type);

                write_params->length = 2;
                write_params->value[0] = OACP_OP_CODE_RESPONSE;
                write_params->value[1] = result;
                break;
            }

            case OACP_OP_CODE_DELETE:
            {
                uint32_t size;
                memcpy(&size, &param->write.value[1], 4);

                oacp_op_code_result_t result = pp_object_manager_delete_object();

                write_params->length = 2;
                write_params->value[0] = OACP_OP_CODE_RESPONSE;
                write_params->value[1] = result;
                pp_set_next_alarm();
                break;
            }

            default:
            {
                write_params->length = 3;
                write_params->value[0] = OACP_OP_CODE_RESPONSE;
                write_params->value[1] = param->write.value[0];
                write_params->value[2] = OACP_RES_OP_CODE_NOT_SUPPORTED;
                break;
            }
        }
    }
    else if(handle == handle_table[OPT_IDX_CHAR_OBJECT_OLCP_VAL])
    {
        switch(param->write.value[0])
        {
            case OLCP_OP_CODE_FIRST:
            {
                olcp_op_code_result_t result = pp_object_manager_first_object();

                write_params->length = 2;
                write_params->value[0] = OLCP_OP_CODE_RESPONSE;
                write_params->value[1] = result;
                break;
            }

            case OLCP_OP_CODE_LAST:
            {
                olcp_op_code_result_t result = pp_object_manager_last_object();

                write_params->length = 2;
                write_params->value[0] = OLCP_OP_CODE_RESPONSE;
                write_params->value[1] = result;
                break;
            }

            case OLCP_OP_CODE_PREVIOUS:
            {
                olcp_op_code_result_t result = pp_object_manager_previous_object();

                write_params->length = 2;
                write_params->value[0] = OLCP_OP_CODE_RESPONSE;
                write_params->value[1] = result;
                break;
            }

            case OLCP_OP_CODE_NEXT:
            {
                olcp_op_code_result_t result = pp_object_manager_next_object();

                write_params->length = 2;
                write_params->value[0] = OLCP_OP_CODE_RESPONSE;
                write_params->value[1] = result;
                break;
            }

            case OLCP_OP_CODE_GOTO:
            {
                uint64_t id = 0;
                memcpy(&id, &param->write.value[1], 6);
                ESP_LOGI(TAG, "Searching for ID: %llx", id);
                
                olcp_op_code_result_t result = pp_object_manager_goto_object(id);

                write_params->length = 2;
                write_params->value[0] = OLCP_OP_CODE_RESPONSE;
                write_params->value[1] = result;
                break;
            }

            case OLCP_OP_CODE_ORDER:
            {
                uint8_t type = param->write.value[1];
                olcp_op_code_result_t result = pp_object_manager_order_object(type);

                write_params->length = 2;
                write_params->value[0] = OLCP_OP_CODE_RESPONSE;
                write_params->value[1] = result;
                break;
            }

            case OLCP_OP_CODE_REQ_NUM_OF_OBJ:
            {
                uint32_t number_of_objects = 0;
                olcp_op_code_result_t result = pp_object_manager_request_number(&number_of_objects);
                
                write_params->value[0] = OLCP_OP_CODE_RESPONSE;
                write_params->value[1] = param->write.value[0];
                write_params->value[2] = result;

                if(result == OLCP_RES_SUCCESS)
                {
                    write_params->length = 7;
                    memcpy(&write_params->value[3], &number_of_objects, sizeof(number_of_objects));
                }
                else
                {
                    write_params->length = 3;
                }
                break;
            }

            case OLCP_OP_CODE_CLEAR_MARING:
            {
                olcp_op_code_result_t result = pp_object_manager_clear_marking();

                write_params->length = 2;
                write_params->value[0] = OLCP_OP_CODE_RESPONSE;
                write_params->value[1] = result;
                break;
            }

            default:
            {
                write_params->length = 3;
                write_params->value[0] = OLCP_OP_CODE_RESPONSE;
                write_params->value[1] = param->write.value[0];
                write_params->value[2] = OLCP_RES_OP_CODE_NOT_SUPPORTED;
                break;
            }
        }
    }
}


// static esp_err_t pp_object_transfer_write_Ringtone_Action(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param, uint16_t *handle_table)
// {
//     ESP_LOGD(TAG, "Object Ringtone Action WRITE EVENT");

//     esp_gatt_rsp_t rsp;
//     rsp.handle = handle_table[OPT_IDX_CHAR_OBJECT_RINGTONE_ACTION_VAL];
//     esp_err_t ret;

//     if(param->write.need_rsp)
//     {
//         ret = esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, STATUS_OK, &rsp);
//         if(ret) return ret;
//     }

//     return ESP_OK;
// }

/** @brief pp_object_transfer_send_found_wifi_ind: Sends found WiFi networks by indication
 * 
 * This function should sends indication to connected Bluetooth device with discovered WiFi
 * network information.
 *
 * @param[in]   wifi_record     (wifi_ap_record_t*) Pointer to discovered WiFi network record
 * 
 * @return
 */
void pp_object_transfer_send_found_wifi_ind(wifi_ap_record_t *wifi_record)
{
    uint8_t indicate_data[37];
    indicate_data[0] = 1;

    uint8_t ssid_len = strlen((char*)wifi_record->ssid);
    indicate_data[1] = ssid_len;

    uint8_t indicate_data_len = 4 + ssid_len;
    uint8_t *payload_ptr = &indicate_data[2];

    memcpy(payload_ptr, wifi_record->ssid, ssid_len);
    payload_ptr += ssid_len;

    *payload_ptr = (uint8_t)wifi_record->rssi;
    payload_ptr++;

    *payload_ptr = (uint8_t)wifi_record->authmode;

    esp_ble_gatts_send_indicate(gatts_if_curr, bt_connection_id_curr, bt_wifi_handle, indicate_data_len, indicate_data, true);
}

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
void pp_object_transfer_send_simple_wifi_ind(uint8_t type)
{
    esp_ble_gatts_send_indicate(gatts_if_curr, bt_connection_id_curr, bt_wifi_handle, 1, &type, true);
}