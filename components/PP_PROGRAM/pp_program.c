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
#include "pp_program.h"

/* Macros */
#define PROGRAM_TAG "PROGRAM"

static QueueHandle_t button_action_queue;

static pairing_sm_t pairing_sm = WAIT_FOR_LEFT;
static time_change_sm_t time_change_sm = IDLE_TIME_CHANGE;
static alarm_add_sm_t alarm_add_sm = IDLE_ALARM_ADD;

static display_digits_t display_digits;
static alarm_mode_args_t alarm_add;

extern uint8_t alarm_type_uuid[ESP_UUID_LEN_128];

void pp_program_init(void)
{
    ESP_LOGI(PROGRAM_TAG, "Initializing program");
    button_action_queue = xQueueCreate(10, sizeof(button_action_t));
    pp_gpio_init(button_action_queue);
    pp_display_manager_init(&display_digits);
    pp_program_main();
}

void pp_program_main(void)
{
    while(true)
    {
        button_action_t action;
        if(xQueueReceive(button_action_queue, &action, portMAX_DELAY) != pdTRUE) continue;

        pp_button_functions(action);
    }
}

static void pp_button_functions(button_action_t action_handler)
{
    switch(device_mode)
    {
        case DEFAULT_MODE:
        {
            switch(pairing_sm)
            {
                case WAIT_FOR_LEFT:
                {
                    if (action_handler.button == BUTTON_LEFT && action_handler.action == HOLDING_SHORT)
                    {
                        pairing_sm = WAIT_FOR_CENTER;
                    }
                    else
                    {
                        pairing_sm = WAIT_FOR_LEFT;
                    }
                    break;
                }

                case WAIT_FOR_CENTER:
                {
                    if (action_handler.button == BUTTON_CENTER && action_handler.action == HOLDING_SHORT)
                    {
                        pairing_sm = WAIT_FOR_LEFT_LONG;
                    }
                    else
                    {
                        pairing_sm = WAIT_FOR_LEFT;
                    }
                    break;
                }

                case WAIT_FOR_LEFT_LONG:
                {
                    if (action_handler.button == BUTTON_LEFT && action_handler.action == LONG_PRESS)
                    {
                        pairing_sm = WAIT_FOR_CENTER_LONG;
                    }
                    else
                    {
                        pairing_sm = WAIT_FOR_LEFT;
                    }
                    break;
                }

                case WAIT_FOR_CENTER_LONG:
                {
                    if (action_handler.button == BUTTON_CENTER && action_handler.action == LONG_PRESS)
                    {
                        ESP_LOGI(MAIN_TAG, "DEFAULT MODE -> PAIRING MODE");
                        pairing_sm = PAIRING;

                        device_mode = PAIRING_MODE;
                    }
                    else
                    {
                        pairing_sm = WAIT_FOR_LEFT;
                    }
                    break;
                }

                default:
                    break;
            }

            if (action_handler.button == BUTTON_LEFT && action_handler.action == LONG_PRESS && pairing_sm == WAIT_FOR_LEFT)
            {
                ESP_LOGI(MAIN_TAG, "DEFAULT MODE -> TIME CHANGE MODE");
                device_mode = TIME_CHANGE_MODE;
                pp_time_change_mode(action_handler, true);
            }

            if (action_handler.button == BUTTON_CENTER && action_handler.action == LONG_PRESS && pairing_sm != PAIRING)
            {
                ESP_LOGI(MAIN_TAG, "DEFAULT MODE -> ALARM ADD MODE");
                device_mode = ALARM_ADD_MODE;
                pp_alarm_add_mode(action_handler, true);
            }

            if (action_handler.button == BUTTON_RIGHT && action_handler.action == LONG_PRESS)
            {
                olcp_op_code_result_t result;
                pp_object_manager_first_object(&result);

                if (result == OLCP_RES_SUCCESS)
                {
                    object_t* cur_obj = pp_object_manager_get_object();
                    bool result_flag = true;

                    while (cur_obj->set_custom_object == false)
                    {
                        pp_object_manager_next_object(&result);
                        if (result == OLCP_RES_SUCCESS)
                        {
                            cur_obj = pp_object_manager_get_object();
                        }
                        else
                        {
                            result_flag = false;
                            break;
                        }
                    }

                    if (result_flag)
                    {
                        ESP_LOGI(MAIN_TAG, "DEFAULT MODE -> ALARM_DELETE_MODE");
                        device_mode = ALARM_DELETE_MODE;
                        pp_set_current_alarm_digits();
                    }
                    else
                    {
                        ESP_LOGI(MAIN_TAG, "NO ALARMS");
                    }
                }
                else
                {
                    ESP_LOGI(MAIN_TAG, "NO FILES");
                }
            }

            if (pairing_sm == PAIRING)
            {
                pairing_sm = WAIT_FOR_LEFT;
            }
            break;
        }

        case TIME_CHANGE_MODE:
        {
            pp_time_change_mode(action_handler, false);
            break;
        }

        case ALARM_ADD_MODE:
        {
            pp_alarm_add_mode(action_handler, false);
            break;
        }

        case ALARM_DELETE_MODE:
        {
            pp_alarm_delete_mode(action_handler);
            break;
        }

        case PAIRING_MODE:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                ESP_LOGI(MAIN_TAG, "PAIRING MODE -> DEFAULT MODE");
                device_mode = DEFAULT_MODE;
            }  
            break;
        }

        case ALARM_RING_MODE:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                ESP_LOGI(MAIN_TAG, "ALARM DISABLED");
                ESP_LOGI(MAIN_TAG, "ALARM RING MODE -> DEFAULT MODE");
                device_mode = DEFAULT_MODE;
                break;
            }
            break;
        }
    }
}

