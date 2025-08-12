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
#include "pp_gpio.h"

/* Declarations */
static void pp_button_isr_handler(void* arg);
static void pp_btn_timer_cb(TimerHandle_t xTimer);
static void pp_button_main(void* arg);

/* Macros */
#define GPIO_TAG "GPIO"

/* Variables */
static QueueHandle_t gpio_evt_queue;
static TimerHandle_t btn_timer_h[3];
static gpio_sm_t button_sm[3];

/* Functions */

/** @brief pp_gpio_init: Initializes and sets gpio configuration for OE line, leds and buttons.
 * 
 * @return
 */

/** @brief pp_button_isr_handler: Interruption callback for buttons (any edge)
 * 
 *  Sends message to the button main task using queue 
 * 
 *  @param[in]  arg (void*) Pointer GPIO number by which interrupt was called
 * 
 * @return
 */
static void IRAM_ATTR pp_button_isr_handler(void* arg)
{
    button_queue_msg_t msg;
    msg.type = ISR;
    msg.enable = !gpio_get_level((uint32_t) arg);;
    msg.gpio_num = (uint32_t) arg;
    
    xQueueSendFromISR(gpio_evt_queue, &msg, NULL);
}

void pp_gpio_init(void)
{
    ESP_LOGI(GPIO_TAG, "Initializing gpio");

    /* OUTPUTS INIT */
    gpio_config_t io_conf_output = {};
    io_conf_output.intr_type = GPIO_INTR_DISABLE;
    io_conf_output.mode = GPIO_MODE_OUTPUT;
    io_conf_output.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    io_conf_output.pull_down_en = 0;
    io_conf_output.pull_up_en = 0;
    ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_config(&io_conf_output));
    ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_set_level(GPIO_OUTPUT_OE, 0));
    pp_led_enable(GPIO_OUTPUT_RED, false);
    pp_led_enable(GPIO_OUTPUT_BLUE, false);
    pp_led_enable(GPIO_OUTPUT_GREEN, false);

    /* INPUTS INIT */
    gpio_config_t io_conf_input = {};
    io_conf_input.intr_type = GPIO_INTR_ANYEDGE;
    io_conf_input.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    io_conf_input.mode = GPIO_MODE_INPUT;
    io_conf_input.pull_up_en = 1;
    ESP_ERROR_CHECK(gpio_config(&io_conf_input));

    for (uint8_t i = 0; i < 3; ++i)
    {
        button_sm[i].state = IDLE;
        button_sm[i].last_enable = false;
    }

    ESP_ERROR_CHECK(gpio_install_isr_service(ESP_INTR_FLAG_EDGE));
    ESP_ERROR_CHECK(gpio_isr_handler_add(GPIO_INPUT_IO_LEFT, pp_button_isr_handler, (void*) GPIO_INPUT_IO_LEFT));
    ESP_ERROR_CHECK(gpio_isr_handler_add(GPIO_INPUT_IO_CENTER, pp_button_isr_handler, (void*) GPIO_INPUT_IO_CENTER));
    ESP_ERROR_CHECK(gpio_isr_handler_add(GPIO_INPUT_IO_RIGHT, pp_button_isr_handler, (void*) GPIO_INPUT_IO_RIGHT));

    btn_timer_h[0] = xTimerCreate(NULL, pdMS_TO_TICKS(100), pdFALSE, NULL, pp_btn_timer_cb);
    btn_timer_h[1] = xTimerCreate(NULL, pdMS_TO_TICKS(100), pdFALSE, NULL, pp_btn_timer_cb);
    btn_timer_h[2] = xTimerCreate(NULL, pdMS_TO_TICKS(100), pdFALSE, NULL, pp_btn_timer_cb);

    gpio_evt_queue = xQueueCreate(10, sizeof(button_queue_msg_t));
    button_action_queue = xQueueCreate(10, sizeof(button_action_t));

    BaseType_t res = xTaskCreate(pp_button_main, "BUTTON_MAIN", 3072, NULL, 1, &button_main_h);
    if(res != pdPASS)
    {
        ESP_ERROR_CHECK(ESP_FAIL);
    }
}

