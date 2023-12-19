#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#define CENTRAL_EUROPEAN_TIME_ZONE	"CET-1CEST,M3.5.0/2,M10.5.0/3"  // for Poland

esp_err_t pp_rtc_init();
esp_err_t pp_rtc_set_time(uint8_t seconds, uint8_t minutes, uint8_t hours, uint8_t dayOfWeek, uint8_t dayOfMonth, uint8_t month, uint8_t year);
esp_err_t pp_rtc_read_time(struct timeval *tv);

#define I2C_MASTER_NUM  0
#define I2C_PORT_0      0
#define I2C_PORT_1      1

#define WRITE_BIT_MASK(x) (((x) << 1) & 0xFE)
#define READ_BIT_MASK(x) (((x) << 1) | 0x01)

#define DS_RTC_ADDR                     0b11010000

#define DS_RTC_START_REG_ADDR           0x00
#define DS_RTC_CONTROL_REG_ADDR         0x0E
#define DS_EOSC_FLAG                    (1<<7)

#define DS_SECONDS_TEN_MSK              0b01110000
#define DS_SECONDS_MSK                  0b00001111

#define DS_MINUTES_TEN_MSK              0b01110000
#define DS_MINUTES_MSK                  0b00001111

#define DS_HOURS_MODE_MSK               0b01000000
#define DS_HOURS_AMPM_MSK               0b00100000
// #define DS_HOURS_TWENTY_MSK             0b00100000
#define DS_HOURS_TEN_MSK                0b00110000
#define DS_HOURS_MSK                    0b00001111

#define DS_WEEK_DAY_MSK                 0b00000111

#define DS_MONTH_DAY_TEN_MSK            0b00110000
#define DS_MONTH_DAY_MSK                0b00001111
#define DS_CENTURY_MSK                  0b10000000

#define DS_MONTH_TEN_MSK                0b00010000
#define DS_MONTH_MSK                    0b00001111

#define DS_YEAR_TEN_MSK                 0b11110000
#define DS_YEAR_MSK                     0b00001111

#define DS_SECONDS_TO_DEC(x)    ((((DS_SECONDS_TEN_MSK & (x)) >> 4) * 10) + (DS_SECONDS_MSK & (x)))
#define DS_MINUTES_TO_DEC(x)    ((((DS_MINUTES_TEN_MSK & (x)) >> 4) * 10) + (DS_MINUTES_MSK & (x)))
#define DS_HOURS_12_TO_DEC(x)   ((((DS_HOURS_TEN_MSK & (x)) * 10) >> 4) + (DS_HOURS_MSK & (x)))
// #define DS_HOURS_24_TO_DEC(x)   ((((DS_HOURS_TWENTY_MSK & (x)) * 20) >> 5) + (((DS_HOURS_TEN_MSK & (x)) * 10) >> 4) + (DS_HOURS_MSK & (x)))
#define DS_HOURS_24_TO_DEC(x)   ((((DS_HOURS_TEN_MSK & (x)) >> 4) * 10) + (DS_HOURS_MSK & (x)))
#define DS_WEEK_DAY_TO_DEC(x)   (DS_WEEK_DAY_MSK & (x))
#define DS_MONTH_DAY_TO_DEC(x)  ((((DS_MONTH_DAY_TEN_MSK & (x)) >> 4) * 10) + (DS_MONTH_DAY_MSK & (x)))
#define DS_MONTH_TO_DEC(x)      ((((DS_MONTH_TEN_MSK & (x)) >> 4) * 10) + (DS_MONTH_MSK & (x)))
#define DS_YEAR_TO_DEC(x)       ((((DS_YEAR_TEN_MSK & (x)) >> 4) * 10) + (DS_YEAR_MSK & (x)))