void pp_time_change_mode(button_action_t action_handler, bool start)
{
    if (action_handler.action == SHORT_PRESS && action_handler.button == BUTTON_RIGHT)
    {
        ESP_LOGI(MAIN_TAG, "TIME CHANGE MODE -> DEFAULT MODE");
        time_change_sm = IDLE_TIME_CHANGE;
        device_mode = DEFAULT_MODE;
    }

    switch(time_change_sm)
    {
        case IDLE_TIME_CHANGE:
        {
            if (start)
            {
                time_t now;
                time(&now);
                struct tm timeinfo;
                localtime_r(&now, &timeinfo);

                nixie_time.hour_first = timeinfo.tm_hour / 10;
                nixie_time.hour_second = timeinfo.tm_hour % 10;
                nixie_time.minute_first = timeinfo.tm_min / 10;
                nixie_time.minute_second = timeinfo.tm_min % 10;
                nixie_time.second_first = timeinfo.tm_sec / 10;
                nixie_time.second_second = timeinfo.tm_sec % 10;
                nixie_time.day_first = timeinfo.tm_mday / 10;
                nixie_time.day_second = timeinfo.tm_mday % 10;
                nixie_time.month_first = (timeinfo.tm_mon + 1) / 10;
                nixie_time.month_second = (timeinfo.tm_mon + 1) % 10;
                nixie_time.year_first = (timeinfo.tm_year - 100) / 10;
                nixie_time.year_second = (timeinfo.tm_year - 100) % 10;

                time_change_sm = SET_HOUR_FIRST;
            }    
            break;
        }

        case SET_HOUR_FIRST:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                switch (action_handler.button)
                {
                    case BUTTON_LEFT:
                    {
                        nixie_time.hour_first++;
                        if (nixie_time.hour_first > 2) nixie_time.hour_first = 0;
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        time_change_sm = SET_HOUR_SECOND;
                        break;
                    }

                    default:
                        break;
                }
            }
            break;
        }

        case SET_HOUR_SECOND:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                switch (action_handler.button)
                {
                    case BUTTON_LEFT:
                    {
                        nixie_time.hour_second++;
                        if ( (nixie_time.hour_first < 2 && nixie_time.hour_second > 9) || (nixie_time.hour_first == 2 && nixie_time.hour_second > 3) )
                        {
                            nixie_time.hour_second = 0;
                        }
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        time_change_sm = SET_MINUTE_FIRST;
                        break;
                    }

                    default:
                        break;
                }
            }
            break;
        }

        case SET_MINUTE_FIRST:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                switch (action_handler.button)
                {
                    case BUTTON_LEFT:
                    {
                        nixie_time.minute_first++;
                        if (nixie_time.minute_first > 5) nixie_time.minute_first = 0;
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        time_change_sm = SET_MINUTE_SECOND;
                        break;
                    }

                    default:
                        break;
                }
            }
            break;
        }
        
        case SET_MINUTE_SECOND:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                switch (action_handler.button)
                {
                    case BUTTON_LEFT:
                    {
                        nixie_time.minute_second++;
                        if (nixie_time.minute_second > 9) nixie_time.minute_second = 0;
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        time_change_sm = SET_SECOND_FIRST;
                        break;
                    }

                    default:
                        break;
                }
            }
            break;
        }

        case SET_SECOND_FIRST:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                switch (action_handler.button)
                {
                    case BUTTON_LEFT:
                    {
                        nixie_time.second_first++;
                        if (nixie_time.second_first > 5) nixie_time.second_first = 0;
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        time_change_sm = SET_SECOND_SECOND;
                        break;
                    }

                    default:
                        break;
                }
            }
            break;
        }
        
        case SET_SECOND_SECOND:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                switch (action_handler.button)
                {
                    case BUTTON_LEFT:
                    {
                        nixie_time.second_second++;
                        if (nixie_time.second_second > 9) nixie_time.second_second = 0;
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        time_change_sm = SET_DAY_FIRST;
                        break;
                    }

                    default:
                        break;
                }
            }
            break;
        }

        case SET_DAY_FIRST:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                switch (action_handler.button)
                {
                    case BUTTON_LEFT:
                    {
                        nixie_time.day_first++;
                        if (nixie_time.day_first > 3) nixie_time.day_first = 0;
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        time_change_sm = SET_DAY_SECOND;
                        break;
                    }

                    default:
                        break;
                }
            }
            break;
        }

        case SET_DAY_SECOND:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                switch (action_handler.button)
                {
                    case BUTTON_LEFT:
                    {
                        nixie_time.day_second++;

                        if ( (nixie_time.day_first > 2 && nixie_time.day_second > 1) || (nixie_time.day_first < 2 && nixie_time.day_second > 9) )
                        {
                            nixie_time.day_second = 0;
                        }
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        time_change_sm = SET_MONTH_FIRST;
                        break;
                    }

                    default:
                        break;
                }
            }
            break;
        }

        case SET_MONTH_FIRST:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                switch (action_handler.button)
                {
                    case BUTTON_LEFT:
                    {
                        nixie_time.month_first++;
                        if (nixie_time.month_first > 1) nixie_time.month_first = 0;
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        time_change_sm = SET_MONTH_SECOND;
                        break;
                    }

                    default:
                        break;
                }
            }
            break;
        }

        case SET_MONTH_SECOND:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                switch (action_handler.button)
                {
                    case BUTTON_LEFT:
                    {
                        nixie_time.month_second++;
                        if ( (nixie_time.month_first > 0 && nixie_time.month_second > 2) || (nixie_time.month_first == 0 && nixie_time.month_second > 9) )
                        {
                            nixie_time.month_second = 0;
                        }
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        time_change_sm = SET_YEAR_FIRST;
                        break;
                    }

                    default:
                        break;
                }
            }
            break;
        }

        case SET_YEAR_FIRST:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                switch (action_handler.button)
                {
                    case BUTTON_LEFT:
                    {
                        nixie_time.year_first++;
                        if (nixie_time.year_first > 9) nixie_time.year_first = 0;
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        time_change_sm = SET_YEAR_SECOND;
                        break;
                    }

                    default:
                        break;
                }
            }
            break;
        }

        case SET_YEAR_SECOND:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                switch (action_handler.button)
                {
                    case BUTTON_LEFT:
                    {
                        nixie_time.year_second++;
                        if (nixie_time.year_second > 9) nixie_time.year_second = 0;
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        time_change_sm = IDLE_TIME_CHANGE;
                        device_mode = DEFAULT_MODE;

                        struct tm timeinfo;
                        timeinfo.tm_sec = nixie_time.second_first * 10 + nixie_time.second_second;
                        timeinfo.tm_min = nixie_time.minute_first * 10 + nixie_time.minute_second;
                        timeinfo.tm_hour = nixie_time.hour_first * 10 + nixie_time.hour_second;
                        timeinfo.tm_mday = nixie_time.day_first * 10 + nixie_time.day_second;
                        timeinfo.tm_mon = nixie_time.month_first * 10 + nixie_time.month_second - 1;
                        timeinfo.tm_year = nixie_time.year_first * 10 + nixie_time.year_second + 100;
                        timeinfo.tm_isdst = -1;

                        time_t t = mktime(&timeinfo);
                        struct timeval tv;
                        tv.tv_sec = t;
                        settimeofday(&tv, NULL);
                        
                        timeinfo.tm_wday = 0;
                        timeinfo.tm_yday = 0;
                        pp_rtc_set_time(&timeinfo);
                        break;
                    }

                    default:
                        break;
                }
            }
            break;
        }    
    }
}

