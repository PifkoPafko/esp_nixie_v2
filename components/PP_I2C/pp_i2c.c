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
#include "pp_i2c.h"

/* Variables */
static SemaphoreHandle_t i2c_mutex;

/* Functions */

/** @brief i2c_create_mutex: Creates mutex for I2C
 *
 * @return  esp_err_t
 */
static esp_err_t i2c_create_mutex(void) 
{
	i2c_mutex = xSemaphoreCreateMutex();
    if(!i2c_mutex)
    {
        ESP_LOGE(I2C_TAG, "Could not create device i2c_mutex");
        return ESP_FAIL;
    }

    return ESP_OK;
}

/** @brief i2c_take_mutex: Takes mutex for I2C
 *
 * @return  esp_err_t
 */
static esp_err_t i2c_take_mutex(void) 
{
#if MUTEX_ON == 1
    if(!xSemaphoreTake(i2c_mutex, pdMS_TO_TICKS(I2CDEV_TIMEOUT)))
    {
        ESP_LOGE(I2C_TAG, "Could not take i2c_mutex");
        return ESP_ERR_TIMEOUT;
    }
#endif

    return ESP_OK;
}

/** @brief i2c_give_mutex: Gives mutex for I2C
 *
 * @return  esp_err_t
 */
static esp_err_t i2c_give_mutex(void) 
{
#if MUTEX_ON == 1
    if(!xSemaphoreGive(i2c_mutex)) 
    {
        ESP_LOGE(I2C_TAG, "Could not give i2c_mutex");
        return ESP_FAIL;
    }
#endif

    return ESP_OK;
}

/** @brief i2c_init: Initializes I2C
 *
 * @return
 */
void i2c_init(void) 
{
    uint32_t freq = I2C_FREQ;
	if(freq < I2C_FREQ_MIN) freq = I2C_FREQ_MIN;
	if(freq > I2C_FREQ_MAX) freq = I2C_FREQ_MAX;

    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = I2C_SDA_IO;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num = I2C_SCL_IO;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = freq;
    conf.clk_flags = I2C_SCLK_SRC_FLAG_FOR_NOMAL;

    ESP_ERROR_CHECK(i2c_param_config(I2C_MASTER_NUM, &conf));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_MASTER_NUM, I2C_MODE_MASTER, 0, 0, 0));
    ESP_ERROR_CHECK(i2c_create_mutex());
}

/** @brief i2c_check_dev: Checks respond of target device
 *
 * @param[in]   slave_addr  (uint8_t) Address of the target slave device
 * @return
 */
void i2c_check_dev(uint8_t slave_addr)
{
	ESP_ERROR_CHECK(i2c_take_mutex());

    i2c_cmd_handle_t icmd = i2c_cmd_link_create();
	ESP_ERROR_CHECK(i2c_master_start(icmd));
	ESP_ERROR_CHECK(i2c_master_write_byte(icmd, slave_addr, 1));
	ESP_ERROR_CHECK(i2c_master_stop(icmd));
	ESP_ERROR_CHECK(i2c_master_cmd_begin(I2C_MASTER_NUM, icmd, 100));
    i2c_cmd_link_delete(icmd);

    ESP_ERROR_CHECK(i2c_give_mutex());
}

/** @brief i2c_write_byte_to_dev: Writes 1 byte to target device
 *
 * @param[in]   slave_addr  (uint8_t) Address of the target slave device
 * @param[in]   byte        (uint8_t) Data to write
 * @return
 */
void i2c_write_byte_to_dev(uint8_t slave_addr, uint8_t byte)
{
	ESP_ERROR_CHECK(i2c_take_mutex());

    i2c_cmd_handle_t icmd = i2c_cmd_link_create();
	ESP_ERROR_CHECK(i2c_master_start(icmd));
	ESP_ERROR_CHECK(i2c_master_write_byte(icmd, slave_addr, 1));
	ESP_ERROR_CHECK(i2c_master_write_byte(icmd, byte, 1));
	ESP_ERROR_CHECK(i2c_master_stop(icmd));
	ESP_ERROR_CHECK(i2c_master_cmd_begin(I2C_MASTER_NUM, icmd, 100));
    i2c_cmd_link_delete(icmd);

    ESP_ERROR_CHECK(i2c_give_mutex());
}

