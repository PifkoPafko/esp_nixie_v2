#include "project_defs.h"
#include "alarm.h"
#include "ObjectManager.h"
#include "ObjectTransfer_attr_ids.h"
#include "ObjectTransfer_defs.h"
#include "ObjectManagerIdList.h"

#include "esp_err.h"
#include "esp_log.h"
#include "stdlib.h"
#include <sys/time.h>
#include <time.h>

static const char* TAG = "ALARM";

alarm_mode_args_t alarm;
uint64_t next_alarm_id = 0;
bool next_alarm_enabled = false;
time_t next_alarm_interval = 0;

#define ALARM_LOG

uint8_t set_alarm_values(uint8_t *payload, uint16_t payload_len)
{
    if((payload_len < ALARM_MODE_PAYLOAD_SIZE_MIN || payload_len > ALARM_MODE_PAYLOAD_SIZE_MAX))
    {
        ESP_LOGE(TAG, "Wrong Alarm Length 1");
        return INVALID_ATTR_VAL_LENGTH;
    }

    alarm.mode = *payload;
    payload += ALARM_FIELD_SIZE;

    alarm.enable = *payload;
    payload += ALARM_FIELD_SIZE;

    alarm.desc_len = *payload;
    payload += ALARM_FIELD_SIZE;

    if(alarm.mode >= ALARM_MODES_NUM)
    {
        ESP_LOGE(TAG, "Wrong Alarm Mode");
        return WRITE_REQUEST_REJECTED;
    }

    if(alarm.enable > 1)
    {
        ESP_LOGE(TAG, "Wrong Alarm Enable value");
        return WRITE_REQUEST_REJECTED;
    }
    
    if(alarm.desc_len > ALARM_DESC_LEN_MAX)
    {
        ESP_LOGE(TAG, "Wrong Alarm Enable value");
        return WRITE_REQUEST_REJECTED;
    }

    switch(alarm.mode) 
    {
        case ALARM_SINGLE_MODE:
        {
            if (payload_len != ALARM_MODE_SINGLE_PAYLOAD_SIZE_MIN + alarm.desc_len ) 
            {
                ESP_LOGE(TAG, "Wrong Alarm Length 2");
                return INVALID_ATTR_VAL_LENGTH;
            }
            break;
        }
            

        case ALARM_WEEKLY_MODE:
        {
            if (payload_len != ALARM_MODE_WEEKLY_PAYLOAD_SIZE_MIN + alarm.desc_len ) 
            {
                ESP_LOGE(TAG, "Wrong Alarm Length 3");
                return INVALID_ATTR_VAL_LENGTH;
            }
            break;
        }
            

        case ALARM_MONTHLY_MODE:
        {
            if (payload_len != ALARM_MODE_MONTHLY_PAYLOAD_SIZE_MIN + alarm.desc_len ) 
            {
                ESP_LOGE(TAG, "Wrong Alarm Length 4");
                return INVALID_ATTR_VAL_LENGTH;
            }
            break;
        }
            

        case ALARM_YEARLY_MODE:
        {
            if (payload_len != ALARM_MODE_YEARLY_PAYLOAD_SIZE_MIN + alarm.desc_len ) 
            {
                ESP_LOGE(TAG, "Wrong Alarm Length 5");
                return INVALID_ATTR_VAL_LENGTH;
            }
            break;
        }
    }
    
    memcpy((uint8_t*)alarm.desc, payload, alarm.desc_len);
    alarm.desc[alarm.desc_len] = '\0';
    payload += alarm.desc_len;

    alarm.hour = *payload;
    payload += ALARM_FIELD_SIZE;

    alarm.minute = *payload;
    payload += ALARM_FIELD_SIZE;

    switch(alarm.mode) 
    {
        case ALARM_SINGLE_MODE:
        {
            alarm.args.single_alarm_args.day = *payload;
            payload += ALARM_FIELD_SIZE;

            alarm.args.single_alarm_args.month = *payload;
            payload += ALARM_FIELD_SIZE;

            alarm.args.single_alarm_args.year = *payload;
            payload += ALARM_FIELD_SIZE;

            break;
        }
            

        case ALARM_WEEKLY_MODE:
        {
            alarm.args.days = *payload;
            payload += ALARM_FIELD_SIZE;
            break;
        }
            

        case ALARM_MONTHLY_MODE:
        {
            alarm.args.day = *payload;
            payload += ALARM_FIELD_SIZE;
            break;
        }
            

        case ALARM_YEARLY_MODE:
        {
            alarm.args.yearly_alarm_args.day = *payload;
            payload += ALARM_FIELD_SIZE;

            alarm.args.yearly_alarm_args.month = *payload;
            payload += ALARM_FIELD_SIZE;
            break;
        }
    }

    alarm.volume = *payload;

    if(alarm.volume > ALARM_VOLUME_MAX)
    {
        ESP_LOGE(TAG, "Wrong Volume value");
        return WRITE_REQUEST_REJECTED;
    }

    return STATUS_OK;
}

