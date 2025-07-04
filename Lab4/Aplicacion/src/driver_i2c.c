#include "driver_i2c.h"
#include "hardware/i2c.h"
#include "pico/stdlib.h"
#include <string.h>
#include <stdio.h>

void eeprom_init(i2c_inst_t *i2c, uint sda_pin, uint scl_pin) {
    i2c_init(i2c, 400000); // 400 kHz
    gpio_set_function(sda_pin, GPIO_FUNC_I2C);
    gpio_set_function(scl_pin, GPIO_FUNC_I2C);
    gpio_pull_up(sda_pin);
    gpio_pull_up(scl_pin);
}

bool eeprom_write_nbytes(i2c_inst_t *i2c_port, uint8_t device_addr, uint8_t offset, uint8_t *data, uint8_t len) {
    if ((offset + len) > 256 || data == NULL) return false;

    uint8_t buf[18];
    buf[0] = offset;
    memcpy(&buf[1], data, len);

    int res = i2c_write_blocking(i2c_port, device_addr, buf, len + 1, false);

    if (res != len + 1) return false;

    sleep_ms(5);
    return true;
}

bool eeprom_read_nbytes(i2c_inst_t *i2c_port, uint8_t device_addr, uint8_t offset, uint8_t *data, uint8_t len) {
    if ((offset + len) > 256 || len == 0 || data == NULL) return false;

    int res = i2c_write_blocking(i2c_port, device_addr, &offset, 1, true);
    if (res != 1) return false;

    res = i2c_read_blocking(i2c_port, device_addr, data, len, false);
    if (res != (int)len) return false;

    return true;
}
