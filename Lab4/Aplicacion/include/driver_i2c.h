#ifndef I2C_EEPROM_H
#define I2C_EEPROM_H

#include "pico/stdlib.h"
#include "hardware/i2c.h"


void eeprom_init(i2c_inst_t *i2c, uint sda_pin, uint scl_pin);
bool eeprom_write_byte(i2c_inst_t *i2c_port, uint8_t value);
uint8_t eeprom_read_byte(i2c_inst_t *i2c_port);

#endif