esp_err_t get_alarm_from_file(uint64_t id, alarm_mode_args_t *alarm_p)
{
    FILE* f = ObjectManager_open_file("r", id);

    if(f == NULL)
    {
        return ESP_ERR_NOT_FOUND;
    }

    fpos_t pos;
    bool found = seekfor(f, "ALARM PROPERTIES\n", &pos);

    if(!found)
    {
        return ESP_ERR_NOT_FOUND;
    }

    char line[70];
    char *ptr;

    fgets(line, sizeof(line), f);
    alarm_p->mode = strtol(&line[strlen("Mode: ") ], &ptr, 16);

    fgets(line, sizeof(line), f);
    alarm_p->enable = strtol(&line[strlen("Enable: ") ], &ptr, 16);

    fgets(line, sizeof(line), f);
    alarm_p->desc_len = strtol(&line[strlen("Description length: ") ], &ptr, 16);

    fgets(line, sizeof(line), f);
    strncpy((char*)alarm_p->desc, (char*)&line[strlen("Description: ")], alarm_p->desc_len);
    alarm_p->desc[alarm_p->desc_len] = '\0';

    fgets(line, sizeof(line), f);
    alarm_p->hour = strtol(&line[strlen("Hour: ") ], &ptr, 16);

    fgets(line, sizeof(line), f);
    alarm_p->minute = strtol(&line[strlen("Minute: ") ], &ptr, 16);

    switch(alarm_p->mode) 
    {
        case ALARM_SINGLE_MODE:
        {
            fgets(line, sizeof(line), f);
            alarm_p->args.single_alarm_args.day = strtol(&line[strlen("Day: ") ], &ptr, 16);

            fgets(line, sizeof(line), f);
            alarm_p->args.single_alarm_args.month = strtol(&line[strlen("Month: ") ], &ptr, 16);

            fgets(line, sizeof(line), f);
            alarm_p->args.single_alarm_args.year = strtol(&line[strlen("Year: ") ], &ptr, 16);
            break;
        }
            
        case ALARM_WEEKLY_MODE:
        {
            fgets(line, sizeof(line), f);
            alarm_p->args.days = strtol(&line[strlen("Days: ") ], &ptr, 16);
            break;
        }
            
        case ALARM_MONTHLY_MODE:
        {
            fgets(line, sizeof(line), f);
            alarm_p->args.day = strtol(&line[strlen("Day: ") ], &ptr, 16);
            break;
        }
            
        case ALARM_YEARLY_MODE:
        {
            fgets(line, sizeof(line), f);
            alarm_p->args.yearly_alarm_args.day = strtol(&line[strlen("Day: ") ], &ptr, 16);

            fgets(line, sizeof(line), f);
            alarm_p->args.yearly_alarm_args.month = strtol(&line[strlen("Month: ") ], &ptr, 16);
            
            break;
        }
    }

    fgets(line, sizeof(line), f);
    alarm_p->volume = strtol(&line[strlen("Volume: ") ], &ptr, 16);
    
    fclose(f);

    return ESP_OK;
}

alarm_mode_args_t get_alarm_values()
{
    return alarm;
}

alarm_mode_args_t* get_alarm_pointer()
{
    return &alarm;
}

