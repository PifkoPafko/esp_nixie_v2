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
#include "pp_alarm.h"

/* Macros */
#define TAG "ALARM"

/* Declarations */
static bool pp_alarm_timer_cb(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_data);
static void pp_alarm_main(void* arg);
void static pp_set_timer_for_playing_alarm(void);
void static pp_disable_current_alarm(void);
uint8_t static pp_get_days_to_next_monthly(uint8_t year, uint8_t month);
uint16_t static pp_get_days_to_next_yearly(uint8_t year);

/* Variables */
static i2s_chan_handle_t tx_handle;
static gptimer_handle_t alarm_timer = NULL;

static uint64_t next_alarm_id = 0;
static bool next_alarm_enabled = false;
static time_t next_alarm_interval = 0;

/* Functions */

/** @brief pp_alarm_timer_cb: Interruption callback for alarm timer
 * 
 *  Notifies main alarm task pp_alarm_main that the time interval has passed. This timer is used to measure
 *  the interval to the next alarm and also the interval of current alarm playing.
 * 
 *  @param[in]  timer     (gptimer_handle_t)             Handle to the called Timer
 *  @param[in]  edata     (gptimer_alarm_event_data_t *) Pointer to alarm event data
 *  @param[in]  user_data (void *)                       Pointer to user data (not used)
 * 
 * @return Whether a high priority task has been waken up by this function
 */
static bool pp_alarm_timer_cb(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_data)
{
    xTaskNotifyFromISR(alarm_main_h, ALARM_TIMER_NOTIFICATION, eNoAction, NULL);
    return false;
}

/** @brief pp_alarm_init: Alarm player initialization function
 * 
 * This functions initializes i2s functionality and creates a task that manages alarm playing. It also initializes
 * the timer used to measure interval to next alarm or current playing alarm remaining playing time.
 * The timer is set to notify the alarm task when the next alarm should be played.
 * 
 * @return
 */
void pp_alarm_init(void)
{
    ESP_LOGI(TAG, "Initializing i2s");

    // setup a standard config and the channel
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &tx_handle, NULL));

    // setup the i2s config
    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(44100),                                                    // the wav file sample rate
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO), // the wav faile bit and channel config
        .gpio_cfg = {
            // refer to configuration.h for pin setup
            .mclk = I2S_GPIO_UNUSED,
            .bclk = GPIO_NUM_10,
            .ws = GPIO_NUM_11,
            .dout = GPIO_NUM_9,
            .din = I2S_GPIO_UNUSED,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };

    ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx_handle, &std_cfg));

    ESP_LOGI(TAG, "Initializing wave player");
    BaseType_t res = xTaskCreate(pp_alarm_main, "WAV PLAYER", 8192, NULL, 1, &alarm_main_h);
    if(res != pdPASS)
    {
        ESP_ERROR_CHECK(ESP_FAIL);
    }

    current_alarm.is_set = false;

    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1000000, // 1000000Hz, 1 tick=1us
    };

    gptimer_new_timer(&timer_config, &alarm_timer);

     gptimer_event_callbacks_t cbs = {
        .on_alarm = pp_alarm_timer_cb,
    };

    gptimer_register_event_callbacks(alarm_timer, &cbs, NULL);

    ESP_LOGI(TAG, "Enable alarm timer");
    gptimer_enable(alarm_timer);

    pp_set_next_alarm();
}

/** @brief pp_alarm_main: Alarm Main Task
 * 
 * The task executing this function is created during alarm player initialization. After creation, the task is waiting for notification to take an action.
 * 
 * There are 4 types of notifications:
 *  - ALARM_TIMER_NOTIFICATION           : Action requested by alarm timer. It notifies to start or to stop playing the alarm.
 *  - ALARM_START_NOTIFICATION           : Action requested by user to play the alarm.
 *  - ALARM_STOP_NOTIFICATION            : Action requested by user to stop the alarm.
 * 
 *  The task updates the alarm state machine according to the received notification and current state.
 *  Playing of the alarm is managed by the state machine with the following states:
 * 
 *  - WAIT_FOR_START_PLAY       : Wiating for notification to start playing the alarm (user action or timer callback).
 *  - SET_RINGTONE              : Opens the desired ringtone file.
 *  - SET_DATA_POSITION         : Setting carriage in file to the start of sound data and sets timer for playing the alarm (300 seconds).
 *  - ALARM_PLAY                : Writing the data to I2S to play the alarm. When it reaches the end of the sound file before the alarm timeout
 *                                then it goes back to the SET_DATA_POSITION and starts playing the same ringtone again.
 *  - SET_NEXT_ALARM            : Disables the playing of the alarm and sets the timer for the next alarm.
 * 
 * @param[in]   arg  (void*) Reserved
 * 
 * @return
 */