/** @brief pp_led_enable: Enables or disables chosen LED
 *
 * @param[in]   led (uint8_t) LED to enable/disable
 * @param[in]   enable (uint8_t) true if enable, false if disable
 * 
 * @return
 */
void pp_led_enable(uint8_t led, bool enable)
{
    if(led == GPIO_OUTPUT_RED || led == GPIO_OUTPUT_BLUE || led == GPIO_OUTPUT_GREEN )
    {
        ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_set_level(led, enable ? 1 : 0));
    }
}

/** @brief pp_btn_timer_cb: Timer callback for buttons
 * 
 *  Sends message to the button main task using queue
 *  
 *  @param[in]  xTimer (TimerHandle_t) Handle to the Timer which called the callback
 * 
 * @return
 */
static void pp_btn_timer_cb(TimerHandle_t xTimer)
{
    button_queue_msg_t msg;
    msg.type = TIMER;
    
    xQueueSendFromISR(gpio_evt_queue, &msg, NULL);
}

/** @brief pp_button_main: Buttons Main Task
 * 
 *  This task detects 3 different button actions:   Short press, Long Press, Holding Short
 *  It woks using messages form interruptions and timers implement functionality and prevert contact vibration
 * 
 *  Short Press - Holding button less than 3 seconds.
 *  Long Press - Holding button at least 3 seconds.
 *  Holding Short - Holding button at least 100 ms.
 *  
 *  @param[in]  arg (void*) Reserved
 * 
 * @return
 */
