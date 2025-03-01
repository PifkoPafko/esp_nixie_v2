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
#include "pp_nixie_display.h"

/* Macros */
#define NIXIE_DISPLAY_TAG "NIXIE DISPLAY"

/* Declarations */
static void pp_nixie_display_generate_i2c_msg(display_state_t *nixie_state, uint8_t *i2c_msg);

/* Variables */
static i2c_master_dev_handle_t exp_dev_handle[6];

static const uint8_t FIRST_NIX_DIGIT_MASK[DIGITS_COUNT] = { NIXIE_0_0_BIT, NIXIE_0_1_BIT, NIXIE_0_2_BIT, NIXIE_0_3_BIT, NIXIE_0_4_BIT, NIXIE_0_5_BIT, NIXIE_0_6_BIT, NIXIE_0_7_BIT, NIXIE_0_8_BIT, NIXIE_0_9_BIT};
static const uint8_t FIRST_NIX_DIGIT_REG_ID[DIGITS_COUNT] = {NIXIE_0_0_REG_ID, NIXIE_0_1_REG_ID, NIXIE_0_2_REG_ID, NIXIE_0_3_REG_ID, NIXIE_0_4_REG_ID, NIXIE_0_5_REG_ID, NIXIE_0_6_REG_ID, NIXIE_0_7_REG_ID, NIXIE_0_8_REG_ID, NIXIE_0_9_REG_ID};
static const uint8_t FIRST_NIX_LEFT_COMMA_MASK = NIXIE_0_LC_BIT;
static const uint8_t FIRST_NIX_LEFT_COMMA_REG_ID = NIXIE_0_LC_REG_ID;
static const uint8_t FIRST_NIX_RIGHT_COMMA_MASK = NIXIE_0_RC_BIT;
static const uint8_t FIRST_NIX_RIGHT_COMMA_REG_ID = NIXIE_0_RC_REG_ID;

static const uint8_t SECOND_NIX_DIGIT_MASK[DIGITS_COUNT] = {NIXIE_1_0_BIT, NIXIE_1_1_BIT, NIXIE_1_2_BIT, NIXIE_1_3_BIT, NIXIE_1_4_BIT, NIXIE_1_5_BIT, NIXIE_1_6_BIT, NIXIE_1_7_BIT, NIXIE_1_8_BIT, NIXIE_1_9_BIT};
static const uint8_t SECOND_NIX_DIGIT_REG_ID[DIGITS_COUNT] = {NIXIE_1_0_REG_ID, NIXIE_1_1_REG_ID, NIXIE_1_2_REG_ID, NIXIE_1_3_REG_ID, NIXIE_1_4_REG_ID, NIXIE_1_5_REG_ID, NIXIE_1_6_REG_ID, NIXIE_1_7_REG_ID, NIXIE_1_8_REG_ID, NIXIE_1_9_REG_ID};
static const uint8_t SECOND_NIX_LEFT_COMMA_MASK = NIXIE_1_LC_BIT;
static const uint8_t SECOND_NIX_LEFT_COMMA_REG_ID = NIXIE_1_LC_REG_ID;
static const uint8_t SECOND_NIX_RIGHT_COMMA_MASK = NIXIE_1_RC_BIT;
static const uint8_t SECOND_NIX_RIGHT_COMMA_REG_ID = NIXIE_1_RC_REG_ID;

static const uint8_t THIRD_NIX_DIGIT_MASK[DIGITS_COUNT] = {NIXIE_2_0_BIT, NIXIE_2_1_BIT, NIXIE_2_2_BIT, NIXIE_2_3_BIT, NIXIE_2_4_BIT, NIXIE_2_5_BIT, NIXIE_2_6_BIT, NIXIE_2_7_BIT, NIXIE_2_8_BIT, NIXIE_2_9_BIT};
static const uint8_t THIRD_NIX_DIGIT_REG_ID[DIGITS_COUNT] = {NIXIE_2_0_REG_ID, NIXIE_2_1_REG_ID, NIXIE_2_2_REG_ID, NIXIE_2_3_REG_ID, NIXIE_2_4_REG_ID, NIXIE_2_5_REG_ID, NIXIE_2_6_REG_ID, NIXIE_2_7_REG_ID, NIXIE_2_8_REG_ID, NIXIE_2_9_REG_ID};
static const uint8_t THIRD_NIX_LEFT_COMMA_MASK = NIXIE_2_LC_BIT;
static const uint8_t THIRD_NIX_LEFT_COMMA_REG_ID = NIXIE_2_LC_REG_ID;
static const uint8_t THIRD_NIX_RIGHT_COMMA_MASK = NIXIE_2_RC_BIT;
static const uint8_t THIRD_NIX_RIGHT_COMMA_REG_ID = NIXIE_2_RC_REG_ID;

static const uint8_t EXPANDER_ADDRESS[EXPANDER_COUNT] = {SLAVE_ADDR_0, SLAVE_ADDR_1, SLAVE_ADDR_2, SLAVE_ADDR_3, SLAVE_ADDR_4, SLAVE_ADDR_5};

/* Functions */