static void pp_alarm_main(void* arg)
{
    alarm_play_sm_t alarm_play_sm = WAIT_FOR_START_PLAY;
    BaseType_t got_notify = pdFALSE;
    uint32_t notify_value = 0xFF;

    // create a writer buffer
    FILE *fh = NULL;
    int16_t buf[AUDIO_BUFFER];
    size_t bytes_read = 0;
    size_t bytes_written = 0;

    while (true)
    {
        notify_value = 0xFF;
        if(alarm_play_sm == WAIT_FOR_START_PLAY)
        {
            got_notify = xTaskNotifyWait(0, 0, &notify_value, portMAX_DELAY);
            if(got_notify != pdTRUE) continue;
        }
        else
        {
            got_notify = xTaskNotifyWait(0, 0, &notify_value, 1);
        }

        switch(alarm_play_sm)
        {
            case WAIT_FOR_START_PLAY:
            {
                if(got_notify == pdTRUE && (notify_value == ALARM_TIMER_NOTIFICATION || notify_value == ALARM_START_NOTIFICATION))
                {
                    ESP_LOGI(TAG, "ANY MODE -> ALARM_RING_MODE");
                    device_mode = ALARM_RING_MODE;
                    ESP_LOGI(TAG, "WAIT_FOR_START_PLAY -> SET_RINGTONE");
                    alarm_play_sm = SET_RINGTONE;
                    pp_update_display();
                }
                break;
            }

            case SET_RINGTONE:
            {
                if(got_notify == pdTRUE && (notify_value == ALARM_TIMER_NOTIFICATION || notify_value == ALARM_STOP_NOTIFICATION))
                {
                    ESP_LOGI(TAG, "SET_RINGTONE -> SET_NEXT_ALARM");
                    alarm_play_sm = SET_NEXT_ALARM;
                    break;
                }

                fh = fopen(BARKA_WAV_16_TRIM, "r");
                if (fh == NULL)
                {
                    ESP_LOGE(TAG, "Failed to open file");
                    ESP_LOGI(TAG, "SET_RINGTONE -> WAIT_FOR_START_PLAY");
                    alarm_play_sm = SET_NEXT_ALARM;
                    break;
                }
                
                ESP_LOGI(TAG, "SET_RINGTONE -> SET_DATA_POSITION");
                alarm_play_sm = SET_DATA_POSITION;
                break;
            }

            case SET_DATA_POSITION:
            {
                if(got_notify == pdTRUE && (notify_value == ALARM_TIMER_NOTIFICATION || notify_value == ALARM_STOP_NOTIFICATION))
                {
                    ESP_LOGI(TAG, "SET_DATA_POSITION -> SET_NEXT_ALARM");
                    alarm_play_sm = SET_NEXT_ALARM;
                    break;
                }

                // skip the header...
                fseek(fh, 44, SEEK_SET);

                i2s_channel_enable(tx_handle);
                pp_set_timer_for_playing_alarm();

                ESP_LOGI(TAG, "SET_DATA_POSITION -> ALARM_PLAY");
                alarm_play_sm = ALARM_PLAY;
                break;
            }

            case ALARM_PLAY:
            {
                if(got_notify == pdTRUE && (notify_value == ALARM_TIMER_NOTIFICATION || notify_value == ALARM_STOP_NOTIFICATION))
                {
                    ESP_LOGI(TAG, "ALARM_PLAY -> SET_NEXT_ALARM");
                    alarm_play_sm = SET_NEXT_ALARM;
                    break;
                }

                bytes_read = fread(buf, sizeof(int16_t), AUDIO_BUFFER, fh);
                // ESP_LOGI(TAG, "Read bytes = %d", bytes_read);
                for (int i=0; i < bytes_read; i++)
                {
                    buf[i] = buf[i]>>1;
                }

                if(bytes_read > 0)
                {
                    // write the buffer to the i2s
                    esp_err_t ret = i2s_channel_write(tx_handle, buf, bytes_read * sizeof(int16_t), &bytes_written, portMAX_DELAY);
                    // ESP_LOGI(TAG, "ret = %d, bytes_written = %d", ret, bytes_written);
                }
                else
                {
                    ESP_LOGI(TAG, "ALARM_PLAY -> SET_DATA_POSITION");
                    alarm_play_sm = SET_DATA_POSITION;
                }
                break;
            }

            case SET_NEXT_ALARM:
            {
                i2s_channel_disable(tx_handle);
                fclose(fh);
                ESP_LOGI(TAG, "SET_NEXT_ALARM -> WAIT_FOR_START_PLAY");
                alarm_play_sm = WAIT_FOR_START_PLAY;

                device_mode = DEFAULT_MODE;
                pp_set_next_alarm();
            }
        }
    }
}

