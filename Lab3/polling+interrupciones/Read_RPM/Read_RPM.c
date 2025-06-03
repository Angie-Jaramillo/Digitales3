/*@file Read_RPM.c
 * @brief Programa para medir RPM de un motor DC usando un encoder óptico y PWM en Raspberry Pi Pico.
 * @details Este programa utiliza interrupciones para contar los pulsos del encoder y calcula las RPM basándose en el número de pulsos por vuelta.
 * @author Angie Paola Jaramillo Ortega y Juan Manuel Rivera Flores
 * @year 2025
*/
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"

#define PIN_PWM             15   ///< Salida PWM al L298
#define PIN_ENCODER             14   ///< Entrada encoder óptico
#define IN1                 16   ///< L298 IN1
#define IN2                 17   ///< L298 IN2

#define PULSOS_POR_VUELTA   20    ///< Ranuras en el disco
#define TIEMPO_REPORTE_MS   500   ///< Reporte cada 500 ms

#define FREQ_PWM            10000 ///< Frecuencia PWM en Hz
#define WRAP                100   ///< Valor de wrap para el PWM

#ifndef SYS_CLK_KHZ
  #define SYS_CLK_KHZ       125000
#endif

static volatile uint32_t contador_pulsos = 0;

/**
 * @brief Interrupción para contar pulsos del encoder.
 * 
 * Esta función se llama cada vez que se detecta un flanco de subida en el pin del encoder.
 * Incrementa el contador de pulsos.
 * 
 * @param gpio Pin GPIO que generó la interrupción.
 * @param events Eventos de interrupción.
 */
void encoder_isr(uint gpio, uint32_t events) {
    contador_pulsos++;
}

/**
 * @brief Función principal del programa.
 * 
 * Configura los pines, inicializa el PWM y comienza a contar los pulsos del encoder.
 * Calcula las RPM cada 500 ms y las imprime en la consola.
 * 
 * @return int Código de salida del programa.
 */
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
    pwm_set_chan_level(slice, chan, 70); // Ajustar el duty cycle al 70%

    // Variables de conteo
    absolute_time_t t_reporte = get_absolute_time();
    gpio_set_irq_enabled_with_callback(PIN_ENCODER, GPIO_IRQ_EDGE_RISE, true, &encoder_isr);


    while (1) {
        // Calcular RPM cada 500 ms
        absolute_time_t t_actual = get_absolute_time();
        int64_t delta_us = absolute_time_diff_us(t_reporte, t_actual);
        if (delta_us >= TIEMPO_REPORTE_MS * 1000) {
            float rpm = ((float)contador_pulsos / PULSOS_POR_VUELTA) * (60000.0f / TIEMPO_REPORTE_MS);
            printf("RPM = %.2f\n", rpm);
            contador_pulsos = 0;
            t_reporte = t_actual;
        }
    }

    return 0;
}
