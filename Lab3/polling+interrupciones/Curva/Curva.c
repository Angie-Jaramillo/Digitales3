#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"

#define PIN_PWM             15   // Salida PWM al L298
#define PIN_ENCODER         14   // Entrada encoder óptico
#define IN1                 16   // L298 IN1
#define IN2                 17   // L298 IN2

#define PULSOS_POR_VUELTA   20    // Ranuras en el disco
#define TIEMPO_MUESTREO_MS  4   // Reporte cada 4 ms
#define TIEMPO_ESCALON_MS   2000 // Cambio de duty cycle cada 2 segundos
#define MAX_MUESTRAS        20000

#define FREQ_PWM            20000
#define WRAP                100

#ifndef SYS_CLK_KHZ
  #define SYS_CLK_KHZ       125000
#endif

typedef struct {
    uint32_t tiempo_ms;
    uint8_t pwm;
    float rpm;
} muestra_t;

muestra_t buffer[MAX_MUESTRAS];
uint32_t indice = 0;

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
    bool subida = true; // Indica si se está subiendo el duty cycle
    int duty = 0;

    absolute_time_t t_inicio = get_absolute_time();
    absolute_time_t t_muestreo = get_absolute_time();
    absolute_time_t t_escalon = get_absolute_time();
    gpio_set_irq_enabled_with_callback(PIN_ENCODER, GPIO_IRQ_EDGE_RISE, true, &encoder_isr);


    while (1) {
        pwm_set_chan_level(slice, chan, duty);

        absolute_time_t t_actual = get_absolute_time();
        int64_t delta_us = absolute_time_diff_us(t_muestreo, t_actual);
        if (delta_us >= TIEMPO_MUESTREO_MS * 1000) {
            float rpm = ((float)contador_pulsos / PULSOS_POR_VUELTA) * (60000.0f / TIEMPO_MUESTREO_MS);
            contador_pulsos = 0;

            if (indice < MAX_MUESTRAS) {
                buffer[indice].tiempo_ms = to_ms_since_boot(t_actual) - to_ms_since_boot(t_inicio);
                buffer[indice].pwm = duty;
                buffer[indice].rpm = rpm;
                indice++;
            }
            t_muestreo = t_actual;
        }
        // Cambio de duty cycle cada 2000 ms
        if (absolute_time_diff_us(t_escalon, t_actual) >= TIEMPO_ESCALON_MS * 1000) {
            if (subida) {
                duty += 20; 
                if (duty >= 100) {
                    duty = 100;        // Tocamos tope
                    subida = false;    // A partir de ahora bajamos
                }
            } else {
                duty -= 20; 
                if (duty < 0) {
                    duty = 0; 
                    break;            // Salimos del bucle principal
                }
            }
            t_escalon = t_actual;
        }
    }
        // Imprimir resultados como CSV
    printf("tiempo_ms,pwm,rpm\n");
    for (uint32_t i = 0; i < indice; i++) {
        printf("%u,%u,%.2f\n", buffer[i].tiempo_ms, buffer[i].pwm, buffer[i].rpm);
    }

    while (1) {}  // mantener el programa activo
    return 0;
}