/** @brief pp_get_days_to_next_monthly: Calculates the number of days to the next monthly alarm.
 * 
 * This function calculates the number of days to the next monthly alarm from current time and provided month and year.
 * 
 * @param[in]   year    (uint8_t) Year
 * @param[in]   month   (uint8_t) Month
 * 
 * @return (uint8_t) Number of days.
 */
uint8_t static pp_get_days_to_next_monthly(uint8_t year, uint8_t month)
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

/** @brief pp_get_days_to_next_yearly: Calculates the number of days to the next yearly alarm.
 * 
 * This function calculates the number of days to the next yearly alarm from current time and provided year.
 * 
 * @param[in]   year    (uint8_t) Year
 * 
 * @return (uint16_t) Number of days.
 */
uint16_t static pp_get_days_to_next_yearly(uint8_t year)
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

/** @brief pp_disable_current_alarm: Disables current alarm and alarm timer.
 * 
 * This function disables current alarm and alarm timer.
 * 
 * @return (void)
 */
void static pp_disable_current_alarm(void)
{
    gptimer_stop(alarm_timer);
    gptimer_set_raw_count(alarm_timer, 0);

    next_alarm_enabled = false;
    next_alarm_interval = 0;
    next_alarm_id = 0;
}

/** @brief pp_set_timer_for_playing_alarm: Set the alarm Timer to play the alarm.
 * 
 * This function sets the Timer to wait 300 seconds when the alarm is played.
 * 
 * @return (void)
 */
void static pp_set_timer_for_playing_alarm(void)
{
    pp_disable_current_alarm();
    gptimer_alarm_config_t alarm_config = {
        .alarm_count = 300000000,
        .reload_count = 0,
        .flags.auto_reload_on_alarm = false
    };

    gptimer_set_alarm_action(alarm_timer, &alarm_config);
    gptimer_start(alarm_timer);
}

/** @brief pp_set_alarm_values: Set the current alarm values.
 * 
 * This functions sets the values of the current alarm global variable with the bluetooth payload data stream.
 * 
 * @param[in]   payload      (uint8_t *) Write bluetooth payload.
 * @param[in]   payload_len  (uint16_t)  Payload length.
 * 
 * @return (otp_rsp_status_t) Operation result code.
 */
