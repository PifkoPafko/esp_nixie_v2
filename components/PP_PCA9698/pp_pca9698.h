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
#include <stdint.h>
#include "pp_i2c.h"
// #include "esp_err.h"

/* Macros */
#define WRITE_BIT_MASK(x) (((x) << 1) & 0xFE)
#define READ_BIT_MASK(x) (((x) << 1) | 0x01)

#define ENABLE_AUTO_INCREMEMT_BIT_MASK(x) ((x) | 0x80)
#define DISABLE_AUTO_INCREMEMT_BIT_MASK(x) ((x) & 0x7F)

/* SLAVE ADDRESSES */
typedef enum{
    SLAVE_ADDR_0 =          0x20,
    SLAVE_ADDR_1 =          0x21,
    SLAVE_ADDR_2 =          0x22,
    SLAVE_ADDR_3 =          0x23,
    SLAVE_ADDR_4 =          0x24,
    SLAVE_ADDR_5 =          0x25,
    SLAVE_ADDR_6 =          0x26,
    SLAVE_ADDR_7 =          0x27,
    GPIO_ALL_CALL_ADDR =    0xDC    /* GPIO ALL CALL ADRESS*/
}slave_addr_t;

/* REGISTERS ADDRESSES */
typedef enum{
    /* INPUT REGISTERS ADDRESSES */
    IP0_ADDR =  0x00,
    IP1_ADDR =  0x01,
    IP2_ADDR =  0x02,
    IP3_ADDR =  0x03,
    IP4_ADDR =  0x04,
    /* OUTPUT REGISTERS ADDRESSES */
    OP0_ADDR =  0x08,
    OP1_ADDR =  0x09,
    OP2_ADDR =  0x0A,
    OP3_ADDR =  0x0B,
    OP4_ADDR =  0x0C,
    /* POLARITY INVERSION REGISTERS ADDRESSES */
    PI0_ADDR =  0x10,
    PI1_ADDR =  0x11,
    PI2_ADDR =  0x12,
    PI3_ADDR =  0x13,
    PI4_ADDR =  0x14,
    /* CONFIGURATION REGISTERS ADDRESSES */
    IOC0_ADDR =  0x18,
    IOC1_ADDR =  0x19,
    IOC2_ADDR =  0x1A,
    IOC3_ADDR =  0x1B,
    IOC4_ADDR =  0x1C,
    /* MASK INTERRUPT REGISTERS ADDRESSES */
    MSK0_ADDR =  0x20,
    MSK1_ADDR =  0x21,
    MSK2_ADDR =  0x22,
    MSK3_ADDR =  0x23,
    MSK4_ADDR =  0x24,
    /* MISCELLANOUS COMMANDS */
    OUTCONF_ADDR = 0x28,
    ALLBANK_ADDR = 0x29,
    MODE_ADDR    = 0x29,
}reg_addr_t;

/* Functions */

/** @brief pp_pca_write_reg: Write one chosen IO register in chosen PCA9698
 *
 * @param[in]   slave_addr  (slave_addr_t) Address of PCA9698
 * @param[in]   reg         (reg_addr_t) Address of register
 * @param[in]   arg         (const uint8_t) Data to write
 * 
 * @return
 */
void pp_pca_write_reg(slave_addr_t slave_addr, reg_addr_t reg, const uint8_t arg);

/** @brief pp_pca_write_reg: Write all 5 IO registers in chosen PCA9698
 *
 * @param[in]   slave_addr  (slave_addr_t) Address of PCA9698
 * @param[in]   reg         (reg_addr_t) Address of register
 * @param[in]   arg         (const uint8_t*) Pointer to data to write
 * 
 * @return
 */
void pp_pca_write_all_reg(slave_addr_t slave_addr, reg_addr_t reg, const uint8_t* arg);