static void pp_button_main(void* arg)
{
    uint8_t button_id;
    button_queue_msg_t msg;
    button_action_t action_handler;

    while(true) 
    {
        if(xQueueReceive(gpio_evt_queue, &msg, portMAX_DELAY) != pdTRUE) continue;

        button_id = msg.gpio_num - GPIO_INPUT_IO_LEFT;
        if(button_id > 2) continue;
                
        button_sm[button_id].last_enable = msg.enable;
        bool action_happened = false;

        switch(button_sm[button_id].state)
        {
            case IDLE:
            {
                if(msg.type == ISR && button_sm[button_id].last_enable)
                {
                    if(xTimerChangePeriod(btn_timer_h[button_id], pdMS_TO_TICKS(100), 1))
                    {
                        ESP_LOGV(GPIO_TAG, "BTN %d IDLE -> WAIT_ENABLE_CONTACT_VIBRATION", button_id);
                        button_sm[button_id].state = WAIT_ENABLE_CONTACT_VIBRATION;  
                    }
                }
                break;
            }

            case WAIT_ENABLE_CONTACT_VIBRATION:
            {
                if(msg.type == TIMER)
                {
                    if(button_sm[button_id].last_enable)
                    {   
                        if (xTimerChangePeriod(btn_timer_h[button_id], pdMS_TO_TICKS(3000), 1))
                        {
                            action_happened = true;
                            action_handler.button = button_id;
                            action_handler.action = HOLDING_SHORT;

                            ESP_LOGV(GPIO_TAG, "BTN %d WAIT_ENABLE_CONTACT_VIBRATION -> WAIT_LONG_PRESS", button_id);
                            button_sm[button_id].state = WAIT_LONG_PRESS;  
                        }
                        else
                        {
                            ESP_LOGV(GPIO_TAG, "BTN %d WAIT_ENABLE_CONTACT_VIBRATION -> IDLE", button_id);
                            button_sm[button_id].state = IDLE;
                        }                 
                    }
                    else
                    {
                        action_happened = true;
                        action_handler.button = button_id;
                        action_handler.action = SHORT_PRESS;

                        if (xTimerChangePeriod(btn_timer_h[button_id], pdMS_TO_TICKS(100), 1))
                        {
                            ESP_LOGV(GPIO_TAG, "BTN %d WAIT_ENABLE_CONTACT_VIBRATION -> WAIT_DISABLE_CONTACT_VIBRATION", button_id);
                            button_sm[button_id].state = WAIT_DISABLE_CONTACT_VIBRATION;
                        }
                        else
                        {
                            ESP_LOGV(GPIO_TAG, "BTN %d WAIT_ENABLE_CONTACT_VIBRATION -> IDLE", button_id);
                            button_sm[button_id].state = IDLE;
                        }
                    }
                }
                
                break;
            }

            case WAIT_LONG_PRESS:
            {
                if(msg.type == TIMER)
                {
                    if (button_sm[button_id].last_enable)
                    {
                        action_happened = true;
                        action_handler.button = button_id;
                        action_handler.action = LONG_PRESS;

                        ESP_LOGV(GPIO_TAG, "BTN %d WAIT_LONG_PRESS -> WAIT_DISABLE", button_id);
                        button_sm[button_id].state = WAIT_DISABLE;
                    }
                    else
                    {
                        action_happened = true;
                        action_handler.button = button_id;
                        action_handler.action = SHORT_PRESS;

                        if (xTimerChangePeriod(btn_timer_h[button_id], pdMS_TO_TICKS(100), 1))
                        {
                            ESP_LOGV(GPIO_TAG, "BTN %d WAIT_LONG_PRESS -> WAIT_DISABLE_CONTACT_VIBRATION", button_id);
                            button_sm[button_id].state = WAIT_DISABLE_CONTACT_VIBRATION;
                        }
                        else
                        {
                            ESP_LOGV(GPIO_TAG, "BTN %d WAIT_LONG_PRESS -> IDLE", button_id);
                            button_sm[button_id].state = IDLE;
                        }
                    }
                }
                else /* msg.type == ISR */
                {
                    if(!button_sm[button_id].last_enable)
                    {
                        action_happened = true;
                        action_handler.button = button_id;
                        action_handler.action = SHORT_PRESS;

                        if (xTimerChangePeriod(btn_timer_h[button_id], pdMS_TO_TICKS(100), 1))
                        {
                            ESP_LOGV(GPIO_TAG, "BTN %d WAIT_LONG_PRESS -> WAIT_DISABLE_CONTACT_VIBRATION", button_id);
                            button_sm[button_id].state = WAIT_DISABLE_CONTACT_VIBRATION;
                        }
                        else
                        {
                            ESP_LOGV(GPIO_TAG, "BTN %d WAIT_LONG_PRESS -> IDLE", button_id);
                            button_sm[button_id].state = IDLE;
                        }
                    }
                }
                
                break;
            }

            case WAIT_DISABLE:
            {
                if (!button_sm[button_id].last_enable)
                {
                    if (xTimerChangePeriod(btn_timer_h[button_id], pdMS_TO_TICKS(100), 1))
                    {
                        ESP_LOGV(GPIO_TAG, "BTN %d WAIT_DISABLE -> WAIT_DISABLE_CONTACT_VIBRATION", button_id);
                        button_sm[button_id].state = WAIT_DISABLE_CONTACT_VIBRATION;
                    }
                    else
                    {
                        ESP_LOGV(GPIO_TAG, "BTN %d WAIT_DISABLE -> IDLE", button_id);
                        button_sm[button_id].state = IDLE;
                    }
                }

                break;
            }

            case WAIT_DISABLE_CONTACT_VIBRATION:
            {
                if(msg.type == TIMER)
                {
                    ESP_LOGV(GPIO_TAG, "BTN %d WAIT_DISABLE_CONTACT_VIBRATION -> IDLE", button_id);
                    button_sm[button_id].state = IDLE; 
                }
                
                break;
            }

            default:
            {
                break;
            }
        }

        if (action_happened)
        {
            xQueueSend(button_action_queue, &action_handler, 1);
        }
    }
}