#include "esp_log.h"
#include "esp_event.h"
#include "esp_wifi.h"


#define DEFAULT_NTP_SERVER_0			"0.pl.pool.ntp.org"
#define DEFAULT_NTP_SERVER_1			"1.pl.pool.ntp.org"
#define DEFAULT_NTP_SERVER_2			"2.pl.pool.ntp.org"
#define DEFAULT_NTP_SERVER_3			"3.pl.pool.ntp.org"
#define DEFAULT_NTP_SERVER_4			"ntp1.tp.pl"
#define DEFAULT_NTP_SERVER_5			"ntp.certum.pl"

typedef struct{
    uint8_t my_ssid_len;
    uint8_t my_password_len;
    wifi_config_t wifi_config;
} my_wifi_t;

esp_err_t start_search_task();
void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
esp_err_t connect_wifi(const uint8_t *ssid, const uint8_t ssid_len, const uint8_t *password, const uint8_t pass_len);
void wifi_sta_init();
bool get_wifi_connect_status();
my_wifi_t* get_current_wifi();
