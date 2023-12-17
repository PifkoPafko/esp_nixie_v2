#include <string.h>
#include "esp_log.h"

#include <sys/time.h>
#include <time.h>
#include "mk_i2c.h"
#include "pp_rtc.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

static const char *TAG = "nixie-display";

esp_err_t pp_rtc_set_time(uint8_t seconds, uint8_t minutes, uint8_t hours, uint8_t dayOfWeek, uint8_t dayOfMonth, uint8_t month, uint8_t year)
{
    uint8_t outData[7];
    outData[0] = ((seconds / 10) << 4 ) | (seconds % 10);
    outData[1] = ((minutes / 10) << 4 ) | (minutes % 10);
    outData[2] = ((hours / 10) << 4 ) | (hours % 10);
    outData[3] = dayOfWeek;
    outData[4] = ((dayOfMonth / 10) << 4 ) | (dayOfMonth % 10);
    outData[5] = ((month / 10) << 4 ) | (month % 10);
    outData[6] = ((year / 10) << 4 ) | (year % 10);

    esp_err_t res = i2c_dev_write_reg(I2C_MASTER_NUM, DS_RTC_ADDR, DS_RTC_START_REG_ADDR, &outData, 7);
    ESP_ERROR_CHECK(res);
    return res;
}

esp_err_t pp_rtc_read_time(struct timeval *tv)
{
    uint8_t recData[7];
    memset(recData, 0, 7);

    esp_err_t res = i2c_dev_read_reg(I2C_MASTER_NUM, DS_RTC_ADDR, DS_RTC_START_REG_ADDR, recData, 7);
    ESP_ERROR_CHECK(res);

    uint8_t seconds = DS_SECONDS_TO_DEC(recData[0]);
    uint8_t minutes = DS_MINUTES_TO_DEC(recData[1]);
    uint8_t hours = DS_HOURS_24_TO_DEC(recData[2]);
    uint8_t day = DS_MONTH_DAY_TO_DEC(recData[4]);
    uint8_t month = DS_MONTH_TO_DEC(recData[5]);
    uint16_t year = DS_YEAR_TO_DEC(recData[6]);

    ESP_LOGI(TAG, "Time: %02d:%02d:%02d", hours, minutes, seconds);
    ESP_LOGI(TAG, "Date: %02d:%02d:%04d", day, month, year + 2000);

    if (tv)
    {
        struct tm tm;

        tm.tm_year 	= year + 100;
        tm.tm_mon 	= month - 1;
        tm.tm_mday 	= day;
        tm.tm_hour 	= hours;
        tm.tm_min 	= minutes;
        tm.tm_sec 	= seconds;
        tm.tm_isdst = 0; // 1 or 0	1-gdy czas ustawiany w czasie LETNIM, 0-gdy czas ustawiany w czasie ZIMOWYM

        time_t t = mktime(&tm);
        tv->tv_sec = t;
    }
    
    return res;
}

esp_err_t pp_rtc_init()
{
    uint8_t regVal = 0x1C;
    ESP_ERROR_CHECK(i2c_dev_write_reg(I2C_MASTER_NUM, DS_RTC_ADDR, DS_RTC_CONTROL_REG_ADDR, &regVal, 1));

    struct timeval now;
    pp_rtc_read_time(&now);
    settimeofday(&now, NULL);

    return ESP_OK;
}