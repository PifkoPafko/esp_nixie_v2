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

/* Headers */
static void pp_button_functions(button_action_t action_handler);
static void pp_time_change_mode(button_action_t action_handler, bool start);
static void pp_alarm_add_mode(button_action_t action_handler, bool start);
static void pp_alarm_delete_mode(button_action_t action_handler);
static void pp_ringtone_list_mode(button_action_t action_handler);
static void pp_led_change_mode(button_action_t action_handler, bool start);
static void pp_set_current_alarm_digits(void);
static void pp_set_current_ringtone_digits(void);

/* Macros */
#define PROGRAM_TAG "PROGRAM"

/* Variables declarations */
static pairing_sm_t pairing_sm = WAIT_FOR_LEFT;             // Pairing State Machine
static ring_sm_t ringtone_sm = WAIT_FOR_LEFT_RINGTONE;      // Ringtone State Machine
static led_sm_t led_sm = WAIT_FOR_CENTER_LED;               // LED State Machine
static led_lamps_sm_t led_lamps_sm = IDLE_LED_CHANGE;       // LED lamps State Machine
static time_change_sm_t time_change_sm = IDLE_TIME_CHANGE;  // Time change State Machine
static alarm_add_sm_t alarm_add_sm = IDLE_ALARM_ADD;        // Alarm Adding State Machine

static uint16_t ringtone_index = 0;
static uint8_t ringtone_default = 1;

static alarm_mode_args_t alarm_add;         // Alarm description when in process of adding it
time_t now;                                 // Timestamp variable for keeping time during adding alarms and changing time
struct tm timeinfo;               // Time and Date structure for keeping time during adding alarms and changing time

/** @brief pp_program_main: Program loop
 * 
 * This function is a anchor for all the user actions.
 * This function waits for a button action and redirect the operation to pp_button_functions.
 * 
 * @return
 */
void pp_program_main(void)
{
    while(true)
    {
        button_action_t action;
        if(xQueueReceive(button_action_queue, &action, portMAX_DELAY) != pdTRUE) continue;

        pp_button_functions(action);
    }
}

