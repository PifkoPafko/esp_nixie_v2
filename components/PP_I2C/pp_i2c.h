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

#ifndef __PP_I2C_H__
#define __PP_I2C_H__

/* Headers */
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_log.h"

/* Macros */
#define I2C_TAG "I2C"

#define I2C_MASTER_NUM  0

#define I2C_SDA_IO      GPIO_NUM_8
#define I2C_SCL_IO      GPIO_NUM_18

#define I2C_FREQ        100000
#define I2C_FREQ_MAX    1000000
#define I2C_FREQ_MIN    10000

#define I2CDEV_TIMEOUT	1000
#define MUTEX_ON		1

/* Functions */

/** @brief pp_i2c_init: Initializes I2C
 *
 * @return
 */
void pp_i2c_init(void);

/** @brief pp_i2c_check_dev: Checks respond of target device
 *
 * @param[in]   slave_addr  (uint8_t) Address of the target slave device
 * @return
 */
void pp_i2c_check_dev(uint8_t slave_addr);

/** @brief pp_i2c_write_byte_to_dev: Writes 1 byte to target device
 *
 * @param[in]   slave_addr  (uint8_t) Address of the target slave device
 * @param[in]   byte        (uint8_t) Data to write
 * @return
 */
void pp_i2c_write_byte_to_dev(uint8_t slave_addr, uint8_t byte);

/** @brief pp_i2c_write_word_to_dev: Writes 1 word to target device
 *
 * @param[in]   slave_addr  (uint8_t) Address of the target slave device
 * @param[in]   word        (uint16_t) Data to write 
 * @return
 */
void pp_i2c_write_word_to_dev(uint8_t slave_addr, uint16_t word);

/** @brief pp_i2c_read_byte_from_dev: Reads 1 byte from target device
 *
 * @param[in]   slave_addr  (uint8_t) Address of the target slave device
 * @param[out]  byte        (uint8_t*) Pointer to memory where the data will be stored
 * @return
 */
void pp_i2c_read_byte_from_dev(uint8_t slave_addr, uint8_t *byte);

/** @brief pp_i2c_read_word_from_dev: Reads 1 word from target device
 *
 * @param[in]   slave_addr  (uint8_t) Address of the target slave device
 * @param[out]  word        (uint16_t*) Pointer to memory where the data will be stored
 * @return
 */
void pp_i2c_read_word_from_dev(uint8_t slave_addr, uint16_t *word);

/** @brief pp_i2c_dev_read: Reads data from target device
 *
 * @param[in]   slave_addr  (uint8_t) Address of the target slave device
 * @param[in]   out_data    (const void*) Pointer to output data
 * @param[in]   out_size    (size_t) Size of output data
 * @param[out]  in_data     (void*) Pointer to memory where the read data will be stored
 * @param[in]   in_size     (size_t) Maximum size of read data
 * @return
 */
void pp_i2c_dev_read(uint8_t slave_addr, const void *out_data, size_t out_size, void *in_data, size_t in_size);

/** @brief pp_i2c_dev_write: Writes data to target device
 *
 * @param[in]   slave_addr   (uint8_t) Address of the target slave device
 * @param[in]   out_reg      (const uint8_t*) Pointer to output register
 * @param[in]   out_reg_size (size_t) Size of output register
 * @param[in]   out_data     (const uint8_t*) Pointer to output data
 * @param[in]   out_size     (size_t) Size of output data
 * @return
 */
void pp_i2c_dev_write(uint8_t slave_addr, const uint8_t *out_reg, size_t out_reg_size, const uint8_t *out_data, size_t out_size);

/** @brief pp_i2c_dev_read_reg: Reads register from target device
 *
 * @param[in]   slave_addr  (uint8_t) Address of the target slave device
 * @param[in]   reg         (uint8_t) Target device register to read
 * @param[out]  in_data     (void*) Pointer to memory where the read data will be stored
 * @param[in]   in_size     (size_t) Maximum size of read data
 * @return
 */
void pp_i2c_dev_read_reg(uint8_t slave_addr, uint8_t reg, void *in_data, size_t in_size);

/** @brief pp_i2c_dev_write_reg: Writes register from target device
 *
 * @param[in]   slave_addr  (uint8_t) Address of the target slave device
 * @param[in]   reg         (uint8_t) Target device register to write
 * @param[in]   out_data    (const void*) Pointer to write data
 * @param[in]   in_size     (size_t) Size of write data
 * @return
 */
void pp_i2c_dev_write_reg(uint8_t slave_addr, uint8_t reg, const void *out_data, size_t out_size);

#endif
