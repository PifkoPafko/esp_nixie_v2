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
static uint64_t ringtone_id = 0;

/* Functions */

/** @brief pp_ringtone_action: Ringtone action
 *
 * @return otp_rsp_status_t
 */
otp_rsp_status_t pp_ringtone_action(uint8_t *data, uint16_t len)
{
    otp_rsp_status_t result = WRITE_REQUEST_REJECTED;

    switch(ringtone_sm)
    {
        case IDLE_RINGTONE:
        {
            if(len < 1)
            {
                ESP_LOGE(RINGTONE_TAG, "Wrong data length: act = %" PRIu16, len);
                result = INVALID_ATTR_VAL_LENGTH;
                break;
            }

            if(data[RINGTONE_CODE_INDEX] == RINGTONE_NEW_CODE)
            {
                uint16_t exp_data_len = RINGTONE_CODE_SIZE + RINGTONE_NAME_LEN_SIZE + data[RINGTONE_NEW_NAME_LEN_INDEX] + RIGNTONE_DATA_LEN_SIZE;
                if(len != exp_data_len)
                {
                    ESP_LOGE(RINGTONE_TAG, "Wrong data length: act = %" PRIu16 ", exp = %" PRIu16, len, exp_data_len);
                    result = INVALID_ATTR_VAL_LENGTH;
                    break;
                }

                if(data[RINGTONE_NEW_NAME_LEN_INDEX] > NAME_LEN_MAX)
                {
                    ESP_LOGE(RINGTONE_TAG, "Name too long, act = %" PRIu8 ", max = %d", data[RINGTONE_NEW_NAME_LEN_INDEX], NAME_LEN_MAX);
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
                ringtone_id = current_object_p->id;

                // print created file
                #ifdef TEST_PRINT_RINGTONE
                ESP_LOGW(RINGTONE_TAG, "TEST READING FILE, ID = %" PRIu64, ringtone_id);
                FILE* f = pp_object_manager_open_file("r+", ringtone_id);
                int c;
                
                for(uint16_t char_cnt = 0; char_cnt < PRINT_RINGTONE_CNT; ++char_cnt)
                {
                    c = fgetc(f);
                    if(c == EOF) break;
                    printf("%c", c);
                }
                printf("\n");

                fclose(f);
                #endif

                memcpy(current_object_p->name, &data[RINGTONE_NEW_NAME_INDEX], data[RINGTONE_NEW_NAME_LEN_INDEX]);
                current_object_p->name[data[RINGTONE_NEW_NAME_LEN_INDEX]] = '\0';
                current_object_p->name_len = data[RINGTONE_NEW_NAME_LEN_INDEX];
                ESP_LOGI(RINGTONE_TAG, "Ringtone name = %s", current_object_p->name);
                pp_object_manager_change_name_in_file();

                // print file after changing the name
                #ifdef TEST_PRINT_RINGTONE
                ESP_LOGW(RINGTONE_TAG, "TEST READING FILE, ID = %" PRIu64, ringtone_id);
                f = pp_object_manager_open_file("r+", ringtone_id);
                
                for(uint16_t char_cnt = 0; char_cnt < PRINT_RINGTONE_CNT; ++char_cnt)
                {
                    c = fgetc(f);
                    if(c == EOF) break;
                    printf("%c", c);
                }
                printf("\n");

                fclose(f);
                #endif

                memcpy(&ringtone_len, &data[RINGTONE_CODE_SIZE + RINGTONE_NAME_LEN_SIZE + data[RINGTONE_NEW_NAME_LEN_INDEX]], sizeof(ringtone_len));
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

                uint64_t id = 0;
                memcpy(&id, &data[RINGTONE_DEL_ID_INDEX], RIGNTONE_ID_SIZE);
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
            else if(data[RINGTONE_CODE_INDEX] == RINGTONE_PLAY_CODE)
            {
                if(len != RINGTONE_CODE_SIZE + RIGNTONE_DEFAULT_SIZE + RIGNTONE_ID_SIZE)
                {
                    ESP_LOGE(RINGTONE_TAG, "Wrong data length: act = %" PRIu16 ", exp = %d", len, RINGTONE_CODE_SIZE + RIGNTONE_DEFAULT_SIZE + RIGNTONE_ID_SIZE);
                    result = INVALID_ATTR_VAL_LENGTH;
                    break;
                }

                uint8_t is_default;
                memcpy(&is_default, &data[RINGTONE_PLAY_DEF_INDEX], RIGNTONE_DEFAULT_SIZE);
                ESP_LOGI(RINGTONE_TAG, "Ringtone defult: %s", is_default ? "yes" : "no");

                uint64_t id;
                memcpy(&id, &data[RINGTONE_PLAY_ID_INDEX], RIGNTONE_ID_SIZE);
                ESP_LOGI(RINGTONE_TAG, "Ringtone object ID = %" PRIu64, id);

                if(is_default != 0 && is_default != 1)
                {
                    ESP_LOGE(RINGTONE_TAG, "Wrong default value: act = %" PRIu8 ", exp = 0 or 1", is_default);
                    result = WRITE_REQUEST_REJECTED;
                    break;
                }

                alarm_notification_type_t notif = ALARM_STOP_NOTIFICATION;
                xQueueSend(alarm_queue, &notif, 10);
                is_playing = false;

                pp_set_next_ringtone_id(is_default, id);

                notif = ALARM_START_NOTIFICATION;
                xQueueSend(alarm_queue, &notif, 10);
                is_playing = true;

                result = STATUS_OK;
                break;
            }
            else if(data[RINGTONE_CODE_INDEX] == RINGTONE_STOP_CODE)
            {
                if(len != RINGTONE_CODE_SIZE)
                {
                    ESP_LOGE(RINGTONE_TAG, "Wrong data length: act = %" PRIu16 ", exp = %d", len, RINGTONE_CODE_SIZE);
                    result = INVALID_ATTR_VAL_LENGTH;
                    break;
                }

                alarm_notification_type_t notif = ALARM_STOP_NOTIFICATION;
                xQueueSend(alarm_queue, &notif, 10);
                is_playing = false;
                result = STATUS_OK;
                break;
            }
            else
            {
                ESP_LOGE(RINGTONE_TAG, "Unsupported code = %" PRIu8, data[RINGTONE_CODE_INDEX]);
                result = WRITE_REQUEST_REJECTED;
                break;
            }

            break;
        }

        case DATA_RINGTONE:
        {
            if(len < 7)
            {
                ESP_LOGE(RINGTONE_TAG, "Wrong data length: act = %" PRIu16, len);
                result = INVALID_ATTR_VAL_LENGTH;

                ESP_LOGE(RINGTONE_TAG, "Abborting ringtone upload");
                ringtone_len_cur = 0;
                ringtone_sm = IDLE_RINGTONE;

                pp_object_manager_goto_object(ringtone_id);
                pp_object_manager_delete_object();
                break;
            }

            if(data[RINGTONE_CODE_INDEX] == RINGTONE_DATA_CODE)
            {
                uint16_t data_length;
                memcpy(&data_length, &data[RINGTONE_DATA_LEN_INDEX], RINGTONE_DATA_LEN_SIZE);
                ESP_LOGI(RINGTONE_TAG, "Requested packet data length = %" PRIu16, data_length);

                if(len != RINGTONE_CODE_SIZE + RINGTONE_DATA_ID_SIZE + RINGTONE_DATA_LEN_SIZE + data_length)
                {
                    ESP_LOGE(RINGTONE_TAG, "Wrong data length: act = %" PRIu16, len);
                    result = INVALID_ATTR_VAL_LENGTH;

                    ESP_LOGE(RINGTONE_TAG, "Abborting ringtone upload");
                    ringtone_len_cur = 0;
                    ringtone_sm = IDLE_RINGTONE;

                    pp_object_manager_goto_object(ringtone_id);
                    pp_object_manager_delete_object();
                    break;
                }

                uint32_t data_id;
                memcpy(&data_id, &data[RINGTONE_DATA_ID_INDEX], RINGTONE_DATA_ID_SIZE);

                if(data_id == ringtone_packet_id_last + 1)
                {
                    ringtone_packet_id_last = data_id;

                    ESP_LOGI(RINGTONE_TAG, "Writing data packet id = %" PRIu32, data_id);
                    FILE* f = pp_object_manager_open_file("a+", ringtone_id);

                    fwrite(&data[RINGTONE_DATA_INDEX], data_length, sizeof(data[0]), f);
                    fclose(f);

                    // print file after the data load
                    #ifdef TEST_PRINT_RINGTONE
                    f = pp_object_manager_open_file("r+", ringtone_id);
                    int c;
                    
                    for(uint16_t char_cnt = 0; char_cnt < PRINT_RINGTONE_CNT; ++char_cnt)
                    {
                        c = fgetc(f);
                        if(c == EOF) break;
                        printf("%c", c);
                    }
                    printf("\n");

                    fclose(f);
                    #endif


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
                    result = ERROR_OLD_DATA;
                    break;
                }
                else
                {
                    ESP_LOGI(RINGTONE_TAG, "Worng data packet id = %" PRIu32, data_id);
                    result = INVALID_ATTR_VAL_LENGTH;

                    ESP_LOGE(RINGTONE_TAG, "Abborting ringtone upload");
                    ringtone_len_cur = 0;
                    ringtone_sm = IDLE_RINGTONE;

                    pp_object_manager_goto_object(ringtone_id);
                    pp_object_manager_delete_object();
                    break;
                }
            }
            else
            {
                ESP_LOGE(RINGTONE_TAG, "Unsupported code = %" PRIu8, data[RINGTONE_CODE_INDEX]);
                result = WRITE_REQUEST_REJECTED;

                ESP_LOGE(RINGTONE_TAG, "Abborting ringtone upload");
                ringtone_len_cur = 0;
                ringtone_sm = IDLE_RINGTONE;

                pp_object_manager_goto_object(ringtone_id);
                pp_object_manager_delete_object();
                break;
            }

            break;

        }

        case VERIFY_RINGTONE:
        {
            if(len < 1)
            {
                ESP_LOGE(RINGTONE_TAG, "Wrong data length: act = %" PRIu16, len);
                result = INVALID_ATTR_VAL_LENGTH;

                ESP_LOGE(RINGTONE_TAG, "Abborting ringtone upload");
                ringtone_len_cur = 0;
                ringtone_sm = IDLE_RINGTONE;

                pp_object_manager_goto_object(ringtone_id);
                pp_object_manager_delete_object();
                break;
            }

            if(data[RINGTONE_CODE_INDEX] == RINGTONE_VERIFY_CODE)
            {
                if(len != 1)
                {
                    ESP_LOGE(RINGTONE_TAG, "Wrong data length: act = %" PRIu16, len);
                    result = INVALID_ATTR_VAL_LENGTH;

                    ESP_LOGE(RINGTONE_TAG, "Abborting ringtone upload");
                    ringtone_len_cur = 0;
                    ringtone_sm = IDLE_RINGTONE;

                    pp_object_manager_goto_object(ringtone_id);
                    pp_object_manager_delete_object();
                    break;
                }

                ESP_LOGI(RINGTONE_TAG, "Ringtone upload complete!!!");
                ringtone_sm = IDLE_RINGTONE;
                result = STATUS_OK;
                break;
            }
            else if(data[RINGTONE_CODE_INDEX] == RINGTONE_DATA_CODE)
            {
                ESP_LOGE(RINGTONE_TAG, "Old data. File already uploaded");
                result = ERROR_OLD_DATA;
                break;
            }
            else
            {
                ESP_LOGE(RINGTONE_TAG, "Wrong verification code = %" PRIu8, data[0]);
                result = WRITE_REQUEST_REJECTED;

                ESP_LOGE(RINGTONE_TAG, "Abborting ringtone upload");
                ringtone_len_cur = 0;
                ringtone_sm = IDLE_RINGTONE;

                pp_object_manager_goto_object(ringtone_id);
                pp_object_manager_delete_object();
                break;
            }

            break;
        }
    }

    return result;
}