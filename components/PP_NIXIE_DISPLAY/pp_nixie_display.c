#include <string.h>

#include "mk_i2c.h"
#include "pp_pca9698.h"
#include "pp_nixie_display.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "driver/gpio.h"
#include "driver/gptimer.h"

#include <sys/time.h>
#include <time.h>

static const char *TAG = "NIXIE DISPLAY";
static const uint8_t FIRST_NIX_DIGIT_MASK[10] = { NIXIE_0_0_BIT, NIXIE_0_1_BIT, NIXIE_0_2_BIT, NIXIE_0_3_BIT, NIXIE_0_4_BIT, NIXIE_0_5_BIT, NIXIE_0_6_BIT, NIXIE_0_7_BIT, NIXIE_0_8_BIT, NIXIE_0_9_BIT };
static const uint8_t FIRST_NIX_DIGIT_REG_ID[10] = { NIXIE_0_0_REG_ID, NIXIE_0_1_REG_ID, NIXIE_0_2_REG_ID, NIXIE_0_3_REG_ID, NIXIE_0_4_REG_ID, NIXIE_0_5_REG_ID, NIXIE_0_6_REG_ID, NIXIE_0_7_REG_ID, NIXIE_0_8_REG_ID, NIXIE_0_9_REG_ID };
static const uint8_t FIRST_NIX_LEFT_COMMA_MASK = NIXIE_0_LC_BIT;
static const uint8_t FIRST_NIX_LEFT_COMMA_REG_ID = NIXIE_0_LC_REG_ID;
static const uint8_t FIRST_NIX_RIGHT_COMMA_MASK = NIXIE_0_RC_BIT;
static const uint8_t FIRST_NIX_RIGHT_COMMA_REG_ID = NIXIE_0_RC_REG_ID;

static const uint8_t SECOND_NIX_DIGIT_MASK[10] = { NIXIE_1_0_BIT, NIXIE_1_1_BIT, NIXIE_1_2_BIT, NIXIE_1_3_BIT, NIXIE_1_4_BIT, NIXIE_1_5_BIT, NIXIE_1_6_BIT, NIXIE_1_7_BIT, NIXIE_1_8_BIT, NIXIE_1_9_BIT };
static const uint8_t SECOND_NIX_DIGIT_REG_ID[10] = { NIXIE_1_0_REG_ID, NIXIE_1_1_REG_ID, NIXIE_1_2_REG_ID, NIXIE_1_3_REG_ID, NIXIE_1_4_REG_ID, NIXIE_1_5_REG_ID, NIXIE_1_6_REG_ID, NIXIE_1_7_REG_ID, NIXIE_1_8_REG_ID, NIXIE_1_9_REG_ID };
static const uint8_t SECOND_NIX_LEFT_COMMA_MASK = NIXIE_1_LC_BIT;
static const uint8_t SECOND_NIX_LEFT_COMMA_REG_ID = NIXIE_1_LC_REG_ID;
static const uint8_t SECOND_NIX_RIGHT_COMMA_MASK = NIXIE_1_RC_BIT;
static const uint8_t SECOND_NIX_RIGHT_COMMA_REG_ID = NIXIE_1_RC_REG_ID;

static const uint8_t THIRD_NIX_DIGIT_MASK[10] = { NIXIE_2_0_BIT, NIXIE_2_1_BIT, NIXIE_2_2_BIT, NIXIE_2_3_BIT, NIXIE_2_4_BIT, NIXIE_2_5_BIT, NIXIE_2_6_BIT, NIXIE_2_7_BIT, NIXIE_2_8_BIT, NIXIE_2_9_BIT };
static const uint8_t THIRD_NIX_DIGIT_REG_ID[10] = { NIXIE_2_0_REG_ID, NIXIE_2_1_REG_ID, NIXIE_2_2_REG_ID, NIXIE_2_3_REG_ID, NIXIE_2_4_REG_ID, NIXIE_2_5_REG_ID, NIXIE_2_6_REG_ID, NIXIE_2_7_REG_ID, NIXIE_2_8_REG_ID, NIXIE_2_9_REG_ID };
static const uint8_t THIRD_NIX_LEFT_COMMA_MASK = NIXIE_2_LC_BIT;
static const uint8_t THIRD_NIX_LEFT_COMMA_REG_ID = NIXIE_2_LC_REG_ID;
static const uint8_t THIRD_NIX_RIGHT_COMMA_MASK = NIXIE_2_RC_BIT;
static const uint8_t THIRD_NIX_RIGHT_COMMA_REG_ID = NIXIE_2_RC_REG_ID;

