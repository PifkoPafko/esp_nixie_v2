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
#include "pp_led.h"

/* Macros */
#define LED_TAG "LED"

/* Variables */
static ledc_channel_config_t ledc_channel_info[LEDC_SET_NUM][LEDC_CH_NUM];

/* Functions */

/** @brief pp_led_main: LED Task main function
 *
 * @param[in]   arg (void*) Reserved
 * @return
 */
static void pp_led_main(void* arg)
{
    while (true)
    {
        led_update_t led_params;
        if(xQueueReceive(led_update_queue, &led_params, portMAX_DELAY) != pdTRUE) continue;

        ESP_LOGI(LED_TAG, "New LED settings: CHANNEL = %u, RED = 0x%04X, GREEN = 0x%04X, BLUE = 0x%04X", (unsigned int)led_params.channel, (unsigned int)led_params.red, (unsigned int)led_params.green, (unsigned int)led_params.blue);

        memcpy(&current_led[led_params.channel], &led_params, sizeof(led_params));

        ledc_set_duty(ledc_channel_info[led_params.channel][LED_RED_INDEX].speed_mode,      ledc_channel_info[led_params.channel][LED_RED_INDEX].channel,   led_params.red);
        ledc_set_duty(ledc_channel_info[led_params.channel][LED_GREEN_INDEX].speed_mode,    ledc_channel_info[led_params.channel][LED_GREEN_INDEX].channel, led_params.green);
        ledc_set_duty(ledc_channel_info[led_params.channel][LED_BLUE_INDEX].speed_mode,     ledc_channel_info[led_params.channel][LED_BLUE_INDEX].channel,  led_params.blue);

        ledc_update_duty(ledc_channel_info[led_params.channel][LED_RED_INDEX].speed_mode,   ledc_channel_info[led_params.channel][LED_RED_INDEX].channel);
        ledc_update_duty(ledc_channel_info[led_params.channel][LED_GREEN_INDEX].speed_mode, ledc_channel_info[led_params.channel][LED_GREEN_INDEX].channel);
        ledc_update_duty(ledc_channel_info[led_params.channel][LED_BLUE_INDEX].speed_mode,  ledc_channel_info[led_params.channel][LED_BLUE_INDEX].channel);
    }
}

/** @brief pp_led_init: Initializes LED driver
 *
 * @return
 */
