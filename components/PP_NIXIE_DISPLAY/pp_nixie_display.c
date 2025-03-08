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

/* Declarations */
static void pp_nixie_display_generate_i2c_msg(nixie_tube_state_t *nixie_state, uint8_t *i2c_msg);

/* Variables */
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

    uint8_t conf_output_mask[EXPANDER_REG_COUNT];
    memset(conf_output_mask, 0, EXPANDER_REG_COUNT*sizeof(conf_output_mask[0]));

    for(uint8_t expander_id = 0; expander_id < EXPANDER_COUNT; expander_id++)
    {
        pp_pca_write_all_reg(EXPANDER_ADDRESS[expander_id], IOC0_ADDR, conf_output_mask);
    }
}

/** @brief pp_display: Displays given nixie tubes state.
 * 
 * This function sets outputs of expanders and siplay desired digits and commas of nixie tubes.
 *
 * @param[in]   nixie_state  (nixie_tube_state_t*) Pointer to nixie_state structure.
 * 
 * @return
 */
void pp_display(nixie_tube_state_t *nixie_state)
{
    uint8_t i2c_msg[EXPANDER_COUNT][EXPANDER_REG_COUNT];
    memset(i2c_msg, 0, EXPANDER_COUNT*EXPANDER_REG_COUNT*sizeof(i2c_msg[0]));
    pp_nixie_display_generate_i2c_msg(nixie_state, &i2c_msg[0][0]);
    
    for(uint8_t expander_id = 0; expander_id < EXPANDER_COUNT; expander_id++)
    {
        pp_pca_write_all_reg(EXPANDER_ADDRESS[expander_id], OP0_ADDR, i2c_msg[expander_id]);
    }
}

/** @brief pp_nixie_display_generate_i2c_msg: Prepares I2C message to set the registers of expanders.
 * 
 * This function generates I2C messages from given nixie state. 
 *
 * @param[in]   nixie_state  (nixie_tube_state_t*) Pointer to nixie_state structure.
 * @param[out]  i2c_msg  (uint8_t*) Pointer to I2C message table which should be i2c_msg[EXPANDER_COUNT][EXPANDER_REG_COUNT]. The result will be stored here.
 * 
 * @return
 */
static void pp_nixie_display_generate_i2c_msg(nixie_tube_state_t *nixie_state, uint8_t *i2c_msg)
{
    for(uint8_t expander_id = 0; expander_id < EXPANDER_COUNT; expander_id++)
    {
        uint8_t expander_nixie_id = expander_id * 3;
        uint8_t *msg = i2c_msg + (expander_id * EXPANDER_REG_COUNT);

        if(nixie_state->digit_enable[expander_nixie_id])
        {
            msg[FIRST_NIX_DIGIT_REG_ID[nixie_state->digit[expander_nixie_id]]] |= FIRST_NIX_DIGIT_MASK[nixie_state->digit[expander_nixie_id]];
        }

        if(nixie_state->left_comma_enable[expander_nixie_id])
        {
            msg[FIRST_NIX_LEFT_COMMA_REG_ID] |= FIRST_NIX_LEFT_COMMA_MASK;
        }

        if(nixie_state->right_comma_enable[expander_nixie_id])
        {
            msg[FIRST_NIX_RIGHT_COMMA_REG_ID] |= FIRST_NIX_RIGHT_COMMA_MASK;
        }

        if(expander_id < 5)
        {
            if(nixie_state->digit_enable[expander_nixie_id + 1])
            {
                msg[SECOND_NIX_DIGIT_REG_ID[nixie_state->digit[expander_nixie_id + 1]]] |= SECOND_NIX_DIGIT_MASK[nixie_state->digit[expander_nixie_id + 1]];
            }

            if(nixie_state->left_comma_enable[expander_nixie_id + 1])
            {
                msg[SECOND_NIX_LEFT_COMMA_REG_ID] |= SECOND_NIX_LEFT_COMMA_MASK;
            }

            if(nixie_state->right_comma_enable[expander_nixie_id + 1])
            {
                msg[SECOND_NIX_RIGHT_COMMA_REG_ID] |= SECOND_NIX_RIGHT_COMMA_MASK;
            }

            if(nixie_state->digit_enable[expander_nixie_id + 2])
            {
                msg[THIRD_NIX_DIGIT_REG_ID[nixie_state->digit[expander_nixie_id + 2]]] |= THIRD_NIX_DIGIT_MASK[nixie_state->digit[expander_nixie_id + 2]];
            }

            if(nixie_state->left_comma_enable[expander_nixie_id + 2])
            {
                msg[THIRD_NIX_LEFT_COMMA_REG_ID] |= THIRD_NIX_LEFT_COMMA_MASK;
            }

            if(nixie_state->right_comma_enable[expander_nixie_id + 2])
            {
                msg[THIRD_NIX_RIGHT_COMMA_REG_ID] |= THIRD_NIX_RIGHT_COMMA_MASK;
            }
        }
    }
}