otp_rsp_status_t pp_set_alarm_values(uint8_t *payload, uint16_t payload_len)
{
    if((payload_len < ALARM_MODE_PAYLOAD_SIZE_MIN || payload_len > ALARM_MODE_PAYLOAD_SIZE_MAX))
    {
        ESP_LOGE(TAG, "Wrong Alarm Length 1");
        return INVALID_ATTR_VAL_LENGTH;
    }

    current_alarm.mode = *payload;
    payload += ALARM_FIELD_SIZE;

    current_alarm.enable = *payload;
    payload += ALARM_FIELD_SIZE;

    current_alarm.desc_len = *payload;
    payload += ALARM_FIELD_SIZE;

    if(current_alarm.mode >= ALARM_MODES_NUM)
    {
        ESP_LOGE(TAG, "Wrong Alarm Mode");
        return WRITE_REQUEST_REJECTED;
    }

    if(current_alarm.enable > 1)
    {
        ESP_LOGE(TAG, "Wrong Alarm Enable value");
        return WRITE_REQUEST_REJECTED;
    }
    
    if(current_alarm.desc_len > ALARM_DESC_LEN_MAX)
    {
        ESP_LOGE(TAG, "Wrong Alarm Enable value");
        return WRITE_REQUEST_REJECTED;
    }

    switch(current_alarm.mode) 
    {
        case ALARM_SINGLE_MODE:
        {
            if (payload_len != ALARM_MODE_SINGLE_PAYLOAD_SIZE_MIN + current_alarm.desc_len ) 
            {
                ESP_LOGE(TAG, "Wrong Alarm Length 2");
                return INVALID_ATTR_VAL_LENGTH;
            }
            break;
        }
            
        case ALARM_WEEKLY_MODE:
        {
            if (payload_len != ALARM_MODE_WEEKLY_PAYLOAD_SIZE_MIN + current_alarm.desc_len ) 
            {
                ESP_LOGE(TAG, "Wrong Alarm Length 3");
                return INVALID_ATTR_VAL_LENGTH;
            }
            break;
        }
            
        case ALARM_MONTHLY_MODE:
        {
            if (payload_len != ALARM_MODE_MONTHLY_PAYLOAD_SIZE_MIN + current_alarm.desc_len ) 
            {
                ESP_LOGE(TAG, "Wrong Alarm Length 4");
                return INVALID_ATTR_VAL_LENGTH;
            }
            break;
        }
            
        case ALARM_YEARLY_MODE:
        {
            if (payload_len != ALARM_MODE_YEARLY_PAYLOAD_SIZE_MIN + current_alarm.desc_len ) 
            {
                ESP_LOGE(TAG, "Wrong Alarm Length 5");
                return INVALID_ATTR_VAL_LENGTH;
            }
            break;
        }
    }
    
    memcpy((uint8_t*)current_alarm.desc, payload, current_alarm.desc_len);
    current_alarm.desc[current_alarm.desc_len] = '\0';
    payload += current_alarm.desc_len;

    current_alarm.hour = *payload;
    payload += ALARM_FIELD_SIZE;

    current_alarm.minute = *payload;
    payload += ALARM_FIELD_SIZE;

    switch(current_alarm.mode) 
    {
        case ALARM_SINGLE_MODE:
        {
            current_alarm.args.single_alarm_args.day = *payload;
            payload += ALARM_FIELD_SIZE;

            current_alarm.args.single_alarm_args.month = *payload;
            payload += ALARM_FIELD_SIZE;

            current_alarm.args.single_alarm_args.year = *payload;
            payload += ALARM_FIELD_SIZE;

            break;
        }
            
        case ALARM_WEEKLY_MODE:
        {
            current_alarm.args.days = *payload;
            payload += ALARM_FIELD_SIZE;
            break;
        }
            
        case ALARM_MONTHLY_MODE:
        {
            current_alarm.args.day = *payload;
            payload += ALARM_FIELD_SIZE;
            break;
        }
            

        case ALARM_YEARLY_MODE:
        {
            current_alarm.args.yearly_alarm_args.day = *payload;
            payload += ALARM_FIELD_SIZE;

            current_alarm.args.yearly_alarm_args.month = *payload;
            payload += ALARM_FIELD_SIZE;
            break;
        }
    }

    current_alarm.volume = *payload;

    if(current_alarm.volume > ALARM_VOLUME_MAX)
    {
        ESP_LOGE(TAG, "Wrong Volume value");
        return WRITE_REQUEST_REJECTED;
    }

    current_alarm.is_set = true;

    return STATUS_OK;
}

/** @brief pp_set_next_alarm: Set the timer for the next alarm.
 * 
 * This function checks all the alarm objects and calculate the time to the nearest alarm. This function
 * takes into consideratation the mode of the alarm (single, weekly, monthly and yearly). This function sets the Timer
 * with the interval to the next alarm. The Timer is disabled if there is no alarm enabled.
 * 
 * @return (void)
 */