static const uint8_t EXPANDER_ADDRESS[6] = { SLAVE_ADDR_0, SLAVE_ADDR_1, SLAVE_ADDR_2, SLAVE_ADDR_3, SLAVE_ADDR_4, SLAVE_ADDR_5 };

nixie_tube_state_t nixie_state[16];
uint8_t prev_i2c_msg[5];
static bool write_display_flag = false;

static void set_nixie_state()
{
    time_t now;
    struct tm timeinfo;

    time(&now);
    localtime_r(&now, &timeinfo);

    for (uint8_t i=0; i<16; i++)
    {
        nixie_state[i].left_comma_enable = false;
        nixie_state[i].right_comma_enable = false;
    }

    for (uint8_t i=0; i<6; i++)
    {
        nixie_state[i].digit_enable = true;
    }

    for (uint8_t i=7; i<13; i++)
    {
        nixie_state[i].digit_enable = true;
    }

    nixie_state[0].digit = timeinfo.tm_hour / 10;
    nixie_state[1].digit = timeinfo.tm_hour % 10;
    nixie_state[1].right_comma_enable = true;
    nixie_state[2].digit = timeinfo.tm_min / 10;
    nixie_state[3].digit = timeinfo.tm_min % 10;
    nixie_state[3].right_comma_enable = true;
    nixie_state[4].digit = timeinfo.tm_sec / 10;
    nixie_state[5].digit = timeinfo.tm_sec % 10;

    nixie_state[7].digit = timeinfo.tm_mday / 10;
    nixie_state[8].digit = timeinfo.tm_mday % 10;
    nixie_state[8].right_comma_enable = true;
    nixie_state[9].digit = (timeinfo.tm_mon + 1) / 10;
    nixie_state[10].digit = (timeinfo.tm_mon + 1) % 10;
    nixie_state[10].right_comma_enable = true;
    nixie_state[11].digit = (timeinfo.tm_year - 100) / 10;
    nixie_state[12].digit = (timeinfo.tm_year - 100) % 10;
}

static bool IRAM_ATTR pp_display_routine_timer_cb(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_data)
{
    write_display_flag = true;
    return false;
}

void pp_nixie_display_main(void* arg)
{
    uint8_t i2c_msg[5];

    while(true)
    {
        if (write_display_flag)
        {
            write_display_flag = false;

            for ( uint8_t expander = 0; expander < 6; expander++ )
            {
                memset(i2c_msg, 0, 5);
                set_nixie_state();

                if (pp_nixie_display_generate_i2c_msg(expander, i2c_msg))
                {
                    pca_write_all_reg(I2C_MASTER_NUM, EXPANDER_ADDRESS[expander], OP0_ADDR, i2c_msg);
                }
            }
        }
        
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

esp_err_t pp_nixie_diplay_init()
{
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);
    gpio_set_level(GPIO_OUTPUT_OE, 0);

    uint8_t conf_output_mask[5];
    memset(conf_output_mask, 0, 5);
    ESP_ERROR_CHECK(pca_write_all_reg(I2C_MASTER_NUM, SLAVE_ADDR_0, IOC0_ADDR, conf_output_mask));
    ESP_ERROR_CHECK(pca_write_all_reg(I2C_MASTER_NUM, SLAVE_ADDR_1, IOC0_ADDR, conf_output_mask));
    ESP_ERROR_CHECK(pca_write_all_reg(I2C_MASTER_NUM, SLAVE_ADDR_2, IOC0_ADDR, conf_output_mask));
    ESP_ERROR_CHECK(pca_write_all_reg(I2C_MASTER_NUM, SLAVE_ADDR_3, IOC0_ADDR, conf_output_mask));
    ESP_ERROR_CHECK(pca_write_all_reg(I2C_MASTER_NUM, SLAVE_ADDR_4, IOC0_ADDR, conf_output_mask));
    ESP_ERROR_CHECK(pca_write_all_reg(I2C_MASTER_NUM, SLAVE_ADDR_5, IOC0_ADDR, conf_output_mask));

    BaseType_t res = xTaskCreate(pp_nixie_display_main, "NIXIE DISPLAY", 4096, NULL, 1, NULL);
    if(res != pdPASS)
    {
        ESP_LOGE(TAG, "Creating display task failed");
        return res;
    }

    gptimer_handle_t gptimer = NULL;
    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1000000, // 1000000Hz, 1 tick=1us
    };

    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &gptimer));

     gptimer_event_callbacks_t cbs = {
        .on_alarm = pp_display_routine_timer_cb,
    };

    ESP_ERROR_CHECK(gptimer_register_event_callbacks(gptimer, &cbs, NULL));

    ESP_LOGI(TAG, "Enable display routine timer");
    ESP_ERROR_CHECK(gptimer_enable(gptimer));

     gptimer_alarm_config_t alarm_config = {
        .alarm_count = 1000000, // period = 1s
        .reload_count = 0,
        .flags.auto_reload_on_alarm = true
     };

    ESP_ERROR_CHECK(gptimer_set_alarm_action(gptimer, &alarm_config));
    ESP_ERROR_CHECK(gptimer_start(gptimer));

    return ESP_OK;
}

