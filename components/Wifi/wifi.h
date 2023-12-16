#include "esp_log.h"
#include "esp_event.h"
#include "esp_wifi.h"

#define DEFAULT_NTP_SERVER			"ntp.certum.pl"
#define CENTRAL_EUROPEAN_TIME_ZONE	"CET-1CEST,M3.5.0/2,M10.5.0/3"  // for Poland

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
