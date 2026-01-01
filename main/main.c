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
#include "pp_nvs.h"
#include "pp_i2c.h"
#include "pp_rtc.h"
#include "pp_bluetooth.h"
#include "pp_object_manager.h"
#include "pp_alarm.h"
#include "pp_wifi.h"
#include "pp_gpio.h"
#include "pp_led.h"
#include "pp_nixie_display_manager.h"
#include "pp_program.h"

/* Macros */
#define MAIN_TAG    "MAIN"

/* Functions */

/** @brief app_main: Main function/Entry point
 * 
 * This function is an entry point to the program.
 * Performs all necessary initializations
 *
 * @return
 */
void app_main(void)
{
    // Initializations
    pp_nvs_init();              // Initializes NVS Flash memory
    pp_i2c_init();              // Initializes I2C bus
    pp_object_manager_init();   // Initializes object manager
    pp_alarm_init();            // Initializes alarm functionality
    pp_rtc_init();              // Initializes RTC module functionality
    pp_gpio_init();             // Initializes gpio funcionality
    pp_led_init();              // Initializes LED funcionality

#ifdef DISPLAY_ENABLE
    pp_display_manager_init();  // Initializes display funcionality
#endif

    pp_bluetooth_init();        // Initializes bluetooth funcionality

#ifdef WIFI_ENABLE
    pp_wifi_init();             // Initializes WiFi functionality
#endif

    // Go to the program main
    pp_program_main();          // Redirect main loop to the program loop
}
