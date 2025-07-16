#ifndef DRIVER_I2C_H
#define DRIVER_I2C_H

#include "pico/stdlib.h"
#include "hardware/i2c.h"


void eeprom_init(i2c_inst_t *i2c, uint sda_pin, uint scl_pin);
bool eeprom_write_nbytes(i2c_inst_t *i2c_port, uint8_t device_addr, uint8_t offset, uint8_t *data, uint8_t len);
bool eeprom_read_nbytes(i2c_inst_t *i2c_port, uint8_t device_addr, uint8_t offset, uint8_t *data, uint8_t len);

#endif
