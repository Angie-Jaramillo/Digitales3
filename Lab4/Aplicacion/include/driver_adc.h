#ifndef DRIVER_ADC_H
#define DRIVER_ADC_H

#include "pico/stdlib.h"

void adc_driver_init(uint adc_gpio, uint adc_input);
uint16_t adc_read_sample(void);

#endif