void pp_led_init(void)
{

    /*
     * Prepare individual configuration
     * for each channel of LED Controller
     * by selecting:
     * - controller's channel number
     * - output duty cycle, set initially to 0
     * - GPIO number where LED is connected to
     * - speed mode, either high or low
     * - timer servicing selected channel
     *   Note: if different channels use one timer,
     *         then frequency and bit_num of these channels
     *         will be the same
     */

    /* First LED strip */
    ledc_channel_info[0][LED_RED_INDEX].channel    = LEDC_LS_CH0_RED_CHANNEL;
    ledc_channel_info[0][LED_RED_INDEX].duty       = 0;
    ledc_channel_info[0][LED_RED_INDEX].gpio_num   = LEDC_LS_CH0_RED_GPIO;
    ledc_channel_info[0][LED_RED_INDEX].speed_mode = LEDC_LS_MODE;
    ledc_channel_info[0][LED_RED_INDEX].intr_type  = LEDC_INTR_DISABLE;
    ledc_channel_info[0][LED_RED_INDEX].hpoint     = 0;
    ledc_channel_info[0][LED_RED_INDEX].timer_sel  = LEDC_LS_TIMER_CH0;
    ledc_channel_info[0][LED_RED_INDEX].sleep_mode = LEDC_SLEEP_MODE_NO_ALIVE_NO_PD;
    ledc_channel_info[0][LED_RED_INDEX].flags.output_invert = 0;

    ledc_channel_info[0][LED_GREEN_INDEX].channel    = LEDC_LS_CH0_GREEN_CHANNEL;
    ledc_channel_info[0][LED_GREEN_INDEX].duty       = 0;
    ledc_channel_info[0][LED_GREEN_INDEX].gpio_num   = LEDC_LS_CH0_GREEN_GPIO;
    ledc_channel_info[0][LED_GREEN_INDEX].speed_mode = LEDC_LS_MODE;
    ledc_channel_info[0][LED_GREEN_INDEX].intr_type  = LEDC_INTR_DISABLE;
    ledc_channel_info[0][LED_GREEN_INDEX].hpoint     = 0;
    ledc_channel_info[0][LED_GREEN_INDEX].timer_sel  = LEDC_LS_TIMER_CH0;
    ledc_channel_info[0][LED_GREEN_INDEX].sleep_mode = LEDC_SLEEP_MODE_NO_ALIVE_NO_PD;
    ledc_channel_info[0][LED_GREEN_INDEX].flags.output_invert = 0;

    ledc_channel_info[0][LED_BLUE_INDEX].channel    = LEDC_LS_CH0_BLUE_CHANNEL;
    ledc_channel_info[0][LED_BLUE_INDEX].duty       = 0;
    ledc_channel_info[0][LED_BLUE_INDEX].gpio_num   = LEDC_LS_CH0_BLUE_GPIO;
    ledc_channel_info[0][LED_BLUE_INDEX].speed_mode = LEDC_LS_MODE;
    ledc_channel_info[0][LED_BLUE_INDEX].intr_type  = LEDC_INTR_DISABLE;
    ledc_channel_info[0][LED_BLUE_INDEX].hpoint     = 0;
    ledc_channel_info[0][LED_BLUE_INDEX].timer_sel  = LEDC_LS_TIMER_CH0;
    ledc_channel_info[0][LED_BLUE_INDEX].sleep_mode = LEDC_SLEEP_MODE_NO_ALIVE_NO_PD;
    ledc_channel_info[0][LED_BLUE_INDEX].flags.output_invert = 0;

    /* Second LED strip */
    ledc_channel_info[1][LED_RED_INDEX].channel    = LEDC_LS_CH1_RED_CHANNEL;
    ledc_channel_info[1][LED_RED_INDEX].duty       = 0;
    ledc_channel_info[1][LED_RED_INDEX].gpio_num   = LEDC_LS_CH1_RED_GPIO;
    ledc_channel_info[1][LED_RED_INDEX].speed_mode = LEDC_LS_MODE;
    ledc_channel_info[1][LED_RED_INDEX].intr_type  = LEDC_INTR_DISABLE;
    ledc_channel_info[1][LED_RED_INDEX].hpoint     = 0;
    ledc_channel_info[1][LED_RED_INDEX].timer_sel  = LEDC_LS_TIMER_CH1;
    ledc_channel_info[1][LED_RED_INDEX].sleep_mode = LEDC_SLEEP_MODE_NO_ALIVE_NO_PD;
    ledc_channel_info[1][LED_RED_INDEX].flags.output_invert = 0;

    ledc_channel_info[1][LED_GREEN_INDEX].channel    = LEDC_LS_CH1_GREEN_CHANNEL;
    ledc_channel_info[1][LED_GREEN_INDEX].duty       = 0;
    ledc_channel_info[1][LED_GREEN_INDEX].gpio_num   = LEDC_LS_CH1_GREEN_GPIO;
    ledc_channel_info[1][LED_GREEN_INDEX].speed_mode = LEDC_LS_MODE;
    ledc_channel_info[1][LED_GREEN_INDEX].intr_type  = LEDC_INTR_DISABLE;
    ledc_channel_info[1][LED_GREEN_INDEX].hpoint     = 0;
    ledc_channel_info[1][LED_GREEN_INDEX].timer_sel  = LEDC_LS_TIMER_CH1;
    ledc_channel_info[1][LED_GREEN_INDEX].sleep_mode = LEDC_SLEEP_MODE_NO_ALIVE_NO_PD;
    ledc_channel_info[1][LED_GREEN_INDEX].flags.output_invert = 0;

    ledc_channel_info[1][LED_BLUE_INDEX].channel    = LEDC_LS_CH1_BLUE_CHANNEL;
    ledc_channel_info[1][LED_BLUE_INDEX].duty       = 0;
    ledc_channel_info[1][LED_BLUE_INDEX].gpio_num   = LEDC_LS_CH1_BLUE_GPIO;
    ledc_channel_info[1][LED_BLUE_INDEX].speed_mode = LEDC_LS_MODE;
    ledc_channel_info[1][LED_BLUE_INDEX].intr_type  = LEDC_INTR_DISABLE;
    ledc_channel_info[1][LED_BLUE_INDEX].hpoint     = 0;
    ledc_channel_info[1][LED_BLUE_INDEX].timer_sel  = LEDC_LS_TIMER_CH1;
    ledc_channel_info[1][LED_BLUE_INDEX].sleep_mode = LEDC_SLEEP_MODE_NO_ALIVE_NO_PD;
    ledc_channel_info[1][LED_BLUE_INDEX].flags.output_invert = 0;

    /*
     * Prepare and set configuration of timers
     * that will be used by LED Controller
     */
    ledc_timer_config_t ledc_timer_ch0 = {
        .duty_resolution    = LEDC_TIMER_12_BIT,    // resolution of PWM duty
        .freq_hz            = 5000,                 // frequency of PWM signal
        .speed_mode         = LEDC_LS_MODE,         // timer mode
        .timer_num          = LEDC_LS_TIMER_CH0,    // timer index
        .clk_cfg            = LEDC_AUTO_CLK,        // Auto select the source clock
    };

    ledc_timer_config_t ledc_timer_ch1 = {
        .duty_resolution    = LEDC_TIMER_12_BIT,    // resolution of PWM duty
        .freq_hz            = 5000,                 // frequency of PWM signal
        .speed_mode         = LEDC_LS_MODE,         // timer mode
        .timer_num          = LEDC_LS_TIMER_CH1,    // timer index
        .clk_cfg            = LEDC_AUTO_CLK,        // Auto select the source clock
    };

    // Set configuration of timer0 for high speed channels
    ledc_timer_config(&ledc_timer_ch0);
    ledc_timer_config(&ledc_timer_ch1);
    
    // Set LED Controller with previously prepared configuration
    for (uint8_t set = 0; set < 2; set++)
    {
        for (uint8_t ch = 0; ch < LEDC_CH_NUM; ch++)
        {
            ledc_channel_config(&ledc_channel_info[set][ch]);
        }
    }

    led_update_queue = xQueueCreate(3, sizeof(led_update_t));
    if(led_update_queue == 0)
    {
        ESP_LOGE(LED_TAG, "led_update_queue not created");
        ESP_ERROR_CHECK(ESP_FAIL);
    }

    BaseType_t res = xTaskCreate(pp_led_main, "LED", 3072, NULL, 2, &led_main_h);
    if(res != pdPASS)
    {
        ESP_ERROR_CHECK(ESP_FAIL);
    }
}