/** @brief pp_nixie_display_init: Initializes nixie display.
 * 
 * This function sets all registers of expanders as outputs.
 *
 * @return
 */
void pp_nixie_display_init(void)
{
    ESP_LOGI(NIXIE_DISPLAY_TAG, "Initializing NIXIE Display");

    i2c_master_bus_handle_t bus_handle;
    ESP_ERROR_CHECK(i2c_master_get_bus_handle(0, &bus_handle));

    for(uint8_t i = 0; i < 6; ++i)
    {
        i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = EXPANDER_ADDRESS[i],
        .scl_speed_hz = 100000,
        };

        ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &exp_dev_handle[i]));
    }

    uint8_t conf_output_mask[EXPANDER_REG_COUNT];
    memset(conf_output_mask, 0, EXPANDER_REG_COUNT*sizeof(conf_output_mask[0]));

    for(uint8_t expander_id = 0; expander_id < EXPANDER_COUNT; expander_id++)
    {
        pp_pca_write_all_reg(exp_dev_handle[expander_id], IOC0_ADDR, conf_output_mask);
    }
}

/** @brief pp_display: Displays given nixie tubes state.
 * 
 * This function sets outputs of expanders and siplay desired digits and commas of nixie tubes.
 * 
 * @param[in]   nixie_state  (display_state_t*) Pointer to display_state_t structure.
 * 
 * @return
 */
void pp_display(display_state_t *display_state)
{
    uint8_t i2c_msg[EXPANDER_COUNT][EXPANDER_REG_COUNT];
    memset(i2c_msg, 0, EXPANDER_COUNT*EXPANDER_REG_COUNT*sizeof(i2c_msg[0][0]));
    pp_nixie_display_generate_i2c_msg(display_state, &i2c_msg[0][0]);
    
    for(uint8_t expander_id = 0; expander_id < EXPANDER_COUNT; expander_id++)
    {
        pp_pca_write_all_reg(exp_dev_handle[expander_id], OP0_ADDR, i2c_msg[expander_id]);
    }
}

/** @brief pp_nixie_display_generate_i2c_msg: Prepares I2C message to set the registers of expanders.
 * 
 * This function generates I2C messages from given nixie state. 
 *
 * @param[in]   nixie_state  (display_state_t*) Pointer to display_state_t structure.
 * @param[out]  i2c_msg  (uint8_t*) Pointer to I2C message table which should be i2c_msg[EXPANDER_COUNT][EXPANDER_REG_COUNT]. The result will be stored here.
 * 
 * @return
 */
static void pp_nixie_display_generate_i2c_msg(display_state_t *display_state, uint8_t *i2c_msg)
{
    for(uint8_t expander_id = 0; expander_id < EXPANDER_COUNT; expander_id++)
    {
        uint8_t expander_nixie_id = expander_id * 3;
        uint8_t *msg = i2c_msg + (expander_id * EXPANDER_REG_COUNT);

        if(display_state->digit_enable[expander_nixie_id])
        {
            msg[FIRST_NIX_DIGIT_REG_ID[display_state->digit[expander_nixie_id]]] |= FIRST_NIX_DIGIT_MASK[display_state->digit[expander_nixie_id]];
        }

        if(display_state->left_comma_enable[expander_nixie_id])
        {
            msg[FIRST_NIX_LEFT_COMMA_REG_ID] |= FIRST_NIX_LEFT_COMMA_MASK;
        }

        if(display_state->right_comma_enable[expander_nixie_id])
        {
            msg[FIRST_NIX_RIGHT_COMMA_REG_ID] |= FIRST_NIX_RIGHT_COMMA_MASK;
        }

        if(expander_id < 5)
        {
            if(display_state->digit_enable[expander_nixie_id + 1])
            {
                msg[SECOND_NIX_DIGIT_REG_ID[display_state->digit[expander_nixie_id + 1]]] |= SECOND_NIX_DIGIT_MASK[display_state->digit[expander_nixie_id + 1]];
            }

            if(display_state->left_comma_enable[expander_nixie_id + 1])
            {
                msg[SECOND_NIX_LEFT_COMMA_REG_ID] |= SECOND_NIX_LEFT_COMMA_MASK;
            }

            if(display_state->right_comma_enable[expander_nixie_id + 1])
            {
                msg[SECOND_NIX_RIGHT_COMMA_REG_ID] |= SECOND_NIX_RIGHT_COMMA_MASK;
            }

            if(display_state->digit_enable[expander_nixie_id + 2])
            {
                msg[THIRD_NIX_DIGIT_REG_ID[display_state->digit[expander_nixie_id + 2]]] |= THIRD_NIX_DIGIT_MASK[display_state->digit[expander_nixie_id + 2]];
            }

            if(display_state->left_comma_enable[expander_nixie_id + 2])
            {
                msg[THIRD_NIX_LEFT_COMMA_REG_ID] |= THIRD_NIX_LEFT_COMMA_MASK;
            }

            if(display_state->right_comma_enable[expander_nixie_id + 2])
            {
                msg[THIRD_NIX_RIGHT_COMMA_REG_ID] |= THIRD_NIX_RIGHT_COMMA_MASK;
            }
        }
    }
}