void pp_alarm_add_mode(button_action_t action_handler, bool start)
{
    if (action_handler.action == SHORT_PRESS && action_handler.button == BUTTON_RIGHT)
    {
        ESP_LOGI(MAIN_TAG, "ALARM_ADD_MODE -> DEFAULT MODE");
        alarm_add_sm = IDLE_ALARM_ADD;
        device_mode = DEFAULT_MODE;
    }

    switch(alarm_add_sm)
    {
        case IDLE_ALARM_ADD:
        {
            if (start)
            {
                time_t now;
                time(&now);

                struct tm alarm_add_timeinfo;
                localtime_r(&now, &alarm_add_timeinfo);

                alatm_add_digits.mode = ALARM_SINGLE_MODE;

                alatm_add_digits.time.hour_first = 1;
                alatm_add_digits.time.hour_second = 2;
                alatm_add_digits.time.minute_first = 0;
                alatm_add_digits.time.minute_second = 0;

                alatm_add_digits.time.day_first = alarm_add_timeinfo.tm_mday / 10;
                alatm_add_digits.time.day_second = alarm_add_timeinfo.tm_mday % 10;
                alatm_add_digits.time.month_first = (alarm_add_timeinfo.tm_mon + 1) / 10;
                alatm_add_digits.time.month_second = (alarm_add_timeinfo.tm_mon + 1) % 10;
                alatm_add_digits.time.year_first = (alarm_add_timeinfo.tm_year - 100) / 10;
                alatm_add_digits.time.year_second = (alarm_add_timeinfo.tm_year - 100) % 10;

                alatm_add_digits.monday = 0;
                alatm_add_digits.tuesday = 0;
                alatm_add_digits.wednesday = 0;
                alatm_add_digits.thursday = 0;
                alatm_add_digits.friday = 0;
                alatm_add_digits.saturday = 0;
                alatm_add_digits.sunday = 0;

                alatm_add_digits.volume = 9;

                memset(&alarm_add, 0, sizeof(alarm_add));
                alarm_add_sm = SET_MODE;
            }    
            break;
        }

        case SET_MODE:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                switch (action_handler.button)
                {
                    case BUTTON_LEFT:
                    {
                        alatm_add_digits.mode++;
                        if (alatm_add_digits.mode > 3) alatm_add_digits.mode = 0;
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        alarm_add_sm = SET_ALARM_HOUR_FIRST;
                        break;
                    }

                    default:
                        break;
                }
            }
            break;
        }

        case SET_ALARM_HOUR_FIRST:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                switch (action_handler.button)
                {
                    case BUTTON_LEFT:
                    {
                        alatm_add_digits.time.hour_first++;
                        if (alatm_add_digits.time.hour_first > 2) alatm_add_digits.time.hour_first = 0;
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        alarm_add_sm = SET_ALARM_HOUR_SECOND;
                        break;
                    }

                    default:
                        break;
                }
            }
            break;
        }

        case SET_ALARM_HOUR_SECOND:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                switch (action_handler.button)
                {
                    case BUTTON_LEFT:
                    {
                        alatm_add_digits.time.hour_second++;
                        if ( (alatm_add_digits.time.hour_first < 2 && alatm_add_digits.time.hour_second > 9) || (alatm_add_digits.time.hour_first == 2 && alatm_add_digits.time.hour_second > 3) )
                        {
                            alatm_add_digits.time.hour_second = 0;
                        }
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        alarm_add_sm = SET_ALARM_MINUTE_FIRST;
                        break;
                    }

                    default:
                        break;
                }
            }
            break;
        }

        case SET_ALARM_MINUTE_FIRST:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                switch (action_handler.button)
                {
                    case BUTTON_LEFT:
                    {
                        alatm_add_digits.time.minute_first++;
                        if (alatm_add_digits.time.minute_first > 5) alatm_add_digits.time.minute_first = 0;
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        alarm_add_sm = SET_ALARM_MINUTE_SECOND;
                        break;
                    }

                    default:
                        break;
                }
            }
            break;
        }
        
        case SET_ALARM_MINUTE_SECOND:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                switch (action_handler.button)
                {
                    case BUTTON_LEFT:
                    {
                        alatm_add_digits.time.minute_second++;
                        if (alatm_add_digits.time.minute_second > 9) alatm_add_digits.time.minute_second = 0;
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        switch(alatm_add_digits.mode)
                        {
                            case ALARM_SINGLE_MODE:
                                alarm_add_sm = SET_SINGLE_DAY_FIRST;
                                break;

                            case ALARM_WEEKLY_MODE:
                                alarm_add_sm = SET_WEEKLY_MONDAY;
                                break;

                            case ALARM_MONTHLY_MODE:
                                alarm_add_sm = SET_MONTHLY_DAY_FIRST;
                                break;

                            case ALARM_YEARLY_MODE:
                                alarm_add_sm = SET_YEARLY_DAY_FIRST;
                                break;

                            default:
                                alarm_add_sm = SET_SINGLE_DAY_FIRST;
                                break;
                        }
                        
                        break;
                    }

                    default:
                        break;
                }
            }
            break;
        }

        case SET_SINGLE_DAY_FIRST:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                switch (action_handler.button)
                {
                    case BUTTON_LEFT:
                    {
                        alatm_add_digits.time.day_first++;
                        if (alatm_add_digits.time.day_first > 3) alatm_add_digits.time.day_first = 0;
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        alarm_add_sm = SET_SINGLE_DAY_SECOND;
                        break;
                    }

                    default:
                        break;
                }
            }
            break;
        }

        case SET_SINGLE_DAY_SECOND:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                switch (action_handler.button)
                {
                    case BUTTON_LEFT:
                    {
                        alatm_add_digits.time.day_second++;

                        if ( (alatm_add_digits.time.day_first > 2 && alatm_add_digits.time.day_second > 1) || (alatm_add_digits.time.day_first < 2 && alatm_add_digits.time.day_second > 9) )
                        {
                            alatm_add_digits.time.day_second = 0;
                        }
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        alarm_add_sm = SET_SINGLE_MONTH_FIRST;
                        break;
                    }

                    default:
                        break;
                }
            }
            break;
        }

        case SET_SINGLE_MONTH_FIRST:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                switch (action_handler.button)
                {
                    case BUTTON_LEFT:
                    {
                        alatm_add_digits.time.month_first++;
                        if (alatm_add_digits.time.month_first > 1) alatm_add_digits.time.month_first = 0;
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        alarm_add_sm = SET_SINGLE_MONTH_SECOND;
                        break;
                    }

                    default:
                        break;
                }
            }
            break;
        }

        case SET_SINGLE_MONTH_SECOND:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                switch (action_handler.button)
                {
                    case BUTTON_LEFT:
                    {
                        alatm_add_digits.time.month_second++;
                        if ( (alatm_add_digits.time.month_first > 0 && alatm_add_digits.time.month_second > 2) || (alatm_add_digits.time.month_first == 0 && alatm_add_digits.time.month_second > 9) )
                        {
                            alatm_add_digits.time.month_second = 0;
                        }
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        alarm_add_sm = SET_SINGLE_YEAR_FIRST;
                        break;
                    }

                    default:
                        break;
                }
            }
            break;
        }

        case SET_SINGLE_YEAR_FIRST:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                switch (action_handler.button)
                {
                    case BUTTON_LEFT:
                    {
                        alatm_add_digits.time.year_first++;
                        if (alatm_add_digits.time.year_first > 9) alatm_add_digits.time.year_first = 0;
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        alarm_add_sm = SET_SINGLE_YEAR_SECOND;
                        break;
                    }

                    default:
                        break;
                }
            }
            break;
        }

        case SET_SINGLE_YEAR_SECOND:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                switch (action_handler.button)
                {
                    case BUTTON_LEFT:
                    {
                        alatm_add_digits.time.year_second++;
                        if (alatm_add_digits.time.year_second > 9) alatm_add_digits.time.year_second = 0;
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        alarm_add_sm = SET_VOLUME;
                        break;
                    }

                    default:
                        break;
                }
            }
            break;
        }

        case SET_WEEKLY_MONDAY:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                switch (action_handler.button)
                {
                    case BUTTON_LEFT:
                    {
                        alatm_add_digits.monday++;
                        if (alatm_add_digits.monday > 1) alatm_add_digits.monday = 0;
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        alarm_add_sm = SET_WEEKLY_TUESDAY;
                        break;
                    }

                    default:
                        break;
                }
            }
            break;
        }

        case SET_WEEKLY_TUESDAY:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                switch (action_handler.button)
                {
                    case BUTTON_LEFT:
                    {
                        alatm_add_digits.tuesday++;
                        if (alatm_add_digits.tuesday > 1) alatm_add_digits.tuesday = 0;
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        alarm_add_sm = SET_WEEKLY_WEDNESDAY;
                        break;
                    }

                    default:
                        break;
                }
            }
            break;
        }

        case SET_WEEKLY_WEDNESDAY:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                switch (action_handler.button)
                {
                    case BUTTON_LEFT:
                    {
                        alatm_add_digits.wednesday++;
                        if (alatm_add_digits.wednesday > 1) alatm_add_digits.wednesday = 0;
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        alarm_add_sm = SET_WEEKLY_THURSDAY;
                        break;
                    }

                    default:
                        break;
                }
            }
            break;
        }

        case SET_WEEKLY_THURSDAY:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                switch (action_handler.button)
                {
                    case BUTTON_LEFT:
                    {
                        alatm_add_digits.thursday++;
                        if (alatm_add_digits.thursday > 1) alatm_add_digits.thursday = 0;
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        alarm_add_sm = SET_WEEKLY_FRIDAY;
                        break;
                    }

                    default:
                        break;
                }
            }
            break;
        }

        case SET_WEEKLY_FRIDAY:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                switch (action_handler.button)
                {
                    case BUTTON_LEFT:
                    {
                        alatm_add_digits.friday++;
                        if (alatm_add_digits.friday > 1) alatm_add_digits.friday = 0;
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        alarm_add_sm = SET_WEEKLY_SATURDAY;
                        break;
                    }

                    default:
                        break;
                }
            }
            break;
        }

        case SET_WEEKLY_SATURDAY:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                switch (action_handler.button)
                {
                    case BUTTON_LEFT:
                    {
                        alatm_add_digits.saturday++;
                        if (alatm_add_digits.saturday > 1) alatm_add_digits.saturday = 0;
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        alarm_add_sm = SET_WEEKLY_SUNDAY;
                        break;
                    }

                    default:
                        break;
                }
            }
            break;
        }

        case SET_WEEKLY_SUNDAY:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                switch (action_handler.button)
                {
                    case BUTTON_LEFT:
                    {
                        alatm_add_digits.sunday++;
                        if (alatm_add_digits.sunday > 1) alatm_add_digits.sunday = 0;
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        alarm_add_sm = SET_VOLUME;
                        break;
                    }

                    default:
                        break;
                }
            }
            break;
        }

        case SET_MONTHLY_DAY_FIRST:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                switch (action_handler.button)
                {
                    case BUTTON_LEFT:
                    {
                        alatm_add_digits.time.day_first++;
                        if (alatm_add_digits.time.day_first > 3) alatm_add_digits.time.day_first = 0;
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        alarm_add_sm = SET_MONTHLY_DAY_SECOND;
                        break;
                    }

                    default:
                        break;
                }
            }
            break;
        }

        case SET_MONTHLY_DAY_SECOND:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                switch (action_handler.button)
                {
                    case BUTTON_LEFT:
                    {
                        alatm_add_digits.time.day_second++;

                        if ( (alatm_add_digits.time.day_first > 2 && alatm_add_digits.time.day_second > 1) || (alatm_add_digits.time.day_first < 2 && alatm_add_digits.time.day_second > 9) )
                        {
                            alatm_add_digits.time.day_second = 0;
                        }
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        alarm_add_sm = SET_VOLUME;
                        break;
                    }

                    default:
                        break;
                }
            }
            break;
        }

        case SET_YEARLY_DAY_FIRST:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                switch (action_handler.button)
                {
                    case BUTTON_LEFT:
                    {
                        alatm_add_digits.time.day_first++;
                        if (alatm_add_digits.time.day_first > 3) alatm_add_digits.time.day_first = 0;
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        alarm_add_sm = SET_YEARLY_DAY_SECOND;
                        break;
                    }

                    default:
                        break;
                }
            }
            break;
        }

        case SET_YEARLY_DAY_SECOND:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                switch (action_handler.button)
                {
                    case BUTTON_LEFT:
                    {
                        alatm_add_digits.time.day_second++;

                        if ( (alatm_add_digits.time.day_first > 2 && alatm_add_digits.time.day_second > 1) || (alatm_add_digits.time.day_first < 2 && alatm_add_digits.time.day_second > 9) )
                        {
                            alatm_add_digits.time.day_second = 0;
                        }
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        alarm_add_sm = SET_YEARLY_MONTH_FIRST;
                        break;
                    }

                    default:
                        break;
                }
            }
            break;
        }

        case SET_YEARLY_MONTH_FIRST:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                switch (action_handler.button)
                {
                    case BUTTON_LEFT:
                    {
                        alatm_add_digits.time.month_first++;
                        if (alatm_add_digits.time.month_first > 1) alatm_add_digits.time.month_first = 0;
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        alarm_add_sm = SET_YEARLY_MONTH_SECOND;
                        break;
                    }

                    default:
                        break;
                }
            }
            break;
        }

        case SET_YEARLY_MONTH_SECOND:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                switch (action_handler.button)
                {
                    case BUTTON_LEFT:
                    {
                        alatm_add_digits.time.month_second++;
                        if ( (alatm_add_digits.time.month_first > 0 && alatm_add_digits.time.month_second > 2) || (alatm_add_digits.time.month_first == 0 && alatm_add_digits.time.month_second > 9) )
                        {
                            alatm_add_digits.time.month_second = 0;
                        }
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        alarm_add_sm = SET_VOLUME;
                        break;
                    }

                    default:
                        break;
                }
            }
            break;
        }

        case SET_VOLUME:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                switch (action_handler.button)
                {
                    case BUTTON_LEFT:
                    {
                        alatm_add_digits.volume++;
                        if (alatm_add_digits.volume > 9) alatm_add_digits.volume = 0;
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        alarm_add_sm = IDLE_ALARM_ADD;
                        device_mode = DEFAULT_MODE;

                        alarm_add.mode = alatm_add_digits.mode;
                        alarm_add.enable = true;
                        alarm_add.desc_len = 0;
                        alarm_add.desc[0] = '\0';
                        alarm_add.hour = alatm_add_digits.time.hour_first * 10 + alatm_add_digits.time.hour_second;
                        alarm_add.minute = alatm_add_digits.time.minute_first * 10 + alatm_add_digits.time.minute_second;

                        switch(alatm_add_digits.mode)
                        {
                            case ALARM_SINGLE_MODE:
                                alarm_add.args.single_alarm_args.day = alatm_add_digits.time.day_first * 10 + alatm_add_digits.time.day_second;
                                alarm_add.args.single_alarm_args.month = alatm_add_digits.time.month_first * 10 + alatm_add_digits.time.month_second;
                                alarm_add.args.single_alarm_args.year = alatm_add_digits.time.year_first * 10 + alatm_add_digits.time.year_second;
                                break;

                            case ALARM_WEEKLY_MODE:
                                if(alatm_add_digits.monday) alarm_add.args.days |= (1 << 0);
                                if(alatm_add_digits.tuesday) alarm_add.args.days |= (1 << 1);
                                if(alatm_add_digits.wednesday) alarm_add.args.days |= (1 << 2);
                                if(alatm_add_digits.thursday) alarm_add.args.days |= (1 << 3);
                                if(alatm_add_digits.friday) alarm_add.args.days |= (1 << 4);
                                if(alatm_add_digits.saturday) alarm_add.args.days |= (1 << 5);
                                if(alatm_add_digits.sunday) alarm_add.args.days |= (1 << 6);
                                break;

                            case ALARM_MONTHLY_MODE:
                                alarm_add.args.day = alatm_add_digits.time.day_first * 10 + alatm_add_digits.time.day_second;
                                break;

                            case ALARM_YEARLY_MODE:
                                alarm_add.args.yearly_alarm_args.day = alatm_add_digits.time.day_first * 10 + alatm_add_digits.time.day_second;
                                alarm_add.args.yearly_alarm_args.month = alatm_add_digits.time.month_first * 10 + alatm_add_digits.time.month_second;
                                break;

                            default:
                                break;
                        }

                        if (alatm_add_digits.volume == 0)
                        {
                            alarm_add.volume = 100;
                        }
                        else
                        {
                            alarm_add.volume = alatm_add_digits.volume * 11;
                        }

                        esp_bt_uuid_t type;
                        type.len = ESP_UUID_LEN_128;
                        oacp_op_code_result_t result;
                        memcpy(type.uuid.uuid128, alarm_type_uuid, ESP_UUID_LEN_128);

                        esp_err_t ret = pp_object_manager_create_object(0, type, &result);
                        if (ret) 
                        {
                            ESP_LOGI(MAIN_TAG, "ObjectManager_create_object: %x", ret);
                            break;
                        }

                        ret = pp_object_manager_change_alarm_data_in_file(alarm_add);
                        if (ret) 
                        {
                            ESP_LOGI(MAIN_TAG, "ObjectManager_change_alarm_data_in_file: %x", ret);
                            break;
                        }

                        pp_set_next_alarm();
                        ESP_LOGI(MAIN_TAG, "ALARM_ADD_MODE -> DEFAULT MODE");
                        
                        break;
                    }

                    default:
                        break;
                }
            }
            break;
        }    
    }
}

