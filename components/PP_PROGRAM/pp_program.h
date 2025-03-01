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
#include "pp_gpio.h"
#include "pp_bluetooth.h"

/* Macros */
#define ALARM_FILE_TYPE ".txt"
#define RINGTONE_FILE_TYPE ".wav"

#define BUTTON_LEFT      0
#define BUTTON_CENTER    1
#define BUTTON_RIGHT     2

/* Structures */
typedef enum {
    WAIT_FOR_LEFT,
    WAIT_FOR_CENTER,
    WAIT_FOR_LEFT_LONG,
    WAIT_FOR_CENTER_LONG,
    PAIRING
}pairing_sm_t;

typedef enum {
    IDLE_TIME_CHANGE,
    SET_HOUR_FIRST,
    SET_HOUR_SECOND,
    SET_MINUTE_FIRST,
    SET_MINUTE_SECOND,
    SET_SECOND_FIRST,
    SET_SECOND_SECOND,
    SET_DAY_FIRST,
    SET_DAY_SECOND,
    SET_MONTH_FIRST,
    SET_MONTH_SECOND,
    SET_YEAR_FIRST,
    SET_YEAR_SECOND
}time_change_sm_t;

typedef enum {
    IDLE_ALARM_ADD,
    SET_MODE,
    SET_ALARM_HOUR_FIRST,
    SET_ALARM_HOUR_SECOND,
    SET_ALARM_MINUTE_FIRST,
    SET_ALARM_MINUTE_SECOND,
    SET_SINGLE_DAY_FIRST,
    SET_SINGLE_DAY_SECOND,
    SET_SINGLE_MONTH_FIRST,
    SET_SINGLE_MONTH_SECOND,
    SET_SINGLE_YEAR_FIRST,
    SET_SINGLE_YEAR_SECOND,
    SET_WEEKLY_MONDAY,
    SET_WEEKLY_TUESDAY,
    SET_WEEKLY_WEDNESDAY,
    SET_WEEKLY_THURSDAY,
    SET_WEEKLY_FRIDAY,
    SET_WEEKLY_SATURDAY,
    SET_WEEKLY_SUNDAY,
    SET_MONTHLY_DAY_FIRST,
    SET_MONTHLY_DAY_SECOND,
    SET_YEARLY_DAY_FIRST,
    SET_YEARLY_DAY_SECOND,
    SET_YEARLY_MONTH_FIRST,
    SET_YEARLY_MONTH_SECOND,
    SET_VOLUME
}alarm_add_sm_t;

/* Functions */

/** @brief pp_program_main: Program loop
 * 
 * This function is a anchor for all the user actions.
 * This function waits for a button action and redirect the operation to pp_button_functions.
 * 
 * @return
 */
void pp_program_main(void);