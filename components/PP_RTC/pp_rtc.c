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
#include "pp_rtc.h"

/* Macros */
#define RTC_TAG "RTC"

/* Variables */
static i2c_master_dev_handle_t rtc_dev_handle;

/* Functions */

/** @brief pp_rtc_main: RTC Task main function
 *
 * @param[in]   arg (void*) Reserved
 * @return
 */
static void pp_rtc_main(void* arg)
{
    // TODO: move this to alarm handling
    while (true)
    {
        struct tm timeinfo;
        pp_rtc_read_time(&timeinfo);
        time_t t = mktime(&timeinfo);
        struct timeval now;
        now.tv_sec = t;
        settimeofday(&now, NULL);
        ESP_LOGI(RTC_TAG, "Time updated from RTC");

        pp_set_next_alarm();
        vTaskDelay(3600000 / portTICK_PERIOD_MS);
    }
}

/** @brief pp_rtc_init: Initializes RTC
 *
 * @return
 */
void pp_rtc_init(void)
{
    ESP_LOGI(RTC_TAG, "Initializing RTC");
    setenv("TZ", CENTRAL_EUROPEAN_TIME_ZONE, 1);
	tzset();

    i2c_master_bus_handle_t bus_handle;
    ESP_ERROR_CHECK(i2c_master_get_bus_handle(0, &bus_handle));

    i2c_device_config_t dev_cfg_rtc = {
    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
    .device_address = DS_RTC_ADDR,
    .scl_speed_hz = 100000,
    };

    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg_rtc, &rtc_dev_handle));

    uint8_t data[2] = {DS_RTC_CONTROL_REG_ADDR, 0x1C};
    i2c_master_transmit(rtc_dev_handle, data, 2, -1);

    BaseType_t res = xTaskCreate(pp_rtc_main, "RTC", 3072, NULL, 2, &rtc_main_h);
    if(res != pdPASS)
    {
        ESP_ERROR_CHECK(ESP_FAIL);
    }
}

/** @brief pp_rtc_set_time: Sets time to RTC
 *
 * @param[in]   timeinfo    (struct tm*) Pointer to the tm structure
 * @return
 */
void pp_rtc_set_time(struct tm *timeinfo)
{
    if(timeinfo == NULL) ESP_ERROR_CHECK(ESP_ERR_INVALID_ARG);

    uint8_t outData[8];
    outData[0] = DS_RTC_START_REG_ADDR;
    outData[1] = ((timeinfo->tm_sec / 10) << 4 ) | (timeinfo->tm_sec % 10);
    outData[2] = ((timeinfo->tm_min / 10) << 4 ) | (timeinfo->tm_min % 10);
    outData[3] = ((timeinfo->tm_hour / 10) << 4 ) | (timeinfo->tm_hour % 10);
    outData[4] = timeinfo->tm_wday + 1;
    outData[5] = ((timeinfo->tm_mday / 10) << 4 ) | (timeinfo->tm_mday % 10);
    outData[6] = (((timeinfo->tm_mon + 1) / 10) << 4 ) | ((timeinfo->tm_mon + 1) % 10);
    outData[7] = (((timeinfo->tm_year - 100) / 10) << 4 ) | ((timeinfo->tm_year - 100) % 10);

    i2c_master_transmit(rtc_dev_handle, outData, 8, -1);
}

/** @brief pp_rtc_read_time: Reads time from RTC
 *
 * @param[out]   timeinfo    (struct tm*) Pointer to the tm structure
 * @return
 */
void pp_rtc_read_time(struct tm *timeinfo)
{
    if(timeinfo == NULL) ESP_ERROR_CHECK(ESP_ERR_INVALID_ARG);

    uint8_t data_out = DS_RTC_START_REG_ADDR;
    uint8_t data_in[7];
    memset(data_in, 0, 7);
    i2c_master_transmit_receive(rtc_dev_handle, &data_out, 1, data_in, 7, -1);

    timeinfo->tm_sec = DS_SECONDS_TO_TM(data_in[0]);
    timeinfo->tm_min = DS_MINUTES_TO_TM(data_in[1]);
    timeinfo->tm_hour = DS_HOURS_24_TO_TM(data_in[2]);
    timeinfo->tm_mday = DS_MONTH_DAY_TO_TM(data_in[4]);
    timeinfo->tm_mon = DS_MONTH_TO_TM(data_in[5]);
    timeinfo->tm_year = DS_YEAR_TO_TM(data_in[6]);
    timeinfo->tm_isdst = -1;

    ESP_LOGI(RTC_TAG, "Time: %02d:%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
    ESP_LOGI(RTC_TAG, "Date: %02d:%02d:%04d", timeinfo->tm_mday, timeinfo->tm_mon + 1, timeinfo->tm_year + 1900);
}

