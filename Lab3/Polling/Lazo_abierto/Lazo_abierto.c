/* * @file Lazo_abierto.c
 * @brief Programa para controlar un motor DC usando PWM en la Raspberry Pi Pico.
 *
 * Este programa implementa el control de un motor DC mediante una señal PWM generada por la Raspberry Pi Pico.
 * Permite variar el ciclo de trabajo (duty cycle) de la señal PWM entre 0% y 100% con una resolución mínima del 1%.
 *
 * @authors Angie Paola Jaramillo Ortega y Juan Manuel Rivera Florez
 * @year 2025
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"

#define PWM_OUT_PIN 15  ///< Pin de salida del PWM
#define IN1 16          ///< Pin de control del motor (IN1)
#define IN2 17          ///< Pin de control del motor (IN2)

#define FREQ 10000      ///< Frecuencia del PWM en Hz
#define TIEMPO_ESCALON_MS 1000  ///< Tiempo de escalón en milisegundos         

#ifndef SYS_CLK_KHZ
    #define SYS_CLK_KHZ 125000  // 125 MHz
#endif
#define WRAP 100 ///< Valor de wrap para el PWM (100 para 1% de resolución)


/**
 * @brief Programa principal que configura y controla un PWM para un motor DC.
 * 
 * Este programa inicializa el PWM en un pin específico, configura los pines de control del motor,
 * y ajusta el duty cycle del PWM en un bucle, incrementando el duty cycle cada segundo.
 * 
 * @return int Retorna 0 al finalizar.
 */
int main() 
{
    stdio_init_all();

    gpio_init(IN1);
    gpio_set_dir(IN1, GPIO_OUT);
    gpio_put(IN1, 1);

    gpio_init(IN2);
    gpio_set_dir(IN2, GPIO_OUT);
    gpio_put(IN2, 0);

    // Obtener el slice y canal PWM asociados al pin
    gpio_set_function(PWM_OUT_PIN, GPIO_FUNC_PWM);
    uint8_t slice_num = pwm_gpio_to_slice_num(PWM_OUT_PIN);
    uint8_t channel_num = pwm_gpio_to_channel(PWM_OUT_PIN);

    // Calcular el divisor del reloj para lograr la frecuencia deseada
    float divider = (SYS_CLK_KHZ * 1000.0f) / (FREQ * WRAP);
    if (divider < 1.0f) divider = 1.0f;
    if (divider > 255.0f) divider = 255.0f;

    // Configuración del PWM
    pwm_config cfg = pwm_get_default_config();
    pwm_config_set_clkdiv(&cfg, divider);
    pwm_config_set_wrap(&cfg, WRAP);
    pwm_init(slice_num, &cfg, false);
    pwm_set_enabled(slice_num, true);
    
    int duty = 0;
    absolute_time_t tiempo_anterior = get_absolute_time();  ///< Tiempo anterior para el escalonamiento

    // Bucle principal que incrementa el duty cycle del PWM
    while(1){
        // Incrementar el duty cycle del PWM de 0% a 100% en pasos del 1%
        while (duty <= 100) {
            absolute_time_t ahora = get_absolute_time();
            if (absolute_time_diff_us(tiempo_anterior, ahora) >= TIEMPO_ESCALON_MS * 1000) {
                pwm_set_chan_level(slice_num, channel_num, duty);  // aplicar nuevo duty
                printf("DUTY = %d%%\n", duty);              // mostrar por consola
                duty+=1;
                tiempo_anterior = ahora;                  // reiniciar temporizador
            }
        }
        duty= 0;
    }
    return 0;
}
