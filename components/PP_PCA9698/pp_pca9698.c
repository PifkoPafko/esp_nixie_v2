#include "pp_pca9698.h"
#include "mk_i2c.h"

esp_err_t pca_write_reg(uint8_t port_nr, slave_addr_t slave_addr, reg_addr_t reg, uint8_t arg) {
    uint8_t slave_write_addr = WRITE_BIT_MASK(slave_addr);
    uint8_t reg_addr = DISABLE_AUTO_INCREMEMT_BIT_MASK(reg);
    return i2c_dev_write(port_nr, slave_write_addr, (const uint8_t*)&reg_addr, 1, (const uint8_t*)&arg, 1);
}

esp_err_t pca_write_all_reg(uint8_t port_nr, slave_addr_t slave_addr, reg_addr_t reg, const uint8_t* arg) {
    uint8_t slave_write_addr = WRITE_BIT_MASK(slave_addr);
    uint8_t reg_addr = ENABLE_AUTO_INCREMEMT_BIT_MASK(reg);
    return i2c_dev_write(port_nr, slave_write_addr, (const uint8_t*)&reg_addr, 1, arg, 5);
}

