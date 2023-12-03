#include "alarm.h"
#include "project_defs.h"
#include "esp_err.h"
#include "esp_log.h"
#include "stdlib.h"
#include "ObjectTransfer_defs.h"

static const char* TAG = "ALARM";

uint8_t set_alarm_values(alarm_mode_args_t *alarm, uint8_t *payload, uint16_t payload_len)
{

    if((payload_len < ALARM_MODE_PAYLOAD_SIZE_MIN || payload_len > ALARM_MODE_PAYLOAD_SIZE_MAX))
    {
        ESP_LOGE(TAG, "Wrong Alarm Length 1");
        return INVALID_ATTR_VAL_LENGTH;
    }

    alarm->mode = *payload;
    ESP_LOGI(TAG, "Alarm mode: %u", alarm->mode);
    payload += ALARM_FIELD_SIZE;

    alarm->enable = *payload;
    ESP_LOGI(TAG, "Alarm enable: %u", alarm->enable);
    payload += ALARM_FIELD_SIZE;

    alarm->desc_len = *payload;
    ESP_LOGI(TAG, "Alarm desc_len: %u", alarm->desc_len);
    payload += ALARM_FIELD_SIZE;

    if(alarm->mode >= ALARM_MODES_NUM)
    {
        ESP_LOGE(TAG, "Wrong Alarm Mode");
        return WRITE_REQUEST_REJECTED;
    }

    if(alarm->enable > 1)
    {
        ESP_LOGE(TAG, "Wrong Alarm Enable value");
        return WRITE_REQUEST_REJECTED;
    }
    
    if(alarm->desc_len > ALARM_DESC_LEN_MAX)
    {
        ESP_LOGE(TAG, "Wrong Alarm Enable value");
        return WRITE_REQUEST_REJECTED;
    }

    switch(alarm->mode) 
    {
        case ALARM_SINGLE_MODE:
        {
            if (payload_len != ALARM_MODE_SINGLE_PAYLOAD_SIZE_MIN + alarm->desc_len ) 
            {
                ESP_LOGE(TAG, "Wrong Alarm Length 2");
                return INVALID_ATTR_VAL_LENGTH;
            }
            break;
        }
            

        case ALARM_WEEKLY_MODE:
        {
            if (payload_len != ALARM_MODE_WEEKLY_PAYLOAD_SIZE_MIN + alarm->desc_len ) 
            {
                ESP_LOGE(TAG, "Wrong Alarm Length 3");
                return INVALID_ATTR_VAL_LENGTH;
            }
            break;
        }
            

        case ALARM_MONTHLY_MODE:
        {
            if (payload_len != ALARM_MODE_MONTHLY_PAYLOAD_SIZE_MIN + alarm->desc_len ) 
            {
                ESP_LOGE(TAG, "Wrong Alarm Length 4");
                return INVALID_ATTR_VAL_LENGTH;
            }
            break;
        }
            

        case ALARM_YEARLY_MODE:
        {
            if (payload_len != ALARM_MODE_YEARLY_PAYLOAD_SIZE_MIN + alarm->desc_len ) 
            {
                ESP_LOGE(TAG, "Wrong Alarm Length 5");
                return INVALID_ATTR_VAL_LENGTH;
            }
            break;
        }
    }
    
    memcpy((uint8_t*)alarm->desc, payload, alarm->desc_len);
    alarm->desc[alarm->desc_len] = '\0';
    payload += alarm->desc_len;

    alarm->hour = *payload;
    payload += ALARM_FIELD_SIZE;

    alarm->minute = *payload;
    payload += ALARM_FIELD_SIZE;

    switch(alarm->mode) 
    {
        case ALARM_SINGLE_MODE:
        {
            alarm->args.single_alarm_args.day = *payload;
            payload += ALARM_FIELD_SIZE;

            alarm->args.single_alarm_args.month = *payload;
            payload += ALARM_FIELD_SIZE;

            alarm->args.single_alarm_args.year = *payload;
            payload += ALARM_FIELD_SIZE;

            break;
        }
            

        case ALARM_WEEKLY_MODE:
        {
            alarm->args.days = *payload;
            payload += ALARM_FIELD_SIZE;
            break;
        }
            

        case ALARM_MONTHLY_MODE:
        {
            alarm->args.day = *payload;
            payload += ALARM_FIELD_SIZE;
            break;
        }
            

        case ALARM_YEARLY_MODE:
        {
            alarm->args.yearly_alarm_args.day = *payload;
            payload += ALARM_FIELD_SIZE;

            alarm->args.yearly_alarm_args.month = *payload;
            payload += ALARM_FIELD_SIZE;
            break;
        }
    }

    alarm->volume = *payload;

    if(alarm->volume > ALARM_VOLUME_MAX)
    {
        ESP_LOGE(TAG, "Wrong Volume value");
        return WRITE_REQUEST_REJECTED;
    }

    return STATUS_OK;
}