void pp_set_next_alarm(void)
{
    uint8_t quantity = pp_object_list_get_how_many();
    ESP_LOGI(TAG, "File count = %d", quantity);
    
    object_id_array_t *object_array = pp_object_list_get_objects_array();
    alarm_mode_args_t next_alarm;
    esp_err_t ret;

    time_t now;
    struct tm timeinfo;

    time(&now);
    localtime_r(&now, &timeinfo);
    pp_disable_current_alarm();

    if( quantity == 0 )
    {
        ESP_LOGI(TAG, "No objects in the list");
        return;
    }

    for( int i = 0; i < quantity; ++i )
    {
        if(object_array[i].type != ALARM_TYPE)
        {
            ESP_LOGI(TAG, "File ID = %lld, Type = %d SKIPPED", object_array[i].id, object_array[i].type);
            continue;
        }

        ESP_LOGI(TAG, "Set Next Alarm Check object ID: %lld", object_array[i].id);
        ret = pp_object_manager_get_alarm_data_from_file(object_array[i].id, &next_alarm);

        if (ret == ESP_OK && next_alarm.enable)
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

                    #ifdef ALARM_LOG
                    ESP_LOGI(TAG, "Next single time: %02d:%02d:%02d, %02d.%02d.%04d", tm.tm_hour, tm.tm_min, tm.tm_sec, tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900);
                    ESP_LOGI(TAG, "Now timestamp: %" PRIu64 " alarm timetamp: %" PRIu64, (uint64_t)now, (uint64_t)t);
                    ESP_LOGI(TAG, "Diff time: %f", difftime(t, now));
                    #endif

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

                    #ifdef ALARM_LOG
                    if (should_check)
                    {
                        struct tm weekly_time_info;
                        localtime_r(&t, &weekly_time_info);
                        ESP_LOGI(TAG, "Next weekly time: %02d:%02d:%02d, %02d.%02d.%04d", weekly_time_info.tm_hour, weekly_time_info.tm_min, weekly_time_info.tm_sec, weekly_time_info.tm_mday, weekly_time_info.tm_mon + 1, weekly_time_info.tm_year + 1900);
                        ESP_LOGI(TAG, "Now timestamp: %" PRIu64 " alarm timetamp: %" PRIu64, (uint64_t)now, (uint64_t)t);
                        ESP_LOGI(TAG, "Diff time: %f", difftime(t, now));
                    }
                    #endif

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
                        t = t + DAYS_TO_SEC(pp_get_days_to_next_monthly(tm.tm_year, tm.tm_mon));
                    }
                    should_check = true;

                    #ifdef ALARM_LOG
                    struct tm monthly_time_info;
                    localtime_r(&t, &monthly_time_info);
                    ESP_LOGI(TAG, "Next monthly time: %02d:%02d:%02d, %02d.%02d.%04d", monthly_time_info.tm_hour, monthly_time_info.tm_min, monthly_time_info.tm_sec, monthly_time_info.tm_mday, monthly_time_info.tm_mon + 1, monthly_time_info.tm_year + 1900);
                    ESP_LOGI(TAG, "Now timestamp: %" PRIu64 " alarm timetamp: %" PRIu64, (uint64_t)now, (uint64_t)t);
                    ESP_LOGI(TAG, "Diff time: %f", difftime(t, now));
                    #endif

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
                        t = t + DAYS_TO_SEC(pp_get_days_to_next_yearly(tm.tm_year));
                    }
                    should_check = true;

                    #ifdef ALARM_LOG
                    struct tm yearly_time_info;
                    localtime_r(&t, &yearly_time_info);
                    ESP_LOGI(TAG, "Next yearly time: %02d:%02d:%02d, %02d.%02d.%04d", yearly_time_info.tm_hour, yearly_time_info.tm_min, yearly_time_info.tm_sec, yearly_time_info.tm_mday, yearly_time_info.tm_mon + 1, yearly_time_info.tm_year + 1900);
                    ESP_LOGI(TAG, "Now timestamp: %" PRIu64 " alarm timetamp: %" PRIu64, (uint64_t)now, (uint64_t)t);
                    ESP_LOGI(TAG, "Diff time: %f", difftime(t, now));
                    #endif

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
                        next_alarm_id = object_array[i].id;
                    }
                }
                else
                {
                    next_alarm_enabled = true;
                    next_alarm_interval = t - now;
                    next_alarm_id = object_array[i].id;
                }
            }
        }
    }

    if (next_alarm_enabled)
    {
        ESP_LOGI(TAG, "Next alarm ID: %" PRIx64, next_alarm_id);
        ESP_LOGI(TAG, "Next alarm interval: %" PRIu64 " sec", (uint64_t)next_alarm_interval);

        gptimer_stop(alarm_timer);
        gptimer_set_raw_count(alarm_timer, 0);

        gptimer_alarm_config_t alarm_config = {
            .alarm_count = (uint64_t)(next_alarm_interval * 1000000),
            .reload_count = 0,
            .flags.auto_reload_on_alarm = false
        };

        gptimer_set_alarm_action(alarm_timer, &alarm_config);
        gptimer_start(alarm_timer);
    }
    else
    {
        pp_disable_current_alarm();
        ESP_LOGI(TAG, "No enabled alarm to be set");
    }
}