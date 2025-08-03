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

#ifndef __GLOBAL_H__
#define __GLOBAL_H__

/* Macros */
#define NOTIFY_TASK(x, y) (xTaskNotify(x, y, eSetValueWithOverwrite))
#define NOTIFY_TASK_FROM_ISR(x, y) (xTaskNotifyFromISR(x, y, eSetValueWithOverwrite, NULL))

/* Alarm mode */
#define ALARM_SINGLE_MODE   0
#define ALARM_WEEKLY_MODE   1
#define ALARM_MONTHLY_MODE  2
#define ALARM_YEARLY_MODE   3

/* Variables declarations */
extern TaskHandle_t display_main_h;
extern TaskHandle_t button_main_h;
extern TaskHandle_t rtc_main_h;
extern TaskHandle_t alarm_main_h;
extern TaskHandle_t wifi_main_h;
extern const uint8_t alarm_type_uuid[ESP_UUID_LEN_128];
extern const uint8_t ringtone_type_uuid[ESP_UUID_LEN_128];

/* Button action queue handle */
extern QueueHandle_t button_action_queue;

/* Display digits description */
extern display_digits_t display_digits;

#endif