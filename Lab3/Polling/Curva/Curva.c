#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"

#define PIN_PWM     15
#define PIN_ENCODER 14
#define IN1         16
#define IN2         17

#define CPR                 20    // Pulsos por vuelta
#define FREQ_PWM            10000
#define WRAP                100
#define ESCALON_PORCENTAJE  20
#define TIEMPO_ESCALON_MS   2000
#define TIEMPO_MUESTREO_MS  50
#define MAX_MUESTRAS        5000
#ifndef SYS_CLK_KHZ
    #define SYS_CLK_KHZ     125000  // 125 MHz
#endif

typedef struct {
    uint32_t tiempo_ms;
    uint8_t pwm;
    float rpm;
} muestra_t;

muestra_t buffer[MAX_MUESTRAS];
uint32_t indice = 0;

int main() {
    stdio_init_all();

    // Configurar pines de dirección (L298N)
    gpio_init(IN1);
    gpio_set_dir(IN1, GPIO_OUT);
    gpio_put(IN1, 1);

    gpio_init(IN2);
    gpio_set_dir(IN2, GPIO_OUT);
    gpio_put(IN2, 0);

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

    // Configurar encoder
    gpio_init(PIN_ENCODER);
    gpio_set_dir(PIN_ENCODER, GPIO_IN);
    gpio_pull_up(PIN_ENCODER);

    bool flanco_anterior = gpio_get(PIN_ENCODER);
    volatile uint32_t pulsos = 0;

    absolute_time_t t_inicio = get_absolute_time();
    absolute_time_t t_muestreo = get_absolute_time();
    absolute_time_t t_escalon = get_absolute_time();

    int pwm_actual = 0;
    int paso = ESCALON_PORCENTAJE;
    bool bajando = false;

    pwm_set_chan_level(slice, chan, pwm_actual);

    while (1) {

        bool flanco_actual = gpio_get(PIN_ENCODER);
        if (flanco_actual && !flanco_anterior) {
            pulsos++;
        }
        flanco_anterior = flanco_actual;

        // 2. Muestreo: medir cada 4 ms cuánto tiempo realmente pasó
        absolute_time_t t_ahora = get_absolute_time();
        int64_t delta_us = absolute_time_diff_us(t_muestreo, t_ahora);
        if (delta_us >= TIEMPO_MUESTREO_MS * 1000) {
            float delta_ms = delta_us / 1000.0f;

            // RPM = (pulsos / CPR) / (delta_ms / 60000)
            float rpm = ((float)pulsos / CPR) * (60000.0f / delta_ms);
            pulsos = 0;

            if (indice < MAX_MUESTRAS) {
                buffer[indice].tiempo_ms = to_ms_since_boot(t_ahora) - to_ms_since_boot(t_inicio);
                buffer[indice].pwm = pwm_actual;
                buffer[indice].rpm = rpm;
                indice++;
            }

            t_muestreo = t_ahora;
        }

        // 3. Cambio de escalón cada 2 s
        if (absolute_time_diff_us(t_escalon, get_absolute_time()) >= TIEMPO_ESCALON_MS * 1000) {
            pwm_actual += bajando ? -paso : paso;
            if (pwm_actual >= 100) {
                pwm_actual = 100;
                bajando = true;
            } else if (pwm_actual <= 0 && bajando) {
                pwm_actual = 0;
                break; // Fin de la captura
            }

            pwm_set_chan_level(slice, chan, pwm_actual);
            t_escalon = get_absolute_time();
        }

        tight_loop_contents();
    }

    // Imprimir resultados como CSV
    printf("tiempo_ms,pwm,rpm\n");
    for (uint32_t i = 0; i < indice; i++) {
        printf("%u,%u,%.2f\n", buffer[i].tiempo_ms, buffer[i].pwm, buffer[i].rpm);
    }

    while (1) {}  // mantener el programa activo
    return 0;
}