/** @brief i2c_write_word_to_dev: Writes 1 word to target device
 *
 * @param[in]   slave_addr  (uint8_t) Address of the target slave device
 * @param[in]   word        (uint16_t) Data to write 
 * @return
 */
void i2c_write_word_to_dev(uint8_t slave_addr, uint16_t word)
{
	ESP_ERROR_CHECK(i2c_take_mutex());

    i2c_cmd_handle_t icmd = i2c_cmd_link_create();
	ESP_ERROR_CHECK(i2c_master_start(icmd));
	ESP_ERROR_CHECK(i2c_master_write_byte(icmd, slave_addr, 1));
	ESP_ERROR_CHECK(i2c_master_write_byte(icmd, (word >> 8), 1));		// MSB
	ESP_ERROR_CHECK(i2c_master_write_byte(icmd, word & 0xFF, 1));		// LSB
	ESP_ERROR_CHECK(i2c_master_stop(icmd));
	ESP_ERROR_CHECK(i2c_master_cmd_begin(I2C_MASTER_NUM, icmd, 100));
    i2c_cmd_link_delete(icmd);

    ESP_ERROR_CHECK(i2c_give_mutex());
}

/** @brief i2c_read_byte_from_dev: Reads 1 byte from target device
 *
 * @param[in]   slave_addr  (uint8_t) Address of the target slave device
 * @param[out]  byte        (uint8_t*) Pointer to memory where the data will be stored
 * @return
 */
void i2c_read_byte_from_dev(uint8_t slave_addr, uint8_t *byte)
{
	ESP_ERROR_CHECK(i2c_take_mutex());

    i2c_cmd_handle_t icmd = i2c_cmd_link_create();
	ESP_ERROR_CHECK(i2c_master_start(icmd));
	ESP_ERROR_CHECK(i2c_master_write_byte(icmd, slave_addr | 1, 1));
	ESP_ERROR_CHECK(i2c_master_read_byte(icmd, byte, 1));
	ESP_ERROR_CHECK(i2c_master_stop(icmd));
	ESP_ERROR_CHECK(i2c_master_cmd_begin(I2C_MASTER_NUM, icmd, 100));
    i2c_cmd_link_delete(icmd);

    ESP_ERROR_CHECK(i2c_give_mutex());
}

/** @brief i2c_read_word_from_dev: Reads 1 word from target device
 *
 * @param[in]   slave_addr  (uint8_t) Address of the target slave device
 * @param[out]  word        (uint16_t*) Pointer to memory where the data will be stored
 * @return
 */
void i2c_read_word_from_dev(uint8_t slave_addr, uint16_t *word)
{
	uint8_t msb, lsb;
	ESP_ERROR_CHECK(i2c_take_mutex());

    i2c_cmd_handle_t icmd = i2c_cmd_link_create();
	ESP_ERROR_CHECK(i2c_master_start(icmd));
	ESP_ERROR_CHECK(i2c_master_write_byte(icmd, slave_addr | 1, 1));
	ESP_ERROR_CHECK(i2c_master_read_byte(icmd, &msb, 0));				// MSB
	ESP_ERROR_CHECK(i2c_master_read_byte(icmd, &lsb, 1));				// LSB
	ESP_ERROR_CHECK(i2c_master_stop(icmd));
	ESP_ERROR_CHECK(i2c_master_cmd_begin(I2C_MASTER_NUM, icmd, 100));
    i2c_cmd_link_delete(icmd);

    *word = (msb<<8) | lsb;
    ESP_ERROR_CHECK(i2c_give_mutex());
}

/** @brief i2c_dev_read: Reads data from target device
 *
 * @param[in]   slave_addr  (uint8_t) Address of the target slave device
 * @param[in]   out_data    (const void*) Pointer to output data
 * @param[in]   out_size    (size_t) Size of output data
 * @param[out]  in_data     (void*) Pointer to memory where the read data will be stored
 * @param[in]   in_size     (size_t) Maximum size of read data
 * @return
 */
