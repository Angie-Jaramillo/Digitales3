#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"

#define PIN_PWM             15   // Salida PWM al L298
#define PIN_ENCODER         14   // Entrada encoder óptico
#define IN1                 16   // L298 IN1
#define IN2                 17   // L298 IN2

#define PULSOS_POR_VUELTA   20    // Ranuras en el disco
#define TIEMPO_REPORTE_MS   2000   // Reporte cada 500 ms

#define FREQ_PWM            10000
#define WRAP                100

#ifndef SYS_CLK_KHZ
  #define SYS_CLK_KHZ       125000
#endif

static volatile uint32_t contador_pulsos = 0;

void encoder_isr(uint gpio, uint32_t events) {
    contador_pulsos++;
}
int main() {
    stdio_init_all();

    // Activar motor hacia adelante con L298
    gpio_init(IN1);
    gpio_set_dir(IN1, GPIO_OUT);
    gpio_put(IN1, 1);  // Motor adelante

    gpio_init(IN2);
    gpio_set_dir(IN2, GPIO_OUT);
    gpio_put(IN2, 0);  // Motor adelante

    // Configurar pin del encoder
    gpio_init(PIN_ENCODER);
    gpio_set_dir(PIN_ENCODER, GPIO_IN);
    gpio_pull_up(PIN_ENCODER);

    // Configurar PWM
    gpio_set_function(PIN_PWM, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(PIN_PWM);
    uint chan  = pwm_gpio_to_channel(PIN_PWM);

    float divider = (SYS_CLK_KHZ * 1000.0f) / (FREQ_PWM * WRAP);
    if (divider < 1.0f) divider = 1.0f;
    if (divider > 255.0f) divider = 255.0f;

    pwm_config cfg = pwm_get_default_config();
    pwm_config_set_clkdiv(&cfg, divider);
    pwm_config_set_wrap(&cfg, WRAP);
    pwm_init(slice, &cfg, true);


    // Variables de conteo
    uint8_t duty = 0;
    absolute_time_t t_reporte = get_absolute_time();
    gpio_set_irq_enabled_with_callback(PIN_ENCODER, GPIO_IRQ_EDGE_RISE, true, &encoder_isr);


    while (1) {
        pwm_set_chan_level(slice, chan, duty);
        absolute_time_t t_actual = get_absolute_time();
        int64_t delta_us = absolute_time_diff_us(t_reporte, t_actual);
        if (delta_us >= TIEMPO_REPORTE_MS * 1000) {
            float rpm = ((float)contador_pulsos / PULSOS_POR_VUELTA) * (60000.0f / TIEMPO_REPORTE_MS);
            printf("RPM = %.2f\n", rpm);
            contador_pulsos = 0;
            t_reporte = t_actual;
            duty += 20; // Aumentar el duty cycle
            if (duty > 100) {
                duty = 0;
            }
        }
    }
    return 0;
}
