#include "pp_pca9698_defs.h"
#include "esp_err.h"

#define WRITE_BIT_MASK(x) (((x) << 1) & 0xFE)
#define READ_BIT_MASK(x) (((x) << 1) | 0x01)

#define ENABLE_AUTO_INCREMEMT_BIT_MASK(x) ((x) | 0x80)
#define DISABLE_AUTO_INCREMEMT_BIT_MASK(x) ((x) & 0x7F)

esp_err_t pca_write_reg(uint8_t port_nr, slave_addr_t slave_addr, reg_addr_t reg, uint8_t arg);
esp_err_t pca_write_all_reg(uint8_t port_nr, slave_addr_t slave_addr, reg_addr_t reg, const uint8_t* arg);