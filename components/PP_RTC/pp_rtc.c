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
void pp_rtc_init(void);
{
    ESP_LOGI(RTC_TAG, "Initializing RTC");
    setenv("TZ", CENTRAL_EUROPEAN_TIME_ZONE, 1);
	tzset();
    
    uint8_t regVal = 0x1C;
    pp_i2c_dev_write_reg(DS_RTC_ADDR, DS_RTC_CONTROL_REG_ADDR, &regVal, 1);
    ESP_ERROR_CHECK(xTaskCreate(pp_rtc_main, "RTC", 3072, NULL, 2, &rtc_main_h));
}

/** @brief pp_rtc_set_time: Sets time to RTC
 *
 * @param[in]   timeinfo    (struct tm*) Pointer to the tm structure
 * @return
 */
void pp_rtc_set_time(struct tm *timeinfo)
{
    if(timeinfo == NULL) ESP_ERROR_CHECK(ESP_ERR_INVALID_ARG);

    uint8_t outData[7];
    outData[0] = ((timeinfo->tm_sec / 10) << 4 ) | (timeinfo->tm_sec % 10);
    outData[1] = ((timeinfo->tm_min / 10) << 4 ) | (timeinfo->tm_min % 10);
    outData[2] = ((timeinfo->tm_hour / 10) << 4 ) | (timeinfo->tm_hour % 10);
    outData[3] = timeinfo->tm_wday + 1;
    outData[4] = ((timeinfo->tm_mday / 10) << 4 ) | (timeinfo->tm_mday % 10);
    outData[5] = (((timeinfo->tm_mon + 1) / 10) << 4 ) | ((timeinfo->tm_mon + 1) % 10);
    outData[6] = (((timeinfo->tm_year - 100) / 10) << 4 ) | ((timeinfo->tm_year - 100) % 10);

    pp_i2c_dev_write_reg(DS_RTC_ADDR, DS_RTC_START_REG_ADDR, &outData, 7);
}

/** @brief pp_rtc_read_time: Reads time from RTC
 *
 * @param[out]   timeinfo    (struct tm*) Pointer to the tm structure
 * @return
 */
void pp_rtc_read_time(struct tm *timeinfo)
{
    if(timeinfo == NULL) ESP_ERROR_CHECK(ESP_ERR_INVALID_ARG);

    uint8_t recData[7];
    memset(recData, 0, 7);
    pp_i2c_dev_read_reg(DS_RTC_ADDR, DS_RTC_START_REG_ADDR, recData, 7);

    timeinfo->tm_sec = DS_SECONDS_TO_TM(recData[0]);
    timeinfo->tm_min = DS_MINUTES_TO_TM(recData[1]);
    timeinfo->tm_hour = DS_HOURS_24_TO_TM(recData[2]);
    timeinfo->tm_mday = DS_MONTH_DAY_TO_TM(recData[4]);
    timeinfo->tm_mon = DS_MONTH_TO_TM(recData[5]);
    timeinfo->tm_year = DS_YEAR_TO_TM(recData[6]);
    timeinfo->tm_isdst = -1;

    ESP_LOGI(RTC_TAG, "Time: %02d:%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
    ESP_LOGI(RTC_TAG, "Date: %02d:%02d:%04d", timeinfo->tm_mday, timeinfo->tm_mon + 1, timeinfo->tm_year + 1900);
}

