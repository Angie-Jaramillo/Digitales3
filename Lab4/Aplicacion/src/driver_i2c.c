#include "i2c_eeprom.h"
#include "hardware/i2c.h"
#include "pico/stdlib.h"

#define EEPROM_I2C_INSTANCE i2c0
#define EEPROM_PAGE_SIZE 16

static i2c_inst_t *i2c_bus;

void eeprom_init(i2c_inst_t *i2c, uint sda_pin, uint scl_pin) {
    i2c_bus = i2c;
    i2c_init(i2c_bus, 400 * 1000); // 100 kHz
    gpio_set_function(sda_pin, GPIO_FUNC_I2C);
    gpio_set_function(scl_pin, GPIO_FUNC_I2C);
    gpio_pull_up(sda_pin);
    gpio_pull_up(scl_pin);
}


bool eeprom_write_byte(i2c_inst_t *i2c_port, uint8_t value) {
    uint8_t dev_addr = 0x50;        // Bloque 0, escritura
    uint8_t mem_addr = 0x00;        // Dirección dentro del bloque
    uint8_t data[2] = {mem_addr, value};

    int res = i2c_write_blocking(i2c_port, dev_addr, data, 2, false);
    if (res < 0) {
        printf("Error en i2c_write_blocking: %d\n", res);
        return false;
    } else if (res != 2) {
        printf("Solo se escribieron %d bytes\n", res);
        return false;
    }

    sleep_ms(5);
    return true;
}

uint8_t eeprom_read_byte(i2c_inst_t *i2c_port) {
    uint8_t dev_addr_wr = 0x50;
    uint8_t dev_addr_rd = 0x50;
    uint8_t mem_addr = 0x00;
    uint8_t buf = 0xFF;

    int res = i2c_write_blocking(i2c_port, dev_addr_wr, &mem_addr, 1, true);
    if (res < 0) {
        printf("Error en i2c_write_blocking (lectura): %d\n", res);
        return 0xEE;
    }

    res = i2c_read_blocking(i2c_port, dev_addr_rd, &buf, 1, false);
    if (res < 0) {
        printf("Error en i2c_read_blocking: %d\n", res);
        return 0xEE;
    }

    return buf;
}