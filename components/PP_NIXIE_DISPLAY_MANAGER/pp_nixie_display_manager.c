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
#include "pp_nixie_display_manager.h"

/* Declarations */
static static void pp_timer_cb(TimerHandle_t xTimer);
static static void pp_timer_anti_poison_cb(TimerHandle_t xTimer);
static inline void pp_timer_set_default(void);
static inline void pp_timer_set_blink(void);
static inline void pp_timer_stop(void);
static void pp_set_nixie_state_default();
static void pp_set_nixie_state_time_change();
static void pp_set_nixie_state_alarm();
static void pp_set_nixie_state_alarm_single();
static void pp_set_nixie_state_alarm_weekly();
static void pp_set_nixie_state_alarm_monthly();
static void pp_set_nixie_state_alarm_yearly();
static void pp_set_nixie_state_pairing_passkey();

static void pp_nixie_display_main(void* arg);

/* Variables */
static display_state_t display_state;
static device_mode_t *device_mode_p;
static display_digits_t *display_digits_p;

static uint8_t passkey[PASSKEY_SIZE];

static TaskHandle_t display_main_h;
static TimerHandle_t timer_h;
static TimerHandle_t timer_anti_poison_h;
static bool anti_poisoning_ongoing = false;

/* Functions */

/** @brief pp_display_manager_init: Initializes display manager and calls init function for display.
 *
 * @param[in]   device_mode  (device_mode_t*) Pointer to device_mode_t structure.
 * @param[in]   display_digits  (nixie_tube_state_t*) Pointer to nixie_tube_state_t structure.
 * 
 * @return
 */
void pp_display_manager_init(device_mode_t *device_mode, nixie_tube_state_t *display_digits)
{
    ESP_LOGI(NIXIE_DISPLAY_MANAGER_TAG, "Initializing NIXIE Display Manager");
    device_mode_p = device_mode;
    display_digits_p = display_digits;

    pp_nixie_display_init();
    
    ESP_ERROR_CHECK(xTaskCreate(pp_display_main, "NIXIE DISPLAY", 4096, NULL, 1, &display_main_h));

    timer_h = xTimerCreate(NULL, DEFAULT_PERIOD, pdTRUE, NULL, pp_timer_cb);
    timer_anti_poison_h = xTimerCreate(NULL, ANTI_POISON_PERIOD, pdTRUE, NULL, pp_timer_anti_poison_cb);

    pp_update_display(DEFAULT_MODE);
    pp_timer_set_default();
}

/** @brief pp_update_display: Update state of the display.
 * 
 * @return
 */
void pp_update_display()
{
    switch(display_mode)
    {
        case DEFAULT_MODE:
        {
            pp_timer_set_default();
            break;
        }

        case TIME_CHANGE_MODE:
        case ALARM_ADD_MODE:
        case PAIRING_MODE:
        case ALARM_RING_MODE:
        {
            pp_timer_set_blink();
        }

        case ALARM_DELETE_MODE:
        case PAIRING_PASSKEY_MODE:
        {
            pp_timer_stop();
            break;
        }

        default:
        {
            ESP_ERROR_CHECK(ESP_FAIL);
        }
    }

    UPDATE_DISPLAY(main_task_h, NOTIFY_NORMAL_VAL);
}

/** @brief pp_set_display_passkey: Set bluetooth passkey to display afterwards
 *
 * @param[in]   key  (uint32_t) Bluetooth passkey.
 * 
 * @return
 */
void pp_set_display_passkey(uint32_t key)
{
    for (uint i=0; i<6; i++)
    {
        passkey[5-i] = key % 10;
        key /= 10;
    }
}

/** @brief pp_check_display_ready: Return if display is ready to change
 * 
 * Display is always ready to change except when there is undergoing anti-poisoning precudure (in default mode it lasts 1 second every 1 minute)
 * 
 * @return (Bool) True if display ready to change, false - display is not ready to change
 */
bool pp_check_display_ready(void)
{
    return anti_poisoning_ongoing;
}