/** @brief pp_button_functions: Reacts to the button actions.
 * 
 * This function is controls the main user logic based on received buttons actions.
 * This function controls Pairing mode state machine and redirects other modes to destined
 * functions for further actions.
 * 
 * The functionality differs based on the current device mode.
 * Device modes:
 *  - DEFAULT_MODE          - Waiting for action
 *  - TIME_CHANGE_MODE      - User time change
 *  - ALARM_ADD_MODE        - User creating/modifying alarm
 *  - ALARM_DELETE_MODE     - User deleting alarm
 *  - PAIRING_MODE          - User turning on the pairing mode
 *  - PAIRING_PASSKEY_MODE  - User entering pairing code
 *  - ALARM_RING_MODE       - Alarm ringing mode
 *  - LED_CHANGE_MODE       - LED settings mode
 * 
 * @param[in]   action_handler  (button_action_t) Buton action
 * 
 * @return
 */
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
                        ESP_LOGI(PROGRAM_TAG, "DEFAULT MODE -> PAIRING MODE");
                        pairing_sm = PAIRING;

                        device_mode = PAIRING_MODE;
                        pp_update_display();
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

            switch(ringtone_sm)
            {
                case WAIT_FOR_LEFT_RINGTONE:
                {
                    if (action_handler.button == BUTTON_LEFT && action_handler.action == HOLDING_SHORT)
                    {
                        ringtone_sm = WAIT_FOR_RIGHT_RINGTONE;
                    }
                    else
                    {
                        ringtone_sm = WAIT_FOR_LEFT_RINGTONE;
                    }
                    break;
                }

                case WAIT_FOR_RIGHT_RINGTONE:
                {
                    if (action_handler.button == BUTTON_RIGHT && action_handler.action == HOLDING_SHORT)
                    {
                        ringtone_sm = WAIT_FOR_LEFT_LONG_RINGTONE;
                    }
                    else
                    {
                        ringtone_sm = WAIT_FOR_LEFT_RINGTONE;
                    }
                    break;
                }

                case WAIT_FOR_LEFT_LONG_RINGTONE:
                {
                    if (action_handler.button == BUTTON_LEFT && action_handler.action == LONG_PRESS)
                    {
                        ringtone_sm = WAIT_FOR_RIGHT_LONG_RINGTONE;
                    }
                    else
                    {
                        ringtone_sm = WAIT_FOR_LEFT_RINGTONE;
                    }
                    break;
                }

                case WAIT_FOR_RIGHT_LONG_RINGTONE:
                {
                    if (action_handler.button == BUTTON_RIGHT && action_handler.action == LONG_PRESS)
                    {
                        ringtone_sm = RINGTONE;

                        ringtone_index = 0;
                        ringtone_default = 1;

                        is_playing = false;
                        pp_set_current_ringtone_digits();
                        display_digits.alarm_add_ringtone = true;

                        ESP_LOGI(PROGRAM_TAG, "DEFAULT MODE -> RINGTONE_LIST_MODE");
                        device_mode = RINGTONE_LIST_MODE;

                        pp_update_display();
                    }
                    else
                    {
                        ringtone_sm = WAIT_FOR_LEFT_RINGTONE;
                    }
                    break;
                }

                case RINGTONE:
                default:
                    break;
            }

            switch(led_sm)
            {
                case WAIT_FOR_CENTER_LED:
                {
                    if (action_handler.button == BUTTON_CENTER && action_handler.action == HOLDING_SHORT)
                    {
                        led_sm = WAIT_FOR_RIGHT_LED;
                    }
                    else
                    {
                        led_sm = WAIT_FOR_CENTER_LED;
                    }
                    break;
                }

                case WAIT_FOR_RIGHT_LED:
                {
                    if (action_handler.button == BUTTON_RIGHT && action_handler.action == HOLDING_SHORT)
                    {
                        led_sm = WAIT_FOR_CENTER_LONG_LED;
                    }
                    else
                    {
                        led_sm = WAIT_FOR_CENTER_LED;
                    }
                    break;
                }

                case WAIT_FOR_CENTER_LONG_LED:
                {
                    if (action_handler.button == BUTTON_CENTER && action_handler.action == LONG_PRESS)
                    {
                        led_sm = WAIT_FOR_RIGHT_LONG_LED;
                    }
                    else
                    {
                        led_sm = WAIT_FOR_CENTER_LED;
                    }
                    break;
                }

                case WAIT_FOR_RIGHT_LONG_LED:
                {
                    if (action_handler.button == BUTTON_RIGHT && action_handler.action == LONG_PRESS)
                    {
                        led_sm = LED_ADJUST;

                        ESP_LOGI(PROGRAM_TAG, "DEFAULT MODE -> RINGTONE_LIST_MODE");
                        device_mode = LED_CHANGE_MODE;

                        pp_led_change_mode(action_handler, true);
                        pp_update_display();
                    }
                    else
                    {
                        led_sm = WAIT_FOR_CENTER_LED;
                    }
                    break;
                }

                case LED_ADJUST:
                default:
                    break;
            }

            if (action_handler.button == BUTTON_LEFT && action_handler.action == LONG_PRESS && pairing_sm == WAIT_FOR_LEFT && ringtone_sm == WAIT_FOR_LEFT_RINGTONE && led_sm == WAIT_FOR_CENTER_LED)
            {
                ESP_LOGI(PROGRAM_TAG, "DEFAULT MODE -> TIME CHANGE MODE");
                pairing_sm = WAIT_FOR_LEFT;
                ringtone_sm = WAIT_FOR_LEFT_RINGTONE;
                led_sm = WAIT_FOR_CENTER_LED;
                device_mode = TIME_CHANGE_MODE;
                pp_time_change_mode(action_handler, true);
                pp_update_display();
            }

            if (action_handler.button == BUTTON_CENTER && action_handler.action == LONG_PRESS && pairing_sm == WAIT_FOR_LEFT && ringtone_sm == WAIT_FOR_LEFT_RINGTONE && led_sm == WAIT_FOR_CENTER_LED)
            {
                ESP_LOGI(PROGRAM_TAG, "DEFAULT MODE -> ALARM ADD MODE");
                pairing_sm = WAIT_FOR_LEFT;
                ringtone_sm = WAIT_FOR_LEFT_RINGTONE;
                led_sm = WAIT_FOR_CENTER_LED;
                device_mode = ALARM_ADD_MODE;
                alarm_add_sm = IDLE_ALARM_ADD;
                pp_alarm_add_mode(action_handler, true);
                pp_update_display();
            }

            if (action_handler.button == BUTTON_RIGHT && action_handler.action == LONG_PRESS && pairing_sm == WAIT_FOR_LEFT && ringtone_sm == WAIT_FOR_LEFT_RINGTONE && led_sm == WAIT_FOR_CENTER_LED)
            {
                pairing_sm = WAIT_FOR_LEFT;
                ringtone_sm = WAIT_FOR_LEFT_RINGTONE;
                led_sm = WAIT_FOR_CENTER_LED;

                olcp_op_code_result_t result = pp_object_manager_first_object();

                if (result == OLCP_RES_SUCCESS)
                {
                    object_t* cur_obj = pp_object_manager_get_object();
                    bool result_flag = true;

                    while (cur_obj->set_custom_object == false)
                    {
                        result = pp_object_manager_next_object();
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
                        ESP_LOGI(PROGRAM_TAG, "DEFAULT MODE -> ALARM_DELETE_MODE");
                        device_mode = ALARM_DELETE_MODE;
                        pp_set_current_alarm_digits();
                        pp_update_display();
                    }
                    else
                    {
                        ESP_LOGI(PROGRAM_TAG, "NO ALARMS");
                    }
                }
                else
                {
                    ESP_LOGI(PROGRAM_TAG, "NO FILES");
                }
            }

            if (pairing_sm == PAIRING)
            {
                pairing_sm = WAIT_FOR_LEFT;
            }

            if (ringtone_sm == RINGTONE)
            {
                ringtone_sm = WAIT_FOR_LEFT_RINGTONE;
            }
            break;

            if (led_sm == LED_ADJUST)
            {
                led_sm = WAIT_FOR_CENTER_LED;
            }
            break;
        }

        case TIME_CHANGE_MODE:
        {
            pp_time_change_mode(action_handler, false);
            pp_update_display();
            break;
        }

        case ALARM_ADD_MODE:
        {
            pp_alarm_add_mode(action_handler, false);
            pp_update_display();
            break;
        }

        case ALARM_DELETE_MODE:
        {
            pp_alarm_delete_mode(action_handler);
            pp_update_display();
            break;
        }

        case PAIRING_MODE:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                ESP_LOGI(PROGRAM_TAG, "PAIRING MODE -> DEFAULT MODE");
                pairing_sm = WAIT_FOR_LEFT;
                ringtone_sm = WAIT_FOR_LEFT_RINGTONE;
                led_sm = WAIT_FOR_CENTER_LED;
                device_mode = DEFAULT_MODE;
                pp_update_display();
            }  
            break;
        }
        
        case PAIRING_PASSKEY_MODE:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                ESP_LOGI(PROGRAM_TAG, "PAIRING_PASSKEY_MODE -> DEFAULT MODE");
                pairing_sm = WAIT_FOR_LEFT;
                ringtone_sm = WAIT_FOR_LEFT_RINGTONE;
                led_sm = WAIT_FOR_CENTER_LED;
                device_mode = DEFAULT_MODE;
                pp_update_display();
            }  
            break;
        }

        case ALARM_RING_MODE:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                ESP_LOGI(PROGRAM_TAG, "ALARM DISABLED");
                ESP_LOGI(PROGRAM_TAG, "ALARM RING MODE -> DEFAULT MODE");
                pairing_sm = WAIT_FOR_LEFT;
                ringtone_sm = WAIT_FOR_LEFT_RINGTONE;
                led_sm = WAIT_FOR_CENTER_LED;
                device_mode = DEFAULT_MODE;

                alarm_notification_type_t notif = ALARM_STOP_NOTIFICATION;
                xQueueSend(alarm_queue, &notif, 10);

                pp_update_display();
                break;
            }
            break;
        }

        case RINGTONE_LIST_MODE:
        {
            pp_ringtone_list_mode(action_handler);
            pp_update_display();
            break;
        }

        case LED_CHANGE_MODE:
        {
            pp_led_change_mode(action_handler, false);
            pp_update_display();
            break;
        }
    }
}

/** @brief pp_time_change_mode: Time change mode action handler
 * 
 * This function controls the Time Change mode State Machine.
 * This function takes action based on the current state and received button action.
 * 
 * Time change State Machine description:
 * Device modes:
 *  - IDLE_TIME_CHANGE      - Waiting for action
 *  - SET_HOUR_FIRST        - Setting tens part of the hour     (X_:__:__)
 *  - SET_HOUR_SECOND       - Setting ones part of the hour     (_X:__:__)
 *  - SET_MINUTE_FIRST      - Setting tens part of the minute   (__:X_:__)
 *  - SET_MINUTE_SECOND     - Setting ones part of the minute   (__:_X:__)
 *  - SET_SECOND_FIRST      - Setting tens part of the seconds  (__:__:X_)
 *  - SET_SECOND_SECOND     - Setting ones part of the seconds  (__:__:_X)
 *  - SET_DAY_FIRST         - Setting tens part of the day      (X_.__.__)
 *  - SET_DAY_SECOND        - Setting ones part of the day      (_X.__.__)
 *  - SET_MONTH_FIRST       - Setting tens part of the month    (__.X_.__)
 *  - SET_MONTH_SECOND      - Setting ones part of the month    (__._X.__)
 *  - SET_YEAR_FIRST        - Setting tens part of the year     (__.__.X_)
 *  - SET_YEAR_SECOND       - Setting ones part of the year     (__.__._X)
 * 
 * @param[in]   action_handler  (button_action_t) Buton action
 * @param[in]   start           (bool)  True - Start of the Time change procedure
 *                                      False - continuation of the Time change procedure
 * 
 * @return
 */
