#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"

#define PIN_PWM     15
#define PIN_ENCODER 14
#define IN1         16
#define IN2         17

#define PULSOS_POR_VUELTA 20
#define WRAP 100
#define FREQ_PWM 10000

#define ESCALON_PORCENTAJE 20
#define TIEMPO_ESCALON_MS 2000
#define TIEMPO_MUESTREO_MS 4
#define MAX_DATOS 5000

typedef struct {
    uint32_t tiempo_ms;
    uint8_t pwm;
    float rpm;
} muestra_t;

muestra_t datos[MAX_DATOS];
uint32_t indice_datos = 0;

int main() {
    stdio_init_all();

    // Dirección fija del motor
    gpio_init(IN1); gpio_set_dir(IN1, GPIO_OUT); gpio_put(IN1, 1);
    gpio_init(IN2); gpio_set_dir(IN2, GPIO_OUT); gpio_put(IN2, 0);

    // Configurar PWM
    gpio_set_function(PIN_PWM, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(PIN_PWM);
    uint chan = pwm_gpio_to_channel(PIN_PWM);
    float clkdiv = 125000000.0f / ((WRAP + 1) * FREQ_PWM);
    if (clkdiv < 1.0f) clkdiv = 1.0f;
    pwm_config cfg = pwm_get_default_config();
    pwm_config_set_clkdiv(&cfg, clkdiv);
    pwm_config_set_wrap(&cfg, WRAP);
    pwm_init(slice, &cfg, false);
    pwm_set_enabled(slice, true);

    // Configurar encoder
    gpio_init(PIN_ENCODER);
    gpio_set_dir(PIN_ENCODER, GPIO_IN);
    gpio_pull_up(PIN_ENCODER);

    bool flanco_anterior = false;
    uint32_t contador_pulsos = 0;

    absolute_time_t tiempo_inicio = get_absolute_time();
    absolute_time_t tiempo_muestreo = get_absolute_time();

    printf("Captura de curva de reacción\n");

    // Secuencia de escalones: 0 → 20 → ... → 100 → 80 → ... → 0
    for (int etapa = 0; etapa < 2; etapa++) {
        int inicio = etapa == 0 ? 0 : 100 - ESCALON_PORCENTAJE;
        int fin    = etapa == 0 ? 100 : 0;
        int paso   = etapa == 0 ? ESCALON_PORCENTAJE : -ESCALON_PORCENTAJE;

        for (int pwm_val = inicio; (etapa == 0 ? pwm_val <= fin : pwm_val >= fin); pwm_val += paso) {
            pwm_set_chan_level(slice, chan, pwm_val);
            absolute_time_t t0 = get_absolute_time();
            tiempo_muestreo = t0;
            contador_pulsos = 0;

            while (absolute_time_diff_us(t0, get_absolute_time()) < TIEMPO_ESCALON_MS * 1000) {
                // Leer encoder por polling
                bool flanco_actual = gpio_get(PIN_ENCODER);
                if (flanco_actual && !flanco_anterior) {
                    contador_pulsos++;
                }
                flanco_anterior = flanco_actual;

                // Verificar si toca muestrear
                if (absolute_time_diff_us(tiempo_muestreo, get_absolute_time()) >= TIEMPO_MUESTREO_MS * 1000) {
                    // Calcular RPM
                    float rpm = (contador_pulsos * 1000.0f / TIEMPO_MUESTREO_MS) * (60.0f / PULSOS_POR_VUELTA);
                    contador_pulsos = 0;

                    uint32_t tiempo_rel = to_ms_since_boot(get_absolute_time()) - to_ms_since_boot(tiempo_inicio);

                    if (indice_datos < MAX_DATOS) {
                        datos[indice_datos++] = (muestra_t){tiempo_rel, pwm_val, rpm};
                    }

                    tiempo_muestreo = get_absolute_time();
                }
            }
        }
    }

    // Imprimir resultados en formato CSV
    printf("tiempo_ms,pwm,rpm\n");
    for (uint32_t i = 0; i < indice_datos; i++) {
        printf("%u,%u,%.2f\n", datos[i].tiempo_ms, datos[i].pwm, datos[i].rpm);
    }

    while (1) {
    }

    return 0;
}
