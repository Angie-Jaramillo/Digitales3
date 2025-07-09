#include "driver_adc.h"
#include "hardware/adc.h"

// Inicializa ADC y pin
void adc_driver_init(uint adc_gpio, uint adc_input) {
    adc_init();
    adc_gpio_init(adc_gpio);
    adc_select_input(adc_input);
}

uint16_t adc_read_sample(void) {
    return adc_read(); // Devuelve 12 bits (0-4095)
}