/** @brief pp_display_main: Display Main Task
 * 
 * The task executing this fucntion is created during display manager initialization. After creation, the task is waiting for notification to take an action.
 * 
 * There are 4 types of notifications:
 *  - NOTIFY_NORMAL_VAL                 : Action requested by main program
 *  - NOTIFY_TIMER_VAL                  : Action requested by local timer in default mode to change displayed time and date
 *  - NOTIFY_TIMER_BLINK_VAL            : Action requested by local timer blink one or more lamps in different modes
 *  - NOTIFY_TIMER_ANTI_POISONING_VAL   : Action requested by local timer to perform anti-poisoning procedure
 * 
 *  The task updates the display according to the received notification and current display mode.
 * 
 * @param[in]   arg  (void*) Reserved
 * 
 * @return
 */
static void pp_display_main(void* arg)
{
    while(true)
    {
        uint32_t notify_value;
        if(xTaskNotifyWait(0, 0, &notify_value, portMAX_DELAY) != pdTRUE) continue;

        if(notify_value == NOTIFY_TIMER_ANTI_POISONING_VAL)
        {
            anti_poisoning_ongoing = true;
            memset(display_state.digit_enable, true, TUBES_COUNT*sizeof(display_state.digit_enable[0]));
            memset(display_state.right_comma_enable, true, TUBES_COUNT*sizeof(display_state.right_comma_enable[0]));
            memset(display_state.left_comma_enable, true, TUBES_COUNT*sizeof(display_state.left_comma_enable[0]));

            for(uint8_t tube = 0; tube < TUBES_COUNT; ++tube)
            {
                memset(display_state.digit, tube, TUBES_COUNT*sizeof(display_state.left_comma_enable[0]));
                vTaskDelay(ANTI_POISON_DIGIT_PERIOD);
            }
            anti_poisoning_ongoing = false;
        }
        else
        {
            switch(*device_mode_p)
            {
                case DEFAULT_MODE:
                {
                    pp_set_nixie_state_default();
                    break;
                }

                case TIME_CHANGE_MODE:
                {
                    if(notify_value == NOTIFY_TIMER_BLINK_VAL)
                    {
                        display_state.digit_enable[display_digits_p->blink_tube] = !display_state.digit_enable[display_digits_p->blink_tube];
                    }
                    else
                    {
                        pp_set_nixie_state_time_change();
                    }
                    break;
                }

                case ALARM_ADD_MODE:
                {
                    if(notify_value == NOTIFY_TIMER_BLINK_VAL)
                    {
                        display_state.digit_enable[display_digits_p->blink_tube] = !display_state.digit_enable[display_digits_p->blink_tube];
                    }
                    else
                    {
                        pp_set_nixie_state_alarm();
                        
                    }
                    break;
                }

                case ALARM_DELETE_MODE:
                {
                    pp_set_nixie_state_alarm();
                    break;
                }

                case PAIRING_MODE:
                {
                    if(notify_value == NOTIFY_TIMER_BLINK_VAL)
                    {
                        memset(display_state.right_comma_enable, !display_state.right_comma_enable[0], TUBES_COUNT*sizeof(display_state.right_comma_enable[0]));
                    }
                    else
                    {
                        memset(display_state.digit_enable, false, TUBES_COUNT*sizeof(display_state.digit_enable[0]));
                        memset(display_state.left_comma_enable, false, TUBES_COUNT*sizeof(display_state.left_comma_enable[0]));
                        memset(display_state.right_comma_enable, true, TUBES_COUNT*sizeof(display_state.right_comma_enable[0]));
                    }
                    break;
                }

                case PAIRING_PASSKEY_MODE:
                {
                    pp_set_nixie_state_pairing_passkey();
                    break;
                }

                case ALARM_RING_MODE:
                {
                    pp_set_nixie_state_default();
                    if(notify_value == NOTIFY_TIMER_BLINK_VAL)
                    {
                        bool enable = !display_state.digit_enable[0];
                        memset(display_state.digit_enable, enable, TUBES_COUNT*sizeof(display_state.right_comma_enable[0]));
                        display_state.right_comma_enable[1] = enable;
                        display_state.right_comma_enable[3] = enable;
                        display_state.right_comma_enable[8] = enable;
                        display_state.right_comma_enable[10] = enable;
                    }
                    break;
                }
            }
        }

        pp_display(&nixie_state);
    }
}

/** @brief pp_timer_cb: General Timer Callback
 * 
 * Neccessary for changing displayed datetime and blinking tubes/commas in different modes.
 * 
 * General Timer sends notification to the main task when:
 *  - Display mode is default and 1 second has passed.
 *  - Display modes that uses blinking and 500 ms have passed.
 * 
 * @param[in]   xTimer  (TimerHandle_t) Timer handle.
 * 
 * @return
 */
