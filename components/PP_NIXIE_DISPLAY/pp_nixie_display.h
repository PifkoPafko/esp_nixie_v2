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

#ifndef __NIXIE_DISPLAY_H__
#define __NIXIE_DISPLAY_H__

/* Headers */
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "esp_log.h"

#include "pp_global.h"
#include "pp_pca9698.h"

/* Macros */
#define TUBES_COUNT 16
#define EXPANDER_COUNT 6
#define EXPANDER_REG_COUNT 5
#define DIGITS_COUNT 10
#define TUBES_PER_EXPANDER 3

#define NIXIE_FIRST_ID  0
#define NIXIE_SECOND_ID 1
#define NIXIE_THIRD_ID  2

#define NIXIE_0_0_BIT   (1<<3)
#define NIXIE_0_1_BIT   (1<<6)
#define NIXIE_0_2_BIT   (1<<7)
#define NIXIE_0_3_BIT   (1<<0)
#define NIXIE_0_4_BIT   (1<<1)
#define NIXIE_0_5_BIT   (1<<2)
#define NIXIE_0_6_BIT   (1<<3)
#define NIXIE_0_7_BIT   (1<<4)
#define NIXIE_0_8_BIT   (1<<5)
#define NIXIE_0_9_BIT   (1<<2)
#define NIXIE_0_LC_BIT  (1<<5)
#define NIXIE_0_RC_BIT  (1<<4)

#define NIXIE_1_0_BIT   (1<<5)
#define NIXIE_1_1_BIT   (1<<0)
#define NIXIE_1_2_BIT   (1<<1)
#define NIXIE_1_3_BIT   (1<<6)
#define NIXIE_1_4_BIT   (1<<7)
#define NIXIE_1_5_BIT   (1<<0)
#define NIXIE_1_6_BIT   (1<<1)
#define NIXIE_1_7_BIT   (1<<2)
#define NIXIE_1_8_BIT   (1<<3)
#define NIXIE_1_9_BIT   (1<<4)
#define NIXIE_1_LC_BIT  (1<<7)
#define NIXIE_1_RC_BIT  (1<<6)

#define NIXIE_2_0_BIT   (1<<7)
#define NIXIE_2_1_BIT   (1<<2)
#define NIXIE_2_2_BIT   (1<<3)
#define NIXIE_2_3_BIT   (1<<4)
#define NIXIE_2_4_BIT   (1<<5)
#define NIXIE_2_5_BIT   (1<<6)
#define NIXIE_2_6_BIT   (1<<7)
#define NIXIE_2_7_BIT   (1<<0)
#define NIXIE_2_8_BIT   (1<<1)
#define NIXIE_2_9_BIT   (1<<6)
#define NIXIE_2_LC_BIT  (1<<1)
#define NIXIE_2_RC_BIT  (1<<0)

#define NIXIE_0_0_REG_ID    4
#define NIXIE_0_1_REG_ID    4
#define NIXIE_0_2_REG_ID    4
#define NIXIE_0_3_REG_ID    0
#define NIXIE_0_4_REG_ID    0
#define NIXIE_0_5_REG_ID    0
#define NIXIE_0_6_REG_ID    0
#define NIXIE_0_7_REG_ID    0
#define NIXIE_0_8_REG_ID    0
#define NIXIE_0_9_REG_ID    4
#define NIXIE_0_LC_REG_ID   4
#define NIXIE_0_RC_REG_ID   4

#define NIXIE_1_0_REG_ID    3
#define NIXIE_1_1_REG_ID    4
#define NIXIE_1_2_REG_ID    4
#define NIXIE_1_3_REG_ID    0
#define NIXIE_1_4_REG_ID    0
#define NIXIE_1_5_REG_ID    1
#define NIXIE_1_6_REG_ID    1
#define NIXIE_1_7_REG_ID    1
#define NIXIE_1_8_REG_ID    1
#define NIXIE_1_9_REG_ID    3
#define NIXIE_1_LC_REG_ID   3
#define NIXIE_1_RC_REG_ID   3

#define NIXIE_2_0_REG_ID    2
#define NIXIE_2_1_REG_ID    3
#define NIXIE_2_2_REG_ID    3
#define NIXIE_2_3_REG_ID    1
#define NIXIE_2_4_REG_ID    1
#define NIXIE_2_5_REG_ID    1
#define NIXIE_2_6_REG_ID    1
#define NIXIE_2_7_REG_ID    2
#define NIXIE_2_8_REG_ID    2
#define NIXIE_2_9_REG_ID    2
#define NIXIE_2_LC_REG_ID   3
#define NIXIE_2_RC_REG_ID   3

/* Structures */
typedef struct nixie_tube_state
{
    bool digit_enable[TUBES_COUNT];
    uint8_t digit[TUBES_COUNT];
    bool left_comma_enable[TUBES_COUNT];
    bool right_comma_enable[TUBES_COUNT];
} display_state_t;

/* Functions */
/** @brief pp_nixie_display_init: Initializes nixie display.
 * 
 * This function sets all registers of expanders as outputs.
 *
 * @return
 */
void pp_nixie_display_init(void);

/** @brief pp_display: Displays given nixie tubes state.
 * 
 * This function sets outputs of expanders and siplay desired digits and commas of nixie tubes.
 *
 * @param[in]   nixie_state  (display_state_t*) Pointer to display_state_t structure.
 * 
 * @return
 */
void pp_display(display_state_t *display_state);

#endif