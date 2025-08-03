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
#include "pp_pca9698.h"

/* Functions */

/** @brief pp_pca_write_reg: Write one chosen IO register in chosen PCA9698
 *
 * @param[in]   slave_addr  (slave_addr_t) Address of PCA9698
 * @param[in]   reg         (reg_addr_t) Address of register
 * @param[in]   arg         (const uint8_t) Data to write
 * 
 * @return
 */
void pp_pca_write_reg(slave_addr_t slave_addr, reg_addr_t reg, const uint8_t arg) 
{
    uint8_t slave_write_addr = WRITE_BIT_MASK(slave_addr);
    uint8_t reg_addr = DISABLE_AUTO_INCREMEMT_BIT_MASK(reg);
    pp_i2c_dev_write(slave_write_addr, (const uint8_t*)&reg_addr, 1, (const uint8_t*)&arg, 1);
}

/** @brief pp_pca_write_reg: Write all 5 IO registers in chosen PCA9698
 *
 * @param[in]   slave_addr  (slave_addr_t) Address of PCA9698
 * @param[in]   reg         (reg_addr_t) Address of register
 * @param[in]   arg         (const uint8_t*) Pointer to data to write
 * 
 * @return
 */
void pp_pca_write_all_reg(slave_addr_t slave_addr, reg_addr_t reg, const uint8_t* arg) 
{
    uint8_t slave_write_addr = WRITE_BIT_MASK(slave_addr);
    uint8_t reg_addr = ENABLE_AUTO_INCREMEMT_BIT_MASK(reg);
    pp_i2c_dev_write(slave_write_addr, (const uint8_t*)&reg_addr, 1, arg, 5);
}

