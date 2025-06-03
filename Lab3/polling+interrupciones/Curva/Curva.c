/*@file Curva.c
 * @brief Programa para capturar la curva de reacción de un motor DC.
 * @details Este programa utiliza PWM para controlar un motor DC y captura la velocidad del motor en función del 
 * duty cycle aplicado. Utiliza interrupciones para contar pulsos de un encoder óptico y almacena los datos en un 
 * buffer que se envía al PC al finalizar la secuencia.
 *
 * @author Angie Paola Jaramillo Ortega y Juan Manuel Rivera florez
 * @year 2025
*/

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"

#define PIN_PWM             15   ///< Salida PWM al L298
#define PIN_ENCODER         14   ///< Entrada encoder óptico
#define IN1                 16   ///< L298 IN1
#define IN2                 17   ///< L298 IN2

#define PULSOS_POR_VUELTA   20    ///< Ranuras en el disco
#define TIEMPO_MUESTREO_MS  4   ///< Reporte cada 4 ms
#define TIEMPO_ESCALON_MS   2000 ///< Cambio de duty cycle cada 2 segundos
#define MAX_MUESTRAS        20000 ///< Máximo de muestras a almacenar

#define FREQ_PWM            20000 ///< Frecuencia del PWM en Hz
#define WRAP                100   ///< Valor de wrap del PWM

#ifndef SYS_CLK_KHZ
  #define SYS_CLK_KHZ       125000
#endif

/*@brief Estructura para almacenar las muestras de tiempo, PWM y RPM.
 * @details Esta estructura se utiliza para almacenar cada muestra tomada durante el proceso de caracterización del motor.
 * @var tiempo_ms Tiempo transcurrido desde el inicio del proceso de caracterización en milisegundos.
 * @var pwm Valor del duty cycle del PWM aplicado al motor.
 * @var rpm Velocidad del motor medida en revoluciones por minuto (RPM).
*/
typedef struct {
    uint32_t tiempo_ms;
    uint8_t pwm;
    float rpm;
} muestra_t;

muestra_t buffer[MAX_MUESTRAS];
uint32_t indice = 0;

static volatile uint32_t contador_pulsos = 0;

/**
 * @brief Interrupción para contar los pulsos del encoder.
 * @details Esta función se llama cada vez que se detecta un flanco ascendente en el pin del encoder, incrementando el contador de pulsos.
 * @param gpio GPIO del encoder.
 * @param events Eventos de interrupción.
 */
void encoder_isr(uint gpio, uint32_t events) {
    contador_pulsos++;
}

/**
 * @brief Función principal del programa.
 * @details Configura el GPIO, PWM y el encoder, y ejecuta el bucle principal para capturar la curva de reacción del motor.
 * @return 0 al finalizar correctamente.
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
        // Muestreo cada TIEMPO_MUESTREO_MS ms
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
