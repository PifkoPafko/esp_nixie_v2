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

#ifndef __NIXIE_DISPLAY_MANAGER_H__
#define __NIXIE_DISPLAY_MANAGER_H__

/* Headers */
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"

#include "pp_nixie_display.h"

/* Macros */
#define NOTIFY_NORMAL_VAL 0
#define NOTIFY_TIMER_VAL 1
#define NOTIFY_TIMER_BLINK_VAL 2
#define NOTIFY_TIMER_ANTI_POISONING_VAL 3

#define DEFAULT_PERIOD pdMS_TO_TICKS(1000)
#define BLINK_PERIOD pdMS_TO_TICKS(500)
#define ANTI_POISON_PERIOD pdMS_TO_TICKS(60000)
#define ANTI_POISON_DIGIT_PERIOD pdMS_TO_TICKS(100)

/* Functions */

/** @brief pp_display_manager_init: Initializes display manager and calls init function for display.
 *
 * @param[in]   display_digits  (nixie_tube_state_t*) Pointer to nixie_tube_state_t structure.
 * 
 * @return
 */
void pp_display_manager_init(nixie_tube_state_t *display_digits);

/** @brief pp_update_display: Update state of the display.
 * 
 * @return
 */
void pp_update_display(void);

/** @brief pp_set_display_passkey: Set bluetooth passkey to display afterwards
 *
 * @param[in]   key  (uint32_t) Bluetooth passkey.
 * 
 * @return
 */
void pp_set_display_passkey(uint32_t key);

/** @brief pp_check_display_ready: Return if display is ready to change
 * 
 * Display is always ready to change except when there is undergoing anti-poisoning precudure (in default mode it lasts 1 second every 1 minute)
 * 
 * @return (Bool) True if display ready to change, false - display is not ready to change
 */
bool pp_check_display_ready(void);

#endif