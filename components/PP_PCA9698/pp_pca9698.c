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
 * @param[in]   dev_handle  (i2c_master_dev_handle_t) I2C device handle
 * @param[in]   reg         (reg_addr_t) Address of register
 * @param[in]   arg         (const uint8_t) Data to write
 * 
 * @return
 */
void pp_pca_write_reg(i2c_master_dev_handle_t dev_handle, reg_addr_t reg, const uint8_t arg) 
{
    uint8_t data_out[2] = {DISABLE_AUTO_INCREMEMT_BIT_MASK(reg), arg};
    ESP_ERROR_CHECK(i2c_master_transmit(dev_handle, data_out, 2, -1));
}

/** @brief pp_pca_write_reg: Write all 5 IO registers in chosen PCA9698
 *
 * @param[in]   dev_handle  (i2c_master_dev_handle_t) I2C device handle
 * @param[in]   reg         (reg_addr_t) Address of register
 * @param[in]   arg         (const uint8_t*) Pointer to data to write
 * 
 * @return
 */
void pp_pca_write_all_reg(i2c_master_dev_handle_t dev_handle, reg_addr_t reg, const uint8_t* arg) 
{
    uint8_t data_out[6];
    data_out[0] = ENABLE_AUTO_INCREMEMT_BIT_MASK(reg);

    for(int i = 1; i < 6; ++i)
    {
        data_out[i] = arg[i-1];
    }

    i2c_master_transmit(dev_handle, data_out, 6, -1);
}