bool pp_nixie_display_generate_i2c_msg(uint8_t expander_id, uint8_t *msg)
{
    uint8_t expander_nixie_id = expander_id * 3;

    if (nixie_state[expander_nixie_id].digit_enable)
    {
        msg[FIRST_NIX_DIGIT_REG_ID[nixie_state[expander_nixie_id].digit]] |= FIRST_NIX_DIGIT_MASK[nixie_state[expander_nixie_id].digit];
    }

    if (nixie_state[expander_nixie_id].left_comma_enable)
    {
        msg[FIRST_NIX_LEFT_COMMA_REG_ID] |= FIRST_NIX_LEFT_COMMA_MASK;
    }

    if (nixie_state[expander_nixie_id].right_comma_enable)
    {
        msg[FIRST_NIX_RIGHT_COMMA_REG_ID] |= FIRST_NIX_RIGHT_COMMA_MASK;
    }

    if (expander_id < 5)
    {
        if (nixie_state[expander_nixie_id + 1].digit_enable)
        {
            msg[SECOND_NIX_DIGIT_REG_ID[nixie_state[expander_nixie_id + 1].digit]] |= SECOND_NIX_DIGIT_MASK[nixie_state[expander_nixie_id + 1].digit];
        }

        if (nixie_state[expander_nixie_id + 1].left_comma_enable)
        {
            msg[SECOND_NIX_LEFT_COMMA_REG_ID] |= SECOND_NIX_LEFT_COMMA_MASK;
        }

        if (nixie_state[expander_nixie_id + 1].right_comma_enable)
        {
            msg[SECOND_NIX_RIGHT_COMMA_REG_ID] |= SECOND_NIX_RIGHT_COMMA_MASK;
        }


        if (nixie_state[expander_nixie_id + 2].digit_enable)
        {
            msg[THIRD_NIX_DIGIT_REG_ID[nixie_state[expander_nixie_id + 2].digit]] |= THIRD_NIX_DIGIT_MASK[nixie_state[expander_nixie_id + 2].digit];
        }

        if (nixie_state[expander_nixie_id + 2].left_comma_enable)
        {
            msg[THIRD_NIX_LEFT_COMMA_REG_ID] |= THIRD_NIX_LEFT_COMMA_MASK;
        }

        if (nixie_state[expander_nixie_id + 2].right_comma_enable)
        {
            msg[THIRD_NIX_RIGHT_COMMA_REG_ID] |= THIRD_NIX_RIGHT_COMMA_MASK;
        }
    }

    bool result = false;

    for (uint8_t i=0; i<5; i++)
    {
        if (prev_i2c_msg[i] != msg[i])
        {
            result = true;
            break;
        }
    }

    memcpy(prev_i2c_msg, msg, 5);

    return result;
}