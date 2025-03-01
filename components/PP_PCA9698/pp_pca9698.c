#include "pp_pca9698.h"
#include "pp_i2c.h"

void pp_pca_write_reg(slave_addr_t slave_addr, reg_addr_t reg, uint8_t arg) {
    uint8_t slave_write_addr = WRITE_BIT_MASK(slave_addr);
    uint8_t reg_addr = DISABLE_AUTO_INCREMEMT_BIT_MASK(reg);
    pp_i2c_dev_write(slave_write_addr, (const uint8_t*)&reg_addr, 1, (const uint8_t*)&arg, 1);
}

void pp_pca_write_all_reg(slave_addr_t slave_addr, reg_addr_t reg, const uint8_t* arg) {
    uint8_t slave_write_addr = WRITE_BIT_MASK(slave_addr);
    uint8_t reg_addr = ENABLE_AUTO_INCREMEMT_BIT_MASK(reg);
    pp_i2c_dev_write(slave_write_addr, (const uint8_t*)&reg_addr, 1, arg, 5);
}

