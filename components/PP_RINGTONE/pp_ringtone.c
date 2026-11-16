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
#include "pp_ringtone.h"

/* Macros */
#define RINGTONE_TAG "RINGTONE"

/* Variables */
static ringtone_sm_t ringtone_sm = IDLE_RINGTONE;
static uint32_t ringtone_packet_id_last = 0;
static uint32_t ringtone_len = 0;
static uint32_t ringtone_len_cur = 0;

/* Functions */

/** @brief pp_ringtone_action: Ringtone action
 *
 * @return otp_rsp_status_t
 */
otp_rsp_status_t pp_ringtone_action(uint8_t *data, uint8_t len)
{
    otp_rsp_status_t result = WRITE_REQUEST_REJECTED;

    switch(ringtone_sm)
    {
        case IDLE_RINGTONE:
        {
            if(len >= 2)
            {
                if(data[RINGTONE_CODE_INDEX] == RINGTONE_NEW_CODE)
                {
                    uint8_t exp_data_len = RINGTONE_CODE_SIZE + RINGTONE_NAME_LEN_SIZE + data[RINGTONE_NAME_LEN_INDEX] + RIGNTONE_DATA_LEN_SIZE;
                    if(len != exp_data_len)
                    {
                        ESP_LOGE(RINGTONE_TAG, "Wrong data length: act = %" PRIu16 ", exp = %" PRIu8, len, exp_data_len);
                        result = INVALID_ATTR_VAL_LENGTH;
                        break;
                    }

                    uint8_t ringtone_type_uuid[16] = {0x03, 0x00, 0x12, 0xAC, 0x42, 0x02, 0x61, 0xA2, 0xED, 0x11, 0xBA, 0x29, 0xB8, 0x13, 0x08, 0xCC};
                    esp_bt_uuid_t type;
                    type.len = 16;
                    memcpy(type.uuid.uuid128, ringtone_type_uuid, 16);

                    if(pp_object_manager_create_object(0, type) != OACP_RES_SUCCESS)
                    {
                        ESP_LOGE(RINGTONE_TAG, "Object not created, result = %d", result);
                        result = WRITE_REQUEST_REJECTED;
                        break;
                    }

                    object_t* current_object_p = pp_object_manager_get_object();

                    memcpy(current_object_p->name, &data[RINGTONE_NAME_INDEX], data[RINGTONE_NAME_LEN_INDEX]);
                    current_object_p->name[data[RINGTONE_NAME_LEN_INDEX]] = '\0';
                    ESP_LOGI(RINGTONE_TAG, "Ringtone name = %s", current_object_p->name);
                    pp_object_manager_change_name_in_file();

                    memcpy(&ringtone_len, &data[RINGTONE_CODE_SIZE + RINGTONE_NAME_LEN_SIZE + data[RINGTONE_NAME_LEN_INDEX]], sizeof(ringtone_len));
                    ESP_LOGI(RINGTONE_TAG, "Ringtone Data length = %" PRIu32, ringtone_len);

                    ringtone_len_cur = 0;
                    ringtone_packet_id_last = 0;
                    ringtone_sm = DATA_RINGTONE;

                    result = STATUS_OK;
                    break;
                }
                else if(data[RINGTONE_CODE_INDEX] == RINGTONE_DELETE_CODE)
                {
                    if(len != RINGTONE_CODE_SIZE + RIGNTONE_ID_SIZE)
                    {
                        ESP_LOGE(RINGTONE_TAG, "Wrong data length: act = %" PRIu16 ", exp = %d", len, RINGTONE_CODE_SIZE + RIGNTONE_ID_SIZE);
                        result = INVALID_ATTR_VAL_LENGTH;
                        break;
                    }

                    uint64_t id;
                    memcpy(&id, &data[RINGTONE_ID_INDEX], RIGNTONE_ID_SIZE);
                    ESP_LOGI(RINGTONE_TAG, "Ringtone object ID = %" PRIu64, id);

                    if(pp_object_manager_goto_object(id) != OLCP_RES_SUCCESS)
                    {
                        ESP_LOGE(RINGTONE_TAG, "Ringtone object ID = %" PRIu64 " not found", id);
                        result = WRITE_REQUEST_REJECTED;
                        break;
                    }

                    if(pp_object_manager_delete_object() != OACP_RES_SUCCESS)
                    {
                        ESP_LOGE(RINGTONE_TAG, "Ringtone object ID = %" PRIu64 " not found", id);
                        result = WRITE_REQUEST_REJECTED;
                        break;
                    }

                    result = STATUS_OK;
                    break;
                }
                else
                {
                    ESP_LOGE(RINGTONE_TAG, "Unsupported code = %" PRIu8, data[RINGTONE_CODE_INDEX]);
                    result = WRITE_REQUEST_REJECTED;
                    break;
                }
            }
            else
            {
                ESP_LOGE(RINGTONE_TAG, "Wrong data length: act = %" PRIu16, len);
                result = INVALID_ATTR_VAL_LENGTH;
                break;
            }
        }

        case DATA_RINGTONE:
        {
            if(len >= 6)
            {
                uint16_t data_length;
                memcpy(&data_length, &data[RINGTONE_DATA_LEN_INDEX], RINGTONE_DATA_LEN_SIZE);
                ESP_LOGI(RINGTONE_TAG, "Requested packet data length = %" PRIu16, data_length);

                if(len != RINGTONE_DATA_ID_SIZE + RINGTONE_DATA_LEN_SIZE + data_length)
                {
                    ESP_LOGE(RINGTONE_TAG, "Wrong data length: act = %" PRIu16, len);
                    result = INVALID_ATTR_VAL_LENGTH;

                    ESP_LOGE(RINGTONE_TAG, "Abborting ringtone upload");
                    ringtone_len_cur = 0;
                    ringtone_sm = IDLE_RINGTONE;

                    pp_object_manager_delete_object();
                    break;
                }

                uint32_t data_id;
                memcpy(&data_id, &data[RINGTONE_DATA_ID_INDEX], RINGTONE_DATA_ID_SIZE);

                if(data_id == ringtone_packet_id_last + 1)
                {
                    ringtone_packet_id_last = data_id;

                    object_t* current_object_p = pp_object_manager_get_object();

                    ESP_LOGI(RINGTONE_TAG, "Writing data packet id = %" PRIu32, data_id);
                    FILE* f = pp_object_manager_open_file("a+", current_object_p->id);

                    fwrite(data, data_length, sizeof(data[0]), f);
                    fclose(f);

                    ringtone_len_cur = ringtone_len_cur + data_length;
                    ESP_LOGI(RINGTONE_TAG, "Data written: %" PRIu32 "/%" PRIu32, ringtone_len_cur, ringtone_len);

                    if(ringtone_len_cur == ringtone_len)
                    {
                        ESP_LOGI(RINGTONE_TAG, "All data collected");
                        ringtone_sm = VERIFY_RINGTONE;
                    }

                    result = STATUS_OK;
                    break;
                }
                else if(data_id == ringtone_packet_id_last)
                {
                    ESP_LOGI(RINGTONE_TAG, "Received the same data packet id = %" PRIu32, data_id);
                    result = STATUS_OK;
                    break;
                }
                else
                {
                    ESP_LOGI(RINGTONE_TAG, "Worng data packet id = %" PRIu32, data_id);
                    result = INVALID_ATTR_VAL_LENGTH;

                    ESP_LOGE(RINGTONE_TAG, "Abborting ringtone upload");
                    ringtone_len_cur = 0;
                    ringtone_sm = IDLE_RINGTONE;

                    pp_object_manager_delete_object();
                    break;
                }
            }
            else
            {
                ESP_LOGE(RINGTONE_TAG, "Wrong data length: act = %" PRIu16, len);
                result = INVALID_ATTR_VAL_LENGTH;

                ESP_LOGE(RINGTONE_TAG, "Abborting ringtone upload");
                ringtone_len_cur = 0;
                ringtone_sm = IDLE_RINGTONE;

                pp_object_manager_delete_object();
                break;
            }
        }

        case VERIFY_RINGTONE:
        {
            if(len != 1)
            {
                ESP_LOGE(RINGTONE_TAG, "Wrong data length: act = %" PRIu16, len);
                result = INVALID_ATTR_VAL_LENGTH;

                ESP_LOGE(RINGTONE_TAG, "Abborting ringtone upload");
                ringtone_len_cur = 0;
                ringtone_sm = IDLE_RINGTONE;

                pp_object_manager_delete_object();
                break;
            }

            if(data[0] != RINGTONE_VER_CODE_INDEX)
            {
                ESP_LOGE(RINGTONE_TAG, "Wrong verification code = %" PRIu8, data[0]);
                result = WRITE_REQUEST_REJECTED;

                ESP_LOGE(RINGTONE_TAG, "Abborting ringtone upload");
                ringtone_len_cur = 0;
                ringtone_sm = IDLE_RINGTONE;

                pp_object_manager_delete_object();
                break;
            }

            ESP_LOGE(RINGTONE_TAG, "Ringtone upload complete!!!");
            ringtone_sm = IDLE_RINGTONE;
            result = STATUS_OK;
            break;
        }
    }

    return result;
}