void pp_alarm_delete_mode(button_action_t action_handler)
{
    if (action_handler.action == SHORT_PRESS)
    {
        switch (action_handler.button)
        {
            case BUTTON_LEFT:
            {
                object_t *cur_obj;
                olcp_op_code_result_t result;
                bool result_flag = true;

                do
                {
                    pp_object_manager_next_object(&result);
                    if (result == OLCP_RES_SUCCESS)
                    {
                        cur_obj = pp_object_manager_get_object();
                    }
                    else
                    {
                        result_flag = false;
                        break;
                    }
                } while (cur_obj->set_custom_object == false);

                if (result_flag)
                {
                    pp_set_current_alarm_digits();
                }
                break;
            }

            case BUTTON_CENTER:
            {
                object_t *cur_obj;
                olcp_op_code_result_t result;
                bool result_flag = true;

                do
                {
                    pp_object_manager_previous_object(&result);
                    if (result == OLCP_RES_SUCCESS)
                    {
                        cur_obj = pp_object_manager_get_object();
                    }
                    else
                    {
                        result_flag = false;
                        break;
                    }
                } while (cur_obj->set_custom_object == false);

                if (result_flag)
                {
                    pp_set_current_alarm_digits();
                }
                break;
            }

            case BUTTON_RIGHT:
            {
                ESP_LOGI(MAIN_TAG, "ALARM_DELETE_MODE -> DEFAULT MODE");
                device_mode = DEFAULT_MODE;
                break;
            }
                
            default:
                break;
        }
    }
    else if (action_handler.action == LONG_PRESS && action_handler.button == BUTTON_RIGHT)
    {
        oacp_op_code_result_t result;
        pp_object_manager_delete_object(&result);
        pp_set_next_alarm();
        ESP_LOGI(MAIN_TAG, "ALARM DELETED");
        ESP_LOGI(MAIN_TAG, "ALARM_DELETE_MODE -> DEFAULT MODE");
        device_mode = DEFAULT_MODE;
    }  
}

