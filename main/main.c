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

#include "pp_object_manager.h"
#include "pp_nvs.h"
#include "pp_wifi.h"
#include "pp_i2c.h"
#include "pp_rtc.h"
#include "pp_nixie_display_manager.h"

#define MAIN_TAG    "MAIN"

void app_main(void)
{
    // Initializations
    pp_nvs_init();
    pp_i2c_init();
    pp_rtc_init();
    // pp_bluetooth_init();
    pp_object_manager_init();
    pp_alarm_init();
    pp_wifi_init();

    // Go to the main program
    pp_program_init();
}
