#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "driver_i2c.h"

#define SDA_PIN 16
#define SCL_PIN 17

#define LED_PIN 25

int main() {
    stdio_init_all();
    sleep_ms(5000); // Espera para estabilizar la conexión

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 1); // Enciende el LED para indicar inicio
    
    eeprom_init(i2c0, SDA_PIN, SCL_PIN);
    printf("EEPROM inicializada\n");

    i2c_scan_bus(i2c0);

    const uint16_t test_addr = 0x0000;
    const uint8_t test_value = 0xAB;

    printf("Escribiendo 0x%02X en dirección 0x%04X\n", test_value, test_addr);
    if (!eeprom_write_byte(i2c0, test_value)) {
        printf("Error escribiendo en EEPROM\n");
        return 1;
    }

    sleep_ms(10); // Tiempo extra por seguridad

    printf("Leyendo desde dirección 0x0000...\n");
    uint8_t read_value = eeprom_read_byte(i2c0);
    printf("Valor leído: 0x%02X\n", read_value);

    if (read_value == test_value) {
        printf("Verificación correcta: los valores coinciden\n");
    } else {
        printf("Verificación fallida: valor incorrecto\n");
    }

    return 0;
}