static void pp_time_change_mode(button_action_t action_handler, bool start)
{
    time_date_digits_t *time_date = &display_digits.display_mode.time_date_digits;

    if (action_handler.action == SHORT_PRESS && action_handler.button == BUTTON_RIGHT)
    {
        ESP_LOGI(PROGRAM_TAG, "TIME CHANGE MODE -> DEFAULT MODE");
        time_change_sm = IDLE_TIME_CHANGE;
        pairing_sm = WAIT_FOR_LEFT;
        ringtone_sm = WAIT_FOR_LEFT_RINGTONE;
        led_sm = WAIT_FOR_CENTER_LED;
        device_mode = DEFAULT_MODE;
    }

    switch(time_change_sm)
    {
        case IDLE_TIME_CHANGE:
        {
            if (start)
            {
                time(&now);
                localtime_r(&now, &timeinfo);

                time_date->time.hour_first = timeinfo.tm_hour / 10;
                time_date->time.hour_second = timeinfo.tm_hour % 10;
                time_date->time.minute_first = timeinfo.tm_min / 10;
                time_date->time.minute_second = timeinfo.tm_min % 10;
                time_date->time.second_first = timeinfo.tm_sec / 10;
                time_date->time.second_second = timeinfo.tm_sec % 10;
                time_date->date.day_first = timeinfo.tm_mday / 10;
                time_date->date.day_second = timeinfo.tm_mday % 10;
                time_date->date.month_first = (timeinfo.tm_mon + 1) / 10;
                time_date->date.month_second = (timeinfo.tm_mon + 1) % 10;
                time_date->date.year_first = (timeinfo.tm_year - 100) / 10;
                time_date->date.year_second = (timeinfo.tm_year - 100) % 10;

                display_digits.blink_tube = 0;

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
                        time_date->time.hour_first++;
                        if (time_date->time.hour_first > 2) time_date->time.hour_first = 0;
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        display_digits.blink_tube = 1;
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
                        time_date->time.hour_second++;
                        if ( (time_date->time.hour_first < 2 && time_date->time.hour_second > 9) || (time_date->time.hour_first == 2 && time_date->time.hour_second > 3) )
                        {
                            time_date->time.hour_second = 0;
                        }
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        if (time_date->time.hour_first == 2 && time_date->time.hour_second > 3) time_date->time.hour_second = 0;
                        display_digits.blink_tube = 2;
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
                        time_date->time.minute_first++;
                        if (time_date->time.minute_first > 5) time_date->time.minute_first = 0;
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        display_digits.blink_tube = 3;
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
                        time_date->time.minute_second++;
                        if (time_date->time.minute_second > 9) time_date->time.minute_second = 0;
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        display_digits.blink_tube = 4;
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
                        time_date->time.second_first++;
                        if (time_date->time.second_first > 5) time_date->time.second_first = 0;
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        display_digits.blink_tube = 5;
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
                        time_date->time.second_second++;
                        if (time_date->time.second_second > 9) time_date->time.second_second = 0;
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        display_digits.blink_tube = 7;
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
                        time_date->date.day_first++;
                        if (time_date->date.day_first > 3) time_date->date.day_first = 0;
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        if (time_date->date.day_first == 3 && time_date->date.day_second > 1) time_date->date.day_second = 0;
                        display_digits.blink_tube = 8;
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
                        time_date->date.day_second++;

                        if ( (time_date->date.day_first == 3 && time_date->date.day_second > 1) || (time_date->date.day_first < 3 && time_date->date.day_second > 9) )
                        {
                            time_date->date.day_second = 0;
                        }
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        display_digits.blink_tube = 9;
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
                        time_date->date.month_first++;
                        if (time_date->date.month_first > 1) time_date->date.month_first = 0;
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        display_digits.blink_tube = 10;
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
                        time_date->date.month_second++;
                        if ( (time_date->date.month_first > 0 && time_date->date.month_second > 2) || (time_date->date.month_first == 0 && time_date->date.month_second > 9) )
                        {
                            time_date->date.month_second = 0;
                        }
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        display_digits.blink_tube = 11;
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
                        time_date->date.year_first++;
                        if (time_date->date.year_first > 9) time_date->date.year_first = 0;
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        display_digits.blink_tube = 12;
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
                        time_date->date.year_second++;
                        if (time_date->date.year_second > 9) time_date->date.year_second = 0;
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        time_change_sm = IDLE_TIME_CHANGE;
                        pairing_sm = WAIT_FOR_LEFT;
                        ringtone_sm = WAIT_FOR_LEFT_RINGTONE;
                        device_mode = DEFAULT_MODE;

                        struct tm timeinfo;
                        timeinfo.tm_sec = time_date->time.second_first * 10 + time_date->time.second_second;
                        timeinfo.tm_min = time_date->time.minute_first * 10 + time_date->time.minute_second;
                        timeinfo.tm_hour = time_date->time.hour_first * 10 + time_date->time.hour_second;
                        timeinfo.tm_mday = time_date->date.day_first * 10 + time_date->date.day_second;
                        timeinfo.tm_mon = time_date->date.month_first * 10 + time_date->date.month_second - 1;
                        timeinfo.tm_year = time_date->date.year_first * 10 + time_date->date.year_second + 100;
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

/** @brief pp_alarm_add_mode: Alarm Add mode action handler
 * 
 * This function controls the Alarm Add mode State Machine.
 * This function takes action based on the current state and received button action.
 * 
 * Alarm Add State Machine description:
 *  - IDLE_ALARM_ADD        - Waiting for action
 *  - SET_MODE
 *  - SET_ALARM_HOUR_FIRST      - Setting tens part of the hour     (X_:__)
 *  - SET_ALARM_HOUR_SECOND     - Setting ones part of the hour     (_X:__)
 *  - SET_ALARM_MINUTE_FIRST    - Setting tens part of the minute   (__:X_)
 *  - SET_ALARM_MINUTE_SECOND   - Setting ones part of the minute   (__:_X)
 *  - SET_ALARM_DAY_FIRST       - Setting tens part of the day      (X_.__.__)
 *  - SET_ALARM_DAY_SECOND      - Setting ones part of the day      (_X.__.__)
 *  - SET_ALARM_MONTH_FIRST     - Setting tens part of the month    (__.X_.__)
 *  - SET_ALARM_MONTH_SECOND    - Setting ones part of the month    (__._X.__)
 *  - SET_ALARM_YEAR_FIRST      - Setting tens part of the year     (__.__.X_)
 *  - SET_ALARM_YEAR_SECOND     - Setting ones part of the year     (__.__._X)
 *  - SET_WEEKLY_MONDAY         - Setting weekdays options - Weekly Mode (Monday)
 *  - SET_WEEKLY_TUESDAY        - Setting weekdays options - Weekly Mode (Tuesday)
 *  - SET_WEEKLY_WEDNESDAY      - Setting weekdays options - Weekly Mode (Wednesday)
 *  - SET_WEEKLY_THURSDAY       - Setting weekdays options - Weekly Mode (Thursday)
 *  - SET_WEEKLY_FRIDAY         - Setting weekdays options - Weekly Mode (Friday)
 *  - SET_WEEKLY_SATURDAY       - Setting weekdays options - Weekly Mode (Saturday)
 *  - SET_WEEKLY_SUNDAY         - Setting weekdays options - Weekly Mode (Sunday)
 *  - SET_MONTHLY_DAY_FIRST     - Setting tens part of the day - Monthly mode   (X_.__.__)
 *  - SET_MONTHLY_DAY_SECOND    - Setting ones part of the day - Monthly mode   (_X.__.__)
 *  - SET_YEARLY_DAY_FIRST      - Setting tens part of the day - Yearly mode    (X_.__.__)
 *  - SET_YEARLY_DAY_SECOND     - Setting ones part of the day - Yearly mode    (_X.__.__)
 *  - SET_YEARLY_MONTH_FIRST    - Setting tens part of the month - Yearly mode  (__.X_.__)
 *  - SET_YEARLY_MONTH_SECOND   - Setting ones part of the month - Yearly mode  (__._X.__)
 *  - SET_VOLUME                - Setting volume level
 * 
 * @param[in]   action_handler  (button_action_t) Button action
 * @param[in]   start           (bool)  True - Start of the Alarm Add procedure
 *                                      False - continuation of the Alarm Add procedure
 * 
 * @return
 */
static void pp_alarm_add_mode(button_action_t action_handler, bool start)
{
    alarm_digits_t *alarm_digits_p = &display_digits.display_mode.alarm_digits;

    if (action_handler.action == SHORT_PRESS && action_handler.button == BUTTON_RIGHT)
    {
        ESP_LOGI(PROGRAM_TAG, "ALARM_ADD_MODE -> DEFAULT MODE");
        display_digits.alarm_add_ringtone = false;
        alarm_add_sm = IDLE_ALARM_ADD;
        pairing_sm = WAIT_FOR_LEFT;
        ringtone_sm = WAIT_FOR_LEFT_RINGTONE;
        led_sm = WAIT_FOR_CENTER_LED;
        device_mode = DEFAULT_MODE;
        memset(&timeinfo, 0, sizeof(struct tm));
    }

    switch(alarm_add_sm)
    {
        case IDLE_ALARM_ADD:
        {
            if (start)
            {
                time(&now);
                localtime_r(&now, &timeinfo);

                alarm_digits_p->alarm_mode = ALARM_SINGLE_MODE;

                alarm_digits_p->time.hour_first = 1;
                alarm_digits_p->time.hour_second = 2;
                alarm_digits_p->time.minute_first = 0;
                alarm_digits_p->time.minute_second = 0;

                alarm_digits_p->arg.date.day_first = timeinfo.tm_mday / 10;
                alarm_digits_p->arg.date.day_second = timeinfo.tm_mday % 10;
                alarm_digits_p->arg.date.month_first = (timeinfo.tm_mon + 1) / 10;
                alarm_digits_p->arg.date.month_second = (timeinfo.tm_mon + 1) % 10;
                alarm_digits_p->arg.date.year_first = (timeinfo.tm_year - 100) / 10;
                alarm_digits_p->arg.date.year_second = (timeinfo.tm_year - 100) % 10;

                alarm_digits_p->volume = 9;

                display_digits.blink_tube = 0;
                display_digits.alarm_add_ringtone = false;

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
                        alarm_digits_p->alarm_mode++;
                        if (alarm_digits_p->alarm_mode == 1)
                        {
                            alarm_digits_p->arg.days = 0;
                        }
                        else if (alarm_digits_p->alarm_mode > 3) 
                        {
                            alarm_digits_p->alarm_mode = 0;
                        }
                        else
                        {    
                            alarm_digits_p->arg.date.day_first = timeinfo.tm_mday / 10;
                            alarm_digits_p->arg.date.day_second = timeinfo.tm_mday % 10;
                            alarm_digits_p->arg.date.month_first = (timeinfo.tm_mon + 1) / 10;
                            alarm_digits_p->arg.date.month_second = (timeinfo.tm_mon + 1) % 10;
                            alarm_digits_p->arg.date.year_first = (timeinfo.tm_year - 100) / 10;
                            alarm_digits_p->arg.date.year_second = (timeinfo.tm_year - 100) % 10;
                        }
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        display_digits.blink_tube = 2;
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
                        alarm_digits_p->time.hour_first++;
                        if (alarm_digits_p->time.hour_first > 2) alarm_digits_p->time.hour_first = 0;
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        display_digits.blink_tube = 3;
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
                        alarm_digits_p->time.hour_second++;
                        if ( (alarm_digits_p->time.hour_first < 2 && alarm_digits_p->time.hour_second > 9) || (alarm_digits_p->time.hour_first == 2 && alarm_digits_p->time.hour_second > 3) )
                        {
                            alarm_digits_p->time.hour_second = 0;
                        }
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        display_digits.blink_tube = 4;
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
                        alarm_digits_p->time.minute_first++;
                        if (alarm_digits_p->time.minute_first > 5) alarm_digits_p->time.minute_first = 0;
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        display_digits.blink_tube = 5;
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
                        alarm_digits_p->time.minute_second++;
                        if (alarm_digits_p->time.minute_second > 9) alarm_digits_p->time.minute_second = 0;
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        switch(alarm_digits_p->alarm_mode)
                        {
                            case ALARM_SINGLE_MODE:
                                display_digits.blink_tube = 7;
                                alarm_add_sm = SET_SINGLE_DAY_FIRST;
                                break;

                            case ALARM_WEEKLY_MODE:
                                display_digits.blink_tube = 7;
                                alarm_add_sm = SET_WEEKLY_MONDAY;
                                break;

                            case ALARM_MONTHLY_MODE:
                                display_digits.blink_tube = 7;
                                alarm_add_sm = SET_MONTHLY_DAY_FIRST;
                                break;

                            case ALARM_YEARLY_MODE:
                                display_digits.blink_tube = 7;
                                alarm_add_sm = SET_YEARLY_DAY_FIRST;
                                break;

                            default:
                                display_digits.blink_tube = 7;
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
                        alarm_digits_p->arg.date.day_first++;
                        if (alarm_digits_p->arg.date.day_first > 3) alarm_digits_p->arg.date.day_first = 0;
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        display_digits.blink_tube = 8;
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
                        alarm_digits_p->arg.date.day_second++;

                        if ( (alarm_digits_p->arg.date.day_first > 2 && alarm_digits_p->arg.date.day_second > 1) || (alarm_digits_p->arg.date.day_first < 2 && alarm_digits_p->arg.date.day_second > 9) )
                        {
                            alarm_digits_p->arg.date.day_second = 0;
                        }
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        display_digits.blink_tube = 9;
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
                        alarm_digits_p->arg.date.month_first++;
                        if (alarm_digits_p->arg.date.month_first > 1) alarm_digits_p->arg.date.month_first = 0;
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        display_digits.blink_tube = 10;
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
                        alarm_digits_p->arg.date.month_second++;
                        if ( (alarm_digits_p->arg.date.month_first > 0 && alarm_digits_p->arg.date.month_second > 2) || (alarm_digits_p->arg.date.month_first == 0 && alarm_digits_p->arg.date.month_second > 9) )
                        {
                            alarm_digits_p->arg.date.month_second = 0;
                        }
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        display_digits.blink_tube = 11;
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
                        alarm_digits_p->arg.date.year_first++;
                        if (alarm_digits_p->arg.date.year_first > 9) alarm_digits_p->arg.date.year_first = 0;
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        display_digits.blink_tube = 12;
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
                        alarm_digits_p->arg.date.year_second++;
                        if (alarm_digits_p->arg.date.year_second > 9) alarm_digits_p->arg.date.year_second = 0;
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        display_digits.blink_tube = 15;
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
                        alarm_digits_p->arg.days ^= (1 << 0);
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        display_digits.blink_tube = 8;
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
                        alarm_digits_p->arg.days ^= (1 << 1);
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        display_digits.blink_tube = 9;
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
                        alarm_digits_p->arg.days ^= (1 << 2);
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        display_digits.blink_tube = 10;
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
                        alarm_digits_p->arg.days ^= (1 << 3);
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        display_digits.blink_tube = 11;
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
                        alarm_digits_p->arg.days ^= (1 << 4);
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        display_digits.blink_tube = 12;
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
                        alarm_digits_p->arg.days ^= (1 << 5);
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        display_digits.blink_tube = 13;
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
                        alarm_digits_p->arg.days ^= (1 << 6);
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        display_digits.blink_tube = 15;
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
                        alarm_digits_p->arg.date.day_first++;
                        if (alarm_digits_p->arg.date.day_first > 3) alarm_digits_p->arg.date.day_first = 0;
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        display_digits.blink_tube = 8;
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
                        alarm_digits_p->arg.date.day_second++;

                        if ( (alarm_digits_p->arg.date.day_first > 2 && alarm_digits_p->arg.date.day_second > 1) || (alarm_digits_p->arg.date.day_first < 2 && alarm_digits_p->arg.date.day_second > 9) )
                        {
                            alarm_digits_p->arg.date.day_second = 0;
                        }
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        display_digits.blink_tube = 15;
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
                        alarm_digits_p->arg.date.day_first++;
                        if (alarm_digits_p->arg.date.day_first > 3) alarm_digits_p->arg.date.day_first = 0;
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        display_digits.blink_tube = 8;
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
                        alarm_digits_p->arg.date.day_second++;

                        if ( (alarm_digits_p->arg.date.day_first > 2 && alarm_digits_p->arg.date.day_second > 1) || (alarm_digits_p->arg.date.day_first < 2 && alarm_digits_p->arg.date.day_second > 9) )
                        {
                            alarm_digits_p->arg.date.day_second = 0;
                        }
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        display_digits.blink_tube = 9;
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
                        alarm_digits_p->arg.date.month_first++;
                        if (alarm_digits_p->arg.date.month_first > 1) alarm_digits_p->arg.date.month_first = 0;
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        display_digits.blink_tube = 10;
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
                        alarm_digits_p->arg.date.month_second++;
                        if ( (alarm_digits_p->arg.date.month_first > 0 && alarm_digits_p->arg.date.month_second > 2) || (alarm_digits_p->arg.date.month_first == 0 && alarm_digits_p->arg.date.month_second > 9) )
                        {
                            alarm_digits_p->arg.date.month_second = 0;
                        }
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        display_digits.blink_tube = 15;
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
                        alarm_digits_p->volume++;
                        if (alarm_digits_p->volume > 9) alarm_digits_p->volume = 0;
                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        alarm_add.mode = alarm_digits_p->alarm_mode;
                        alarm_add.enable = true;
                        alarm_add.desc_len = 0;
                        alarm_add.desc[0] = '\0';
                        alarm_add.hour = alarm_digits_p->time.hour_first * 10 + alarm_digits_p->time.hour_second;
                        alarm_add.minute = alarm_digits_p->time.minute_first * 10 + alarm_digits_p->time.minute_second;

                        switch(alarm_digits_p->alarm_mode)
                        {
                            case ALARM_SINGLE_MODE:
                                alarm_add.args.single_alarm_args.day = alarm_digits_p->arg.date.day_first * 10 + alarm_digits_p->arg.date.day_second;
                                alarm_add.args.single_alarm_args.month = alarm_digits_p->arg.date.month_first * 10 + alarm_digits_p->arg.date.month_second;
                                alarm_add.args.single_alarm_args.year = alarm_digits_p->arg.date.year_first * 10 + alarm_digits_p->arg.date.year_second;
                                break;

                            case ALARM_WEEKLY_MODE:
                                alarm_add.args.days = alarm_digits_p->arg.days;
                                break;

                            case ALARM_MONTHLY_MODE:
                                alarm_add.args.day = alarm_digits_p->arg.date.day_first * 10 + alarm_digits_p->arg.date.day_second;
                                break;

                            case ALARM_YEARLY_MODE:
                                alarm_add.args.yearly_alarm_args.day = alarm_digits_p->arg.date.day_first * 10 + alarm_digits_p->arg.date.day_second;
                                alarm_add.args.yearly_alarm_args.month = alarm_digits_p->arg.date.month_first * 10 + alarm_digits_p->arg.date.month_second;
                                break;

                            default:
                                break;
                        }

                        alarm_add.volume = alarm_digits_p->volume;

                        ringtone_index = 0;
                        ringtone_default = 1;

                        is_playing = false;
                        pp_set_current_ringtone_digits();
                        display_digits.alarm_add_ringtone = true;
                        alarm_add_sm = SET_RINGTONE_ALARM;
                    }

                    default:
                        break;
                }
            }
            break;
        }

        case SET_RINGTONE_ALARM:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                switch (action_handler.button)
                {
                    case BUTTON_LEFT:
                    {
                        alarm_notification_type_t notif = ALARM_STOP_NOTIFICATION;
                        xQueueSend(alarm_queue, &notif, 10);
                        is_playing = false;

                        if(ringtone_index < DEFAULT_ALARM_COUNT - 1)
                        {
                            ringtone_index = ringtone_index + 1;
                        }
                        else if(ringtone_index == DEFAULT_ALARM_COUNT - 1)
                        {
                            ListFilter_t* filter = pp_object_list_get_filter();
                            uint8_t *order = pp_object_list_get_order();

                            *order = 0;
                            filter->type = OBJECT_TYPE;
                            filter->parameter[0] = RINGTONE_TYPE;
                            filter->par_length = 1;
                            pp_object_list_make_list();

                            olcp_op_code_result_t result = pp_object_manager_first_object();

                            if (result == OLCP_RES_SUCCESS)
                            {
                                ringtone_default = 0;
                                ringtone_index = ringtone_index + 1;
                            }
                            else
                            {
                                ringtone_default = 1;
                                ringtone_index = 0;
                            }
                        }
                        else
                        {
                            olcp_op_code_result_t result = pp_object_manager_next_object();

                            if (result == OLCP_RES_OUT_OF_THE_BONDS)
                            {
                                ringtone_default = 1;
                                ringtone_index = 0;
                            }
                            else if (result == OLCP_RES_SUCCESS)
                            {
                                ringtone_index = ringtone_index + 1;
                            }
                            else
                            {
                                ESP_LOGI(PROGRAM_TAG, "ALARM_ADD_MODE -> DEFAULT MODE");
                                pp_set_next_alarm();
                                pairing_sm = WAIT_FOR_LEFT;
                                ringtone_sm = WAIT_FOR_LEFT_RINGTONE;
                                led_sm = WAIT_FOR_CENTER_LED;
                                device_mode = DEFAULT_MODE;
                            }
                        }
                        
                        pp_set_current_ringtone_digits();

                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        alarm_notification_type_t notif = ALARM_STOP_NOTIFICATION;
                        xQueueSend(alarm_queue, &notif, 10);
                        is_playing = false;

                        alarm_add.ringtone_default = ringtone_default;

                        if(ringtone_default)
                        {
                            alarm_add.ringtone_id = ringtone_index;
                        }
                        else
                        {
                            object_t *cur_obj = pp_object_manager_get_object();
                            alarm_add.ringtone_id = cur_obj->id;
                        }

                        esp_bt_uuid_t type;
                        type.len = ESP_UUID_LEN_128;
                        memcpy(type.uuid.uuid128, alarm_type_uuid, ESP_UUID_LEN_128);

                        oacp_op_code_result_t result = pp_object_manager_create_object(0, type);
                        if (result != OACP_RES_SUCCESS) 
                        {
                            ESP_LOGI(PROGRAM_TAG, "ObjectManager_create_object failed: %x", result);
                            ESP_LOGI(PROGRAM_TAG, "ALARM_ADD_MODE -> DEFAULT MODE");
                            alarm_add_sm = IDLE_ALARM_ADD;
                            pairing_sm = WAIT_FOR_LEFT;
                            ringtone_sm = WAIT_FOR_LEFT_RINGTONE;
                            led_sm = WAIT_FOR_CENTER_LED;
                            device_mode = DEFAULT_MODE;
                            break;
                        }
                        
                        pp_object_manager_change_alarm_data_in_file(&alarm_add);

                        pp_set_next_alarm();
                        alarm_add_sm = IDLE_ALARM_ADD;
                        pairing_sm = WAIT_FOR_LEFT;
                        ringtone_sm = WAIT_FOR_LEFT_RINGTONE;
                        led_sm = WAIT_FOR_CENTER_LED;
                        device_mode = DEFAULT_MODE;
                        ESP_LOGI(PROGRAM_TAG, "ALARM_ADD_MODE -> DEFAULT MODE");
                        
                        break;
                    }

                    default:
                        break;
                }
            }
            else if (action_handler.action == LONG_PRESS)
            {
                switch (action_handler.button)
                {
                    case BUTTON_LEFT:
                    {
                        if(is_playing)
                        {
                            alarm_notification_type_t notif = ALARM_STOP_NOTIFICATION;
                            xQueueSend(alarm_queue, &notif, 10);
                            is_playing = false;
                        }
                        else
                        {
                            if(ringtone_default)
                            {
                                pp_set_next_ringtone_id(ringtone_default, ringtone_index);
                            }
                            else
                            {
                                object_t *cur_obj = pp_object_manager_get_object();
                                pp_set_next_ringtone_id(ringtone_default, cur_obj->id);
                            }

                            alarm_notification_type_t notif = ALARM_START_NOTIFICATION;
                            xQueueSend(alarm_queue, &notif, 10);
                            is_playing = true;
                        }
                        break;
                    }

                    default:
                        break;
                }
            }

            break;
        }

        default:
            break;
    }
}

/** @brief pp_alarm_delete_mode: Alarm Delete action handler
 * 
 * This function takes action based on the received button action.
 * 
 * @param[in]   action_handler  (button_action_t) Button action
 * 
 * @return
 */
static void pp_alarm_delete_mode(button_action_t action_handler)
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
                    result = pp_object_manager_next_object();
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
                    result = pp_object_manager_previous_object();
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
                ESP_LOGI(PROGRAM_TAG, "ALARM_DELETE_MODE -> DEFAULT MODE");
                pairing_sm = WAIT_FOR_LEFT;
                ringtone_sm = WAIT_FOR_LEFT_RINGTONE;
                led_sm = WAIT_FOR_CENTER_LED;
                device_mode = DEFAULT_MODE;
                break;
            }
                
            default:
                break;
        }
    }
    else if (action_handler.action == LONG_PRESS && action_handler.button == BUTTON_RIGHT)
    {
        oacp_op_code_result_t result = pp_object_manager_delete_object();

        if(result == OACP_RES_SUCCESS)
        {
            pp_set_next_alarm();
            ESP_LOGI(PROGRAM_TAG, "ALARM DELETED");
        }
        else
        {
            ESP_LOGI(PROGRAM_TAG, "ALARM NOT DELETED");
        }
        
        ESP_LOGI(PROGRAM_TAG, "ALARM_DELETE_MODE -> DEFAULT MODE");
        pairing_sm = WAIT_FOR_LEFT;
        ringtone_sm = WAIT_FOR_LEFT_RINGTONE;
        led_sm = WAIT_FOR_CENTER_LED;
        device_mode = DEFAULT_MODE;
    }  
}

/** @brief pp_ringtone_list_mode: Ringtone List action handler
 * 
 * This function takes action based on the received button action.
 * 
 * @param[in]   action_handler  (button_action_t) Button action
 * 
 * @return
 */
static void pp_ringtone_list_mode(button_action_t action_handler)
{
    if (action_handler.action == SHORT_PRESS)
    {
        switch (action_handler.button)
        {
            case BUTTON_LEFT:
            {
                alarm_notification_type_t notif = ALARM_STOP_NOTIFICATION;
                xQueueSend(alarm_queue, &notif, 10);
                is_playing = false;

                if(ringtone_index < DEFAULT_ALARM_COUNT - 1)
                {
                    ringtone_index = ringtone_index + 1;
                }
                else if(ringtone_index == DEFAULT_ALARM_COUNT - 1)
                {
                    ListFilter_t* filter = pp_object_list_get_filter();
                    uint8_t *order = pp_object_list_get_order();

                    *order = 0;
                    filter->type = OBJECT_TYPE;
                    filter->parameter[0] = RINGTONE_TYPE;
                    filter->par_length = 1;
                    pp_object_list_make_list();

                    olcp_op_code_result_t result = pp_object_manager_first_object();

                    if (result == OLCP_RES_SUCCESS)
                    {
                        ringtone_default = 0;
                        ringtone_index = ringtone_index + 1;
                    }
                    else
                    {
                        ringtone_default = 1;
                        ringtone_index = 0;
                    }
                }
                else
                {
                    olcp_op_code_result_t result = pp_object_manager_next_object();

                    if (result == OLCP_RES_OUT_OF_THE_BONDS)
                    {
                        ringtone_default = 1;
                        ringtone_index = 0;
                    }
                    else if (result == OLCP_RES_SUCCESS)
                    {
                        ringtone_index = ringtone_index + 1;
                    }
                    else
                    {
                        ESP_LOGI(PROGRAM_TAG, "RINGTONE_LIST_MODE -> DEFAULT MODE");
                        pairing_sm = WAIT_FOR_LEFT;
                        ringtone_sm = WAIT_FOR_LEFT_RINGTONE;
                        led_sm = WAIT_FOR_CENTER_LED;
                        device_mode = DEFAULT_MODE;
                    }
                }
                
                pp_set_current_ringtone_digits();
                break;
            }

            case BUTTON_CENTER:
            {
                if(is_playing)
                {
                    alarm_notification_type_t notif = ALARM_STOP_NOTIFICATION;
                    xQueueSend(alarm_queue, &notif, 10);
                    is_playing = false;
                }
                else
                {
                    if(ringtone_default)
                    {
                        pp_set_next_ringtone_id(ringtone_default, ringtone_index);
                    }
                    else
                    {
                        object_t *cur_obj = pp_object_manager_get_object();
                        pp_set_next_ringtone_id(ringtone_default, cur_obj->id);
                    }

                    alarm_notification_type_t notif = ALARM_START_NOTIFICATION;
                    xQueueSend(alarm_queue, &notif, 10);
                    is_playing = true;
                }
        
                break;
            }

            case BUTTON_RIGHT:
            {
                alarm_notification_type_t notif = ALARM_STOP_NOTIFICATION;
                xQueueSend(alarm_queue, &notif, 10);
                pp_set_next_alarm();
                ESP_LOGI(PROGRAM_TAG, "RINGTONE_LIST_MODE -> DEFAULT MODE");
                pairing_sm = WAIT_FOR_LEFT;
                ringtone_sm = WAIT_FOR_LEFT_RINGTONE;
                led_sm = WAIT_FOR_CENTER_LED;
                device_mode = DEFAULT_MODE;
                break;
            }
                
            default:
                break;
        }
    }
    else if (action_handler.action == LONG_PRESS && action_handler.button == BUTTON_RIGHT)
    {
        if(ringtone_default)
        {
            ESP_LOGI(PROGRAM_TAG, "Deleting  defult ringtone not allowed");
        }
        else
        {
            oacp_op_code_result_t result = pp_object_manager_delete_object();

            if(result == OACP_RES_SUCCESS)
            {
                ESP_LOGI(PROGRAM_TAG, "ALARM DELETED");
            }
            else
            {
                ESP_LOGI(PROGRAM_TAG, "ALARM NOT DELETED");
            }
        }
        
        pp_set_next_alarm();
        ESP_LOGI(PROGRAM_TAG, "RINGTONE_LIST_MODE -> DEFAULT MODE");
        pairing_sm = WAIT_FOR_LEFT;
        ringtone_sm = WAIT_FOR_LEFT_RINGTONE;
        led_sm = WAIT_FOR_CENTER_LED;
        device_mode = DEFAULT_MODE;
    }  
}


/** @brief pp_led_change_mode: LED change action handler
 * 
 * This function takes action based on the received button action.
 * 
 * @param[in]   action_handler  (button_action_t) Button action
 * @param[in]   start           (bool)  True - Start of the LED Change procedure
 *                                      False - continuation of the LED Change procedure
 * 
 * @return
 */
static void pp_led_change_mode(button_action_t action_handler, bool start)
{
    led_digits_t *led_digits_p = &display_digits.display_mode.led_change_digits;

    if (action_handler.action == SHORT_PRESS && action_handler.button == BUTTON_RIGHT)
    {
        ESP_LOGI(PROGRAM_TAG, "LED_CHANGE_MODE -> DEFAULT MODE");
        led_lamps_sm = IDLE_LED_CHANGE;
        led_sm = WAIT_FOR_CENTER_LED;
        pairing_sm = WAIT_FOR_LEFT;
        ringtone_sm = WAIT_FOR_LEFT_RINGTONE;
        device_mode = DEFAULT_MODE;
        pp_nvs_led_store(0x1 | 0x2);
    }

    switch(led_lamps_sm)
    {
        case IDLE_LED_CHANGE:
        {
            if (start)
            {
                led_digits_p->led_lamp_enable = current_led[0].enable ? 1 : 0;
                led_digits_p->led_lamp_red = current_led[0].red / 455; 
                led_digits_p->led_lamp_blue = current_led[0].blue / 455;
                led_digits_p->led_lamp_green = current_led[0].green / 455;

                led_digits_p->led_bottom_enable = current_led[1].enable ? 1 : 0;
                led_digits_p->led_bottom_red = current_led[1].red / 455; 
                led_digits_p->led_bottom_blue = current_led[1].blue / 455;
                led_digits_p->led_bottom_green = current_led[1].green / 455;

                display_digits.blink_tube = 0;

                led_lamps_sm = SET_LED0_ENABLE;
            }    
            break;
        }

        case SET_LED0_ENABLE:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                switch (action_handler.button)
                {
                    case BUTTON_LEFT:
                    {
                        led_digits_p->led_lamp_enable++;
                        if (led_digits_p->led_lamp_enable > 1)
                        {
                            led_digits_p->led_lamp_enable = 0;
                        }

                        current_led[0].enable = (bool)led_digits_p->led_lamp_enable;

                        pp_update_led(0);

                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        display_digits.blink_tube = 1;
                        led_lamps_sm = SET_LED0_RED;
                        break;
                    }

                    default:
                        break;
                }
            }
            break;
        }

        case SET_LED0_RED:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                switch (action_handler.button)
                {
                    case BUTTON_LEFT:
                    {
                        led_digits_p->led_lamp_red++;
                        if (led_digits_p->led_lamp_red > 9)
                        {
                            led_digits_p->led_lamp_red = 0;
                        }

                        current_led[0].red = led_digits_p->led_lamp_red * 455;

                        pp_update_led(0);

                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        display_digits.blink_tube = 2;
                        led_lamps_sm = SET_LED0_GREEN;
                        break;
                    }

                    default:
                        break;
                }
            }
            break;
        }

        case SET_LED0_GREEN:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                switch (action_handler.button)
                {
                    case BUTTON_LEFT:
                    {
                        led_digits_p->led_lamp_green++;
                        if (led_digits_p->led_lamp_green > 9)
                        {
                            led_digits_p->led_lamp_green = 0;
                        }

                        current_led[0].green = led_digits_p->led_lamp_green * 455;

                        pp_update_led(0);

                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        display_digits.blink_tube = 3;
                        led_lamps_sm = SET_LED0_BLUE;
                        break;
                    }

                    default:
                        break;
                }
            }
            break;
        }

        case SET_LED0_BLUE:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                switch (action_handler.button)
                {
                    case BUTTON_LEFT:
                    {
                        led_digits_p->led_lamp_blue++;
                        if (led_digits_p->led_lamp_blue > 9)
                        {
                            led_digits_p->led_lamp_blue = 0;
                        }

                        current_led[0].blue = led_digits_p->led_lamp_blue * 455;

                        pp_update_led(0);

                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        display_digits.blink_tube = 5;
                        led_lamps_sm = SET_LED1_ENABLE;
                        break;
                    }

                    default:
                        break;
                }
            }
            break;
        }

        case SET_LED1_ENABLE:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                switch (action_handler.button)
                {
                    case BUTTON_LEFT:
                    {
                        led_digits_p->led_bottom_enable++;
                        if (led_digits_p->led_bottom_enable > 1)
                        {
                            led_digits_p->led_bottom_enable = 0;
                        }

                        current_led[1].enable = (bool)led_digits_p->led_bottom_enable;

                        pp_update_led(1);

                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        display_digits.blink_tube = 6;
                        led_lamps_sm = SET_LED1_RED;
                        break;
                    }

                    default:
                        break;
                }
            }
            break;
        }

        case SET_LED1_RED:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                switch (action_handler.button)
                {
                    case BUTTON_LEFT:
                    {
                        led_digits_p->led_bottom_red++;
                        if (led_digits_p->led_bottom_red > 9)
                        {
                            led_digits_p->led_bottom_red = 0;
                        }

                        current_led[1].red = led_digits_p->led_bottom_red * 455;

                        pp_update_led(1);

                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        display_digits.blink_tube = 7;
                        led_lamps_sm = SET_LED1_GREEN;
                        break;
                    }

                    default:
                        break;
                }
            }
            break;
        }

        case SET_LED1_GREEN:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                switch (action_handler.button)
                {
                    case BUTTON_LEFT:
                    {
                        led_digits_p->led_bottom_green++;
                        if (led_digits_p->led_bottom_green > 9)
                        {
                            led_digits_p->led_bottom_green = 0;
                        }

                        current_led[1].green = led_digits_p->led_bottom_green * 455;

                        pp_update_led(1);

                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        display_digits.blink_tube = 8;
                        led_lamps_sm = SET_LED1_BLUE;
                        break;
                    }

                    default:
                        break;
                }
            }
            break;
        }

        case SET_LED1_BLUE:
        {
            if (action_handler.action == SHORT_PRESS)
            {
                switch (action_handler.button)
                {
                    case BUTTON_LEFT:
                    {
                        led_digits_p->led_bottom_blue++;
                        if (led_digits_p->led_bottom_blue > 9)
                        {
                            led_digits_p->led_bottom_blue = 0;
                        }

                        current_led[1].blue = led_digits_p->led_bottom_blue * 455;

                        pp_update_led(1);

                        break;
                    }

                    case BUTTON_CENTER:
                    {
                        display_digits.blink_tube = 0;
                        led_lamps_sm = SET_LED0_ENABLE;
                        break;
                    }

                    default:
                        break;
                }
            }
            break;
        }
            
        default:
            break;

    }
}


/** @brief pp_set_current_alarm_digits: Sets alarm digits on be display during Alarm Delete mode.
 * 
 * Reads current alarm parameters, calculates digits to be displayed and stores them in the structure.
 * 
 * @return
 */
static void pp_set_current_alarm_digits(void)
{
    alarm_digits_t *alarm_digits_p = &display_digits.display_mode.alarm_digits;

    alarm_digits_p->alarm_mode = current_alarm.mode;

    alarm_digits_p->time.hour_first = current_alarm.hour / 10;
    alarm_digits_p->time.hour_second = current_alarm.hour % 10;
    alarm_digits_p->time.minute_first = current_alarm.minute / 10;
    alarm_digits_p->time.minute_second = current_alarm.minute % 10;

    switch (alarm_digits_p->alarm_mode)
    {
        case ALARM_SINGLE_MODE:
        {
            alarm_digits_p->arg.date.day_first = current_alarm.args.single_alarm_args.day / 10;
            alarm_digits_p->arg.date.day_second = current_alarm.args.single_alarm_args.day % 10;
            alarm_digits_p->arg.date.month_first = current_alarm.args.single_alarm_args.month / 10;
            alarm_digits_p->arg.date.month_second = current_alarm.args.single_alarm_args.month % 10;
            alarm_digits_p->arg.date.year_first = current_alarm.args.single_alarm_args.year / 10;
            alarm_digits_p->arg.date.year_second = current_alarm.args.single_alarm_args.year % 10;
            break;
        }

        case ALARM_WEEKLY_MODE:
        {
            alarm_digits_p->arg.days = current_alarm.args.days;
            break;
        }

        case ALARM_MONTHLY_MODE:
        {
            alarm_digits_p->arg.date.day_first = current_alarm.args.day / 10;
            alarm_digits_p->arg.date.day_second = current_alarm.args.day / 10;
            break;
        }

        case ALARM_YEARLY_MODE:
        {
            alarm_digits_p->arg.date.day_first = current_alarm.args.yearly_alarm_args.day / 10;
            alarm_digits_p->arg.date.day_second = current_alarm.args.yearly_alarm_args.day % 10;
            alarm_digits_p->arg.date.month_first = current_alarm.args.yearly_alarm_args.month / 10;
            alarm_digits_p->arg.date.month_second = current_alarm.args.yearly_alarm_args.month % 10;
            break;
        }
    }

    alarm_digits_p->volume = current_alarm.volume / 11;
}

/** @brief pp_set_current_ringtone_digits: Sets ringtone digits on the display during Ringtone List mode.
 * 
 * Reads current alarm parameters, calculates digits to be displayed and stores them in the structure.
 * 
 * @return
 */
static void pp_set_current_ringtone_digits(void)
{
    ringtone_digits_t *ringtone_digits_p = &display_digits.display_mode.ringtone_digits;

    ringtone_digits_p->number_third = ringtone_index % 10;
    ringtone_digits_p->number_second = (ringtone_index / 10) % 10;
    ringtone_digits_p->number_first = (ringtone_index / 100) % 10;
}