static void pp_set_current_alarm_digits(void)
{
    alatm_add_digits.mode = current_alarm.mode;

    alatm_add_digits.time.hour_first = current_alarm.hour / 10;
    alatm_add_digits.time.hour_second = current_alarm.hour % 10;
    alatm_add_digits.time.minute_first = current_alarm.minute / 10;
    alatm_add_digits.time.minute_second = current_alarm.minute % 10;

    switch (alatm_add_digits.mode)
    {
        case ALARM_SINGLE_MODE:
        {
            alatm_add_digits.time.day_first = current_alarm.args.single_alarm_args.day / 10;
            alatm_add_digits.time.day_second = current_alarm.args.single_alarm_args.day % 10;
            alatm_add_digits.time.month_first = current_alarm.args.single_alarm_args.month / 10;
            alatm_add_digits.time.month_second = current_alarm.args.single_alarm_args.month % 10;
            alatm_add_digits.time.year_first = current_alarm.args.single_alarm_args.year / 10;
            alatm_add_digits.time.year_second = current_alarm.args.single_alarm_args.year % 10;
            break;
        }

        case ALARM_WEEKLY_MODE:
        {
            alatm_add_digits.monday = (current_alarm.args.days && (1 << 0)) ? 1 : 0;
            alatm_add_digits.tuesday = (current_alarm.args.days && (1 << 1)) ? 1 : 0;
            alatm_add_digits.wednesday = (current_alarm.args.days && (1 << 2)) ? 1 : 0;
            alatm_add_digits.thursday = (current_alarm.args.days && (1 << 3)) ? 1 : 0;
            alatm_add_digits.friday = (current_alarm.args.days && (1 << 4)) ? 1 : 0;
            alatm_add_digits.saturday = (current_alarm.args.days && (1 << 5)) ? 1 : 0;
            alatm_add_digits.sunday = (current_alarm.args.days && (1 << 6)) ? 1 : 0;
            break;
        }

        case ALARM_MONTHLY_MODE:
        {
            alatm_add_digits.time.day_first = current_alarm.args.day / 10;
            alatm_add_digits.time.day_second = current_alarm.args.day / 10;
            break;
        }

        case ALARM_YEARLY_MODE:
        {
            alatm_add_digits.time.day_first = current_alarm.args.yearly_alarm_args.day / 10;
            alatm_add_digits.time.day_second = current_alarm.args.yearly_alarm_args.day % 10;
            alatm_add_digits.time.month_first = current_alarm.args.yearly_alarm_args.month / 10;
            alatm_add_digits.time.month_second = current_alarm.args.yearly_alarm_args.month % 10;
            break;
        }
    }

    alatm_add_digits.volume = current_alarm.volume / 11;
}