void i2c_dev_read(uint8_t slave_addr, const void *out_data, size_t out_size, void *in_data, size_t in_size)
{
    if(!in_data || !in_size)
    {
        ESP_ERROR_CHECK(ESP_ERR_INVALID_ARG);
    }

    ESP_ERROR_CHECK(i2c_take_mutex());

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    if(out_data && out_size)
    {
        ESP_ERROR_CHECK(i2c_master_start(cmd));
        ESP_ERROR_CHECK(i2c_master_write_byte(cmd, slave_addr, true));
        ESP_ERROR_CHECK(i2c_master_write(cmd, out_data, out_size, true));
    }
    ESP_ERROR_CHECK(i2c_master_start(cmd));
    ESP_ERROR_CHECK(i2c_master_write_byte(cmd, slave_addr | 1, true));
    ESP_ERROR_CHECK(i2c_master_read(cmd, in_data, in_size, I2C_MASTER_LAST_NACK));
    ESP_ERROR_CHECK(i2c_master_stop(cmd));

    ESP_ERROR_CHECK(i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(I2CDEV_TIMEOUT)));
    i2c_cmd_link_delete(cmd);

    ESP_ERROR_CHECK(i2c_give_mutex());
}

/** @brief i2c_dev_write: Writes data to target device
 *
 * @param[in]   slave_addr   (uint8_t) Address of the target slave device
 * @param[in]   out_reg      (const uint8_t*) Pointer to output register
 * @param[in]   out_reg_size (size_t) Size of output register
 * @param[in]   out_data     (const uint8_t*) Pointer to output data
 * @param[in]   out_size     (size_t) Size of output data
 * @return
 */
void i2c_dev_write(uint8_t slave_addr, const uint8_t *out_reg, size_t out_reg_size, const uint8_t *out_data, size_t out_size)
{
    if(!out_data || !out_size) 
    {
        ESP_ERROR_CHECK(ESP_ERR_INVALID_ARG);
    }

    ESP_ERROR_CHECK(i2c_take_mutex());

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    ESP_ERROR_CHECK(i2c_master_start(cmd));
    ESP_ERROR_CHECK(i2c_master_write_byte(cmd, slave_addr, true));

    if(out_reg && out_reg_size)
    {
        ESP_ERROR_CHECK(i2c_master_write(cmd, out_reg, out_reg_size, true));
    }

    ESP_ERROR_CHECK(i2c_master_write(cmd, out_data, out_size, true));
    ESP_ERROR_CHECK(i2c_master_stop(cmd));
    ESP_ERROR_CHECK(i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(I2CDEV_TIMEOUT)));
    i2c_cmd_link_delete(cmd);

    ESP_ERROR_CHECK(i2c_give_mutex());
}

/** @brief i2c_dev_read_reg: Reads register from target device
 *
 * @param[in]   slave_addr  (uint8_t) Address of the target slave device
 * @param[in]   reg         (uint8_t) Target device register to read
 * @param[out]  in_data     (void*) Pointer to memory where the read data will be stored
 * @param[in]   in_size     (size_t) Maximum size of read data
 * @return
 */
void i2c_dev_read_reg(uint8_t slave_addr, uint8_t reg, void *in_data, size_t in_size)
{
    i2c_dev_read(slave_addr, &reg, 1, in_data, in_size);
}

/** @brief i2c_dev_write_reg: Writes register from target device
 *
 * @param[in]   slave_addr  (uint8_t) Address of the target slave device
 * @param[in]   reg         (uint8_t) Target device register to write
 * @param[in]   out_data    (const void*) Pointer to write data
 * @param[in]   in_size     (size_t) Size of write data
 * @return
 */
void i2c_dev_write_reg(uint8_t slave_addr, uint8_t reg, const void *out_data, size_t out_size)
{
    i2c_dev_write(slave_addr, &reg, 1, out_data, out_size);
}