uint8_t get_days_to_next_monthly(uint8_t year, uint8_t month)
{
    switch(month)
    {
        case 0:
            return 31;

        case 1:
            if ( ( year % 4 == 0 && year % 100 != 0 ) || ( year % 400 == 0 ) )
            {
                return 29;
            }
            else
            {
                return 28;
            }
            
            return 31;

        case 2:
            return 31;

        case 3:
            return 30;

        case 4:
            return 31;

        case 5:
            return 30;

        case 6:
            return 31;

        case 7:
            return 31;

        case 8:
            return 30;

        case 9:
            return 31;

        case 10:
            return 30;

        case 11:
            return 31;
    }

    return 0;
}

uint16_t get_days_to_next_yearly(uint8_t year)
{
    uint16_t next_year = year + 1;
    if ( ( next_year % 4 == 0 && next_year % 100 != 0 ) || ( next_year % 400 == 0 ) )
    {
        return 366;
    }
    else
    {
        return 365;
    }
}

bool get_alarm_state()
{
    return next_alarm_enabled;
}

uint64_t get_current_active_alarm_id()
{
    return next_alarm_id;
}

void disable_current_alarm()
{
    next_alarm_enabled = false;
    next_alarm_interval = 0;
    next_alarm_id = 0;
}

void set_next_alarm()
{
    alarm_mode_args_t next_alarm;
    object_id_list_t* object_p =  ObjectManager_list_first_elem();
    esp_err_t ret;

    time_t now;
    struct tm timeinfo;

    time(&now);
    localtime_r(&now, &timeinfo);

    if ( object_p == NULL )
    {
        next_alarm_id = 0;
        next_alarm_enabled = false;
        return;
    }

    while ( object_p )
    {
        ret = get_alarm_from_file(object_p->id, &next_alarm);

        if (!ret && next_alarm.enable)
        {
            struct tm tm;
            tm.tm_hour 	= next_alarm.hour;
            tm.tm_min 	= next_alarm.minute;
            tm.tm_sec 	= 0;
            tm.tm_isdst = -1;

            time_t t;
            bool should_check = false;

            switch (next_alarm.mode)
            {
                case ALARM_SINGLE_MODE:
                {
                    tm.tm_year 	= next_alarm.args.single_alarm_args.year + 100;
                    tm.tm_mon 	= next_alarm.args.single_alarm_args.month - 1;
                    tm.tm_mday 	= next_alarm.args.single_alarm_args.day;
                    t = mktime(&tm);

                    if (t < now)
                    {
                        break;
                    }

                    should_check = true;

                    // #ifdef ALARM_LOG
                    // ESP_LOGI(TAG, "Next single time: %02d:%02d:%02d, %02d.%02d.%04d", tm.tm_hour, tm.tm_min, tm.tm_sec, tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900);
                    // ESP_LOGI(TAG, "Now timestamp: %" PRIu64 " alarm timetamp: %" PRIu64, (uint64_t)now, (uint64_t)t);
                    // ESP_LOGI(TAG, "Diff time: %f", difftime(t, now));
                    // #endif

                    break;
                }
                        
                case ALARM_WEEKLY_MODE:
                {
                    tm.tm_year 	= timeinfo.tm_year;
                    tm.tm_mon 	= timeinfo.tm_mon;
                    tm.tm_mday 	= timeinfo.tm_mday;
                    t = mktime(&tm);

                    uint8_t day = timeinfo.tm_wday;
                    if (day == 0) day = 6;
                    else day--;

                    if ( (next_alarm.args.days & ( 1<<day )) && ( next_alarm.hour * 60 + next_alarm.minute > timeinfo.tm_hour * 60 + timeinfo.tm_min ) )
                    {   //same day as current one and later hour
                        should_check = true;
                    }
                    else
                    {
                        uint8_t days_remaining = 1;
                        bool found = false;
                        for ( uint8_t i = day + 1; i < 7; i++, days_remaining++ )
                        {
                            if ( next_alarm.args.days & ( 1<<i ) )
                            {
                                found = true;
                                break;
                            }
                        }

                        if ( found == false )
                        {
                            for ( uint8_t i = 0; i <= day; i++, days_remaining++ )
                            {
                                if ( next_alarm.args.days & ( 1<<i ) )
                                {
                                    found = true;
                                    break;
                                }
                            }
                        }

                        if ( found )
                        {
                            t = t + DAYS_TO_SEC(days_remaining);
                            should_check = true;
                        }
                    }

                    // #ifdef ALARM_LOG
                    // if (should_check)
                    // {
                    //     struct tm weekly_time_info;
                    //     localtime_r(&t, &weekly_time_info);
                    //     ESP_LOGI(TAG, "Next weekly time: %02d:%02d:%02d, %02d.%02d.%04d", weekly_time_info.tm_hour, weekly_time_info.tm_min, weekly_time_info.tm_sec, weekly_time_info.tm_mday, weekly_time_info.tm_mon + 1, weekly_time_info.tm_year + 1900);
                    //     ESP_LOGI(TAG, "Now timestamp: %" PRIu64 " alarm timetamp: %" PRIu64, (uint64_t)now, (uint64_t)t);
                    //     ESP_LOGI(TAG, "Diff time: %f", difftime(t, now));
                    // }
                    // #endif

                    break;
                }
                        
                case ALARM_MONTHLY_MODE:
                {
                    tm.tm_year 	= timeinfo.tm_year;
                    tm.tm_mon 	= timeinfo.tm_mon;
                    tm.tm_mday 	= next_alarm.args.day;
                    t = mktime(&tm);

                    if ( t < now )
                    {
                        t = t + DAYS_TO_SEC(get_days_to_next_monthly(tm.tm_year, tm.tm_mon));
                    }
                    should_check = true;

                    // #ifdef ALARM_LOG
                    // struct tm monthly_time_info;
                    // localtime_r(&t, &monthly_time_info);
                    // ESP_LOGI(TAG, "Next monthly time: %02d:%02d:%02d, %02d.%02d.%04d", monthly_time_info.tm_hour, monthly_time_info.tm_min, monthly_time_info.tm_sec, monthly_time_info.tm_mday, monthly_time_info.tm_mon + 1, monthly_time_info.tm_year + 1900);
                    // ESP_LOGI(TAG, "Now timestamp: %" PRIu64 " alarm timetamp: %" PRIu64, (uint64_t)now, (uint64_t)t);
                    // ESP_LOGI(TAG, "Diff time: %f", difftime(t, now));
                    // #endif

                    break;
                }
                        
                case ALARM_YEARLY_MODE:
                {
                    tm.tm_year 	= timeinfo.tm_year;
                    tm.tm_mon 	= next_alarm.args.yearly_alarm_args.month - 1;
                    tm.tm_mday 	= next_alarm.args.yearly_alarm_args.day;
                    t = mktime(&tm);

                    if ( t < now )
                    {
                        t = t + DAYS_TO_SEC(get_days_to_next_yearly(tm.tm_year));
                    }
                    should_check = true;

                    // #ifdef ALARM_LOG
                    // struct tm yearly_time_info;
                    // localtime_r(&t, &yearly_time_info);
                    // ESP_LOGI(TAG, "Next yearly time: %02d:%02d:%02d, %02d.%02d.%04d", yearly_time_info.tm_hour, yearly_time_info.tm_min, yearly_time_info.tm_sec, yearly_time_info.tm_mday, yearly_time_info.tm_mon + 1, yearly_time_info.tm_year + 1900);
                    // ESP_LOGI(TAG, "Now timestamp: %" PRIu64 " alarm timetamp: %" PRIu64, (uint64_t)now, (uint64_t)t);
                    // ESP_LOGI(TAG, "Diff time: %f", difftime(t, now));
                    // #endif

                    break;
                }
            }

            if ( should_check )
            {
                if ( next_alarm_enabled )
                {
                    time_t new_next_alarm_interval = t - now;
                    if (new_next_alarm_interval < next_alarm_interval)
                    {
                        next_alarm_interval = new_next_alarm_interval;
                        next_alarm_id = object_p->id;
                    }
                }
                else
                {
                    next_alarm_enabled = true;
                    next_alarm_interval = t - now;
                    next_alarm_id = object_p->id;
                }
            }
        }

        object_p = object_p->next;
    }

    #ifdef ALARM_LOG
    if (next_alarm_enabled)
    {
        ESP_LOGI(TAG, "Next alarm ID: %" PRIx64, next_alarm_id);
        ESP_LOGI(TAG, "Next alarm interval: %" PRIu64, (uint64_t)next_alarm_interval);
    }
    #endif
}