static void pp_timer_cb(TimerHandle_t xTimer)
{
    if(*device_mode_p == TIME_CHANGE_MODE || *device_mode_p == ALARM_ADD_MODE)
    {
        UPDATE_DISPLAY(main_task_h, NOTIFY_TIMER_BLINK_VAL);
    }
    else
    {
        UPDATE_DISPLAY(main_task_h, NOTIFY_TIMER_VAL);
    }
}

/** @brief pp_timer_anti_poison_cb: Anti-poisoning Timer Callback
 * 
 * Neccessary to perform anti-poisoning procedure every minute in default mode.
 * 
 * Anti-poisoning Timer sends notification to the main task when:
 *  - Display mode is default and 60 second has passed since the last mode change or the last anti-poisoning proceure.
 * 
 * @param[in]   xTimer  (TimerHandle_t) Timer handle.
 * 
 * @return
 */
static void pp_timer_anti_poison_cb(TimerHandle_t xTimer)
{
    UPDATE_DISPLAY(main_task_h, NOTIFY_TIMER_ANTI_POISONING_VAL);
}


/** @brief pp_timer_set_default: Setting General and Anti-poisoning timers to work in default mode
 * 
 * @return
 */
static inline void pp_timer_set_default(void)
{
    ESP_ERROR_CHECK(xTimerStop(timer_h, 10));
    ESP_ERROR_CHECK(xTimerChangePeriod(timer_h, DEFAULT_PERIOD, 10));
    ESP_ERROR_CHECK(xTimerReset(timer_h, 10));
    ESP_ERROR_CHECK(xTimerReset(timer_anti_poison_h, 10));
}

/** @brief pp_timer_set_blink: Setting General and Anti-poisoning timers to work in blinking mode
 * 
 * @return
 */
static inline void pp_timer_set_blink(void)
{
    ESP_ERROR_CHECK(xTimerStop(timer_anti_poison_h, 10));
    ESP_ERROR_CHECK(xTimerStop(timer_h, 10));
    ESP_ERROR_CHECK(xTimerChangePeriod(timer_h, BLINK_PERIOD, 10));
    ESP_ERROR_CHECK(xTimerReset(timer_h, 10));
}

/** @brief pp_timer_stop: Stops General and Anti-poisoning timers.
 * 
 * @return
 */
static inline void pp_timer_stop(void)
{
    ESP_ERROR_CHECK(xTimerStop(timer_anti_poison_h, 10));
    ESP_ERROR_CHECK(xTimerStop(timer_h, 10));
}


/** @brief pp_set_nixie_state_default: Prepare data to display in DEFAULT MODE
 * 
 * Read current datetime and sets display_state structure as below:
 * 
 * [HOUR] [HOUR,] [MIN] [MIN,] [SEC] [SEC] [] [DAY] [DAY,] [MON] [MON,] [YEAR] [YEAR] [YEAR] [YEAR] []
 * 
 * @return
 */
static void pp_set_nixie_state_default()
{
    time_t now;
    struct tm timeinfo;

    time(&now);
    localtime_r(&now, &timeinfo);

    memset(display_state.digit_enable, true, TUBES_COUNT*sizeof(display_state.digit_enable[0]));
    memset(display_state.right_comma_enable, false, TUBES_COUNT*sizeof(display_state.right_comma_enable[0]));
    memset(display_state.left_comma_enable, false, TUBES_COUNT*sizeof(display_state.left_comma_enable[0]));

    display_state.digit_enable[6] = false;
    display_state.digit_enable[13] = false;
    display_state.digit_enable[14] = false;
    display_state.digit_enable[15] = false;

    display_state.digit[0] = timeinfo.tm_hour / 10;
    display_state.digit[1] = timeinfo.tm_hour % 10;
    display_state.digit[2] = timeinfo.tm_min / 10;
    display_state.digit[3] = timeinfo.tm_min % 10;
    display_state.digit[4] = timeinfo.tm_sec / 10;
    display_state.digit[5] = timeinfo.tm_sec % 10;
    display_state.digit[7] = timeinfo.tm_mday / 10;
    display_state.digit[8] = timeinfo.tm_mday % 10;
    display_state.digit[9] = (timeinfo.tm_mon + 1) / 10;
    display_state.digit[10] = (timeinfo.tm_mon + 1) % 10;
    display_state.digit[11] = (timeinfo.tm_year - 100) / 10;
    display_state.digit[12] = (timeinfo.tm_year - 100) % 10;

    display_state.right_comma_enable[1] = true;
    display_state.right_comma_enable[3] = true;
    display_state.right_comma_enable[8] = true;
    display_state.right_comma_enable[10] = true;
}

/** @brief pp_set_nixie_state_time_change: Prepare data to display in TIME CHANGE MODE
 * 
 * Sets display_state structure using time and date provided by user as below:
 * 
 * [HOUR] [HOUR,] [MIN] [MIN,] [SEC] [SEC] [] [DAY] [DAY,] [MON] [MON,] [YEAR] [YEAR] [YEAR] [YEAR] []
 * 
 * @return
 */
static void pp_set_nixie_state_time_change()
{
    memset(display_state.digit_enable, true, TUBES_COUNT*sizeof(display_state.digit_enable[0]));
    memset(display_state.right_comma_enable, false, TUBES_COUNT*sizeof(display_state.right_comma_enable[0]));
    memset(display_state.left_comma_enable, false, TUBES_COUNT*sizeof(display_state.left_comma_enable[0]));

    display_state.digit_enable[6] = false;
    display_state.digit_enable[13] = false;
    display_state.digit_enable[14] = false;
    display_state.digit_enable[15] = false;

    display_state.digit[0] = display_digits_p->display_mode.time_date_digits.time.hour_first;
    display_state.digit[1] = display_digits_p->display_mode.time_date_digits.time.hour_second;
    display_state.digit[2] = display_digits_p->display_mode.time_date_digits.time.minute_first;
    display_state.digit[3] = display_digits_p->display_mode.time_date_digits.time.minute_second;
    display_state.digit[4] = display_digits_p->display_mode.time_date_digits.time.second_first;
    display_state.digit[5] = display_digits_p->display_mode.time_date_digits.time.second_second;
    display_state.digit[7] = display_digits_p->display_mode.time_date_digits.date.day_first;
    display_state.digit[8] = display_digits_p->display_mode.time_date_digits.date.day_second;
    display_state.digit[9] = display_digits_p->display_mode.time_date_digits.date.month_first;
    display_state.digit[10] = display_digits_p->display_mode.time_date_digits.date.month_second;
    display_state.digit[11] = display_digits_p->display_mode.time_date_digits.date.year_first;
    display_state.digit[12] = display_digits_p->display_mode.time_date_digits.date.year_second;

    display_state.right_comma_enable[1] = true;
    display_state.right_comma_enable[3] = true;
    display_state.right_comma_enable[8] = true;
    display_state.right_comma_enable[10] = true;
}

/** @brief pp_set_nixie_state_alarm: Prepare data to display in all ALARM modes.
 * 
 * @return
 */
static void pp_set_nixie_state_alarm()
{
    switch(display_digits_p->mode.alarm_digits.mode)
    {
        case ALARM_SINGLE_MODE:
        {
            pp_set_nixie_state_alarm_single();
            break;
        }

        case ALARM_WEEKLY_MODE:
        {
            pp_set_nixie_state_alarm_weekly();
            break;
        }

        case ALARM_MONTHLY_MODE:
        {
            pp_set_nixie_state_alarm_monthly();
            break;
        }

        case ALARM_YEARLY_MODE:
        {
            pp_set_nixie_state_alarm_yearly();
            break;
        }
    }
}

/** @brief pp_set_nixie_state_alarm_single: Prepare data to display in ALARM_SINGLE_MODE
 * 
 * Sets display_state structure using alarm parameters provided by user as below:
 * 
 * [MODE] [] [HOUR] [HOUR,] [MIN] [MIN] [] [DAY] [DAY,] [MON] [MON,] [YEAR] [YEAR] [] [VOL]
 * 
 * MODE - ALARM MODE
 * VOL - VOLUME
 * 
 * @return
 */
static void pp_set_nixie_state_alarm_single()
{
    memset(display_state.digit_enable, true, TUBES_COUNT*sizeof(display_state.digit_enable[0]));
    memset(display_state.right_comma_enable, false, TUBES_COUNT*sizeof(display_state.right_comma_enable[0]));
    memset(display_state.left_comma_enable, false, TUBES_COUNT*sizeof(display_state.left_comma_enable[0]));

    display_state.digit_enable[1] = false;
    display_state.digit_enable[6] = false;
    display_state.digit_enable[13] = false;
    display_state.digit_enable[14] = false;

    display_state.digit[0] = display_digits_p->display_mode.alarm_digits.alarm_mode;
    display_state.digit[2] = display_digits_p->display_mode.alarm_digits.time.hour_first;
    display_state.digit[3] = display_digits_p->display_mode.alarm_digits.time.hour_second;
    display_state.digit[4] = display_digits_p->display_mode.alarm_digits.time.minute_first;
    display_state.digit[5] = display_digits_p->display_mode.alarm_digits.time.minute_second;
    display_state.digit[7] = display_digits_p->display_mode.alarm_digits.arg.date.day_first;
    display_state.digit[8] = display_digits_p->display_mode.alarm_digits.arg.date.day_second;
    display_state.digit[9] = display_digits_p->display_mode.alarm_digits.arg.date.month_first;
    display_state.digit[10] = display_digits_p->display_mode.alarm_digits.arg.date.month_second;
    display_state.digit[11] = display_digits_p->display_mode.alarm_digits.arg.date.year_first;
    display_state.digit[12] = display_digits_p->display_mode.alarm_digits.arg.date.year_second;
    display_state.digit[15] = display_digits_p->display_mode.alarm_digits.volume;

    display_state.right_comma_enable[3] = true;
    display_state.right_comma_enable[8] = true;
    display_state.right_comma_enable[10] = true;
}

/** @brief pp_set_nixie_state_alarm_weekly: Prepare data to display in ALARM_WEEKLY_MODE
 * 
 * Sets display_state structure using alarm parameters provided by user as below:
 * 
 * [MODE] [] [HOUR] [HOUR,] [MIN] [MIN] [] [MON] [TUE] [WED] [THU] [FRI] [SAT] [SUN] [] [VOL]
 * 
 * MODE - ALARM MODE
 * VOL - VOLUME
 * 
 * @return
 */
static void pp_set_nixie_state_alarm_weekly()
{
    memset(display_state.digit_enable, true, TUBES_COUNT*sizeof(display_state.digit_enable[0]));
    memset(display_state.right_comma_enable, false, TUBES_COUNT*sizeof(display_state.right_comma_enable[0]));
    memset(display_state.left_comma_enable, false, TUBES_COUNT*sizeof(display_state.left_comma_enable[0]));

    display_state.digit_enable[1] = false;
    display_state.digit_enable[6] = false;
    display_state.digit_enable[14] = false;

    display_state.digit[0] = display_digits_p->display_mode.alarm_digits.alarm_mode;
    display_state.digit[2] = display_digits_p->display_mode.alarm_digits.time.hour_first;
    display_state.digit[3] = display_digits_p->display_mode.alarm_digits.time.hour_second;
    display_state.digit[4] = display_digits_p->display_mode.alarm_digits.time.minute_first;
    display_state.digit[5] = display_digits_p->display_mode.alarm_digits.time.minute_second;

    for(uint8_t tube = 7, shift = 0; tube <= 13; ++tube, ++shift)
    {
        display_state.digit[tube] = display_digits_p->display_mode.alarm_digits.arg.days & (1<<shift) ? 1 : 0;
    }

    display_state.digit[15] = display_digits_p->display_mode.alarm_digits.volume;

    nixie_state[3].right_comma_enable = true;
}

/** @brief pp_set_nixie_state_alarm_monthly: Prepare data to display in ALARM_MONTHLY_MODE
 * 
 * Sets display_state structure using alarm parameters provided by user as below:
 * 
 * [MODE] [] [HOUR] [HOUR,] [MIN] [MIN] [] [DAY] [DAY] [] [] [] [] [] [] [VOL]
 * 
 * MODE - ALARM MODE
 * VOL - VOLUME
 * 
 * @return
 */
static void pp_set_nixie_state_alarm_monthly()
{
    memset(display_state.digit_enable, true, TUBES_COUNT*sizeof(display_state.digit_enable[0]));
    memset(display_state.right_comma_enable, false, TUBES_COUNT*sizeof(display_state.right_comma_enable[0]));
    memset(display_state.left_comma_enable, false, TUBES_COUNT*sizeof(display_state.left_comma_enable[0]));

    display_state.digit_enable[1] = false;
    display_state.digit_enable[6] = false;
    display_state.digit_enable[9] = false;
    display_state.digit_enable[10] = false;
    display_state.digit_enable[11] = false;
    display_state.digit_enable[12] = false;
    display_state.digit_enable[13] = false;
    display_state.digit_enable[14] = false;

    display_state.digit[0] = display_digits_p->display_mode.alarm_digits.alarm_mode;
    display_state.digit[2] = display_digits_p->display_mode.alarm_digits.time.hour_first;
    display_state.digit[3] = display_digits_p->display_mode.alarm_digits.time.hour_second;
    display_state.digit[4] = display_digits_p->display_mode.alarm_digits.time.minute_first;
    display_state.digit[5] = display_digits_p->display_mode.alarm_digits.time.minute_second;
    display_state.digit[7] = display_digits_p->display_mode.alarm_digits.arg.date.day_first;
    display_state.digit[8] = display_digits_p->display_mode.alarm_digits.arg.date.day_second;
    display_state.digit[15] = display_digits_p->display_mode.alarm_digits.volume;

    display_state.right_comma_enable[3] = true;
}

/** @brief pp_set_nixie_state_alarm_yearly: Prepare data to display in ALARM_YEARLY_MODE
 * 
 * Sets display_state structure using alarm parameters provided by user as below:
 * 
 * [MODE] [] [HOUR] [HOUR,] [MIN] [MIN] [] [DAY] [DAY] [MON] [MON] [] [] [] [] [VOL]
 * 
 * MODE - ALARM MODE
 * VOL - VOLUME
 * 
 * @return
 */
static void pp_set_nixie_state_alarm_yearly()
{
    memset(display_state.digit_enable, true, TUBES_COUNT*sizeof(display_state.digit_enable[0]));
    memset(display_state.right_comma_enable, false, TUBES_COUNT*sizeof(display_state.right_comma_enable[0]));
    memset(display_state.left_comma_enable, false, TUBES_COUNT*sizeof(display_state.left_comma_enable[0]));

    display_state.digit_enable[1] = false;
    display_state.digit_enable[6] = false;
    display_state.digit_enable[11] = false;
    display_state.digit_enable[12] = false;
    display_state.digit_enable[13] = false;
    display_state.digit_enable[14] = false;

    display_state.digit[0] = display_digits_p->display_mode.alarm_digits.alarm_mode;
    display_state.digit[2] = display_digits_p->display_mode.alarm_digits.time.hour_first;
    display_state.digit[3] = display_digits_p->display_mode.alarm_digits.time.hour_second;
    display_state.digit[4] = display_digits_p->display_mode.alarm_digits.time.minute_first;
    display_state.digit[5] = display_digits_p->display_mode.alarm_digits.time.minute_second;
    display_state.digit[7] = display_digits_p->display_mode.alarm_digits.arg.date.day_first;
    display_state.digit[8] = display_digits_p->display_mode.alarm_digits.arg.date.day_second;
    display_state.digit[9] = display_digits_p->display_mode.alarm_digits.arg.date.month_first;
    display_state.digit[10] = display_digits_p->display_mode.alarm_digits.arg.date.month_second;
    display_state.digit[15] = display_digits_p->display_mode.alarm_digits.volume;

    display_state.right_comma_enable[3] = true;
    display_state.right_comma_enable[8] = true;
}

/** @brief pp_set_nixie_state_pairing_passkey: Prepare data to display in PAIRING_PASSKEY_MODE
 * 
 * Sets display_state structure using alarm parameters provided by user as below:
 * 
 * [P0] [P1] [P2] [P3] [P4] [P5] [] [] [] [] [] [] [] [] [] []
 * 
 * P0..P6 : Bluetooth passkey
 * 
 * @return
 */
static void pp_set_nixie_state_pairing_passkey()
{
    memset(display_state.digit_enable, true, PASSKEY_SIZE*sizeof(display_state.digit_enable[0]));
    memset(&display_state.digit_enable[PASSKEY_SIZE], false, (TUBES_COUNT-PASSKEY_SIZE)*sizeof(display_state.digit_enable[0]));
    memset(display_state.right_comma_enable, false, TUBES_COUNT*sizeof(display_state.right_comma_enable[0]));
    memset(display_state.left_comma_enable, false, TUBES_COUNT*sizeof(display_state.left_comma_enable[0]));
    memcpy(display_state.digit, passkey, PASSKEY_SIZE*sizeof(display_state.digit[0]));
}