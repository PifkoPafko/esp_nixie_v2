/* TASKS */

TaskHandle_t display_main_h;
TaskHandle_t button_main_h;
TaskHandle_t rtc_main_h;
TaskHandle_t alarm_main_h;
TaskHandle_t wifi_main_h;

volatile device_mode_t device_mode = DEFAULT_MODE;

alarm_mode_args_t current_alarm;