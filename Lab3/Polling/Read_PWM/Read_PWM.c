/*
 * @file Read_PWM.c
 * @brief Programa para medir RPM utilizando un encoder óptico en Raspberry Pi Pico.
 * @details
 * Este programa utiliza polling para leer los pulsos de un encoder óptico conectado a un pin GPIO de la Raspberry Pi Pico. 
 * Calcula las revoluciones por minuto (RPM) basándose en el número de pulsos detectados en un intervalo de tiempo fijo.
 * El programa está diseñado para funcionar con un encoder óptico que tiene un número fijo de ranuras en su disco,
 * lo que permite calcular las RPM de un motor DC conectado al encoder.
 * @author Angie Paola jaramillo Ortega y Juan Manuel Rivera Florez
 * @year 2025
 */

#include <stdio.h>
#include "pico/stdlib.h"

#define PIN_ENCODER 14       ///< Pin del encoder óptico
#define PULSOS_POR_VUELTA 20 ///< Número de ranuras en el disco
#define INTERVALO_MS 500     ///< Intervalo de cálculo de RPM (500 ms)


/**
 * @brief Función principal que configura el GPIO y mide las RPM del motor DC.
 * 
 * Esta función inicializa el pin del encoder, lee los pulsos en un bucle infinito,
 * calcula las RPM cada INTERVALO_MS y muestra el resultado por consola.
 */
int main() {
    stdio_init_all();

    gpio_init(PIN_ENCODER);
    gpio_set_dir(PIN_ENCODER, GPIO_IN);
    gpio_pull_up(PIN_ENCODER);

    uint32_t contador_pulsos = 0;
    bool flanco_anterior = false;

    absolute_time_t tiempo_anterior = get_absolute_time();

    // Bucle infinito para medir RPM
    while (1) {
        // Leer el estado actual del pin del encoder
        bool flanco_actual = gpio_get(PIN_ENCODER);

        // Detectar flanco de subida (LOW -> HIGH)
        if (flanco_actual && !flanco_anterior) {
            contador_pulsos++;
        }

        // Guardar el estado anterior
        flanco_anterior = flanco_actual;

        // Calcular RPM cada INTERVALO_MS
        absolute_time_t ahora = get_absolute_time();
        if (absolute_time_diff_us(tiempo_anterior, ahora) >= INTERVALO_MS * 1000) {
            // Calcular RPM
            float rpm = (contador_pulsos * 1000.0f / INTERVALO_MS) * (60.0f / PULSOS_POR_VUELTA);

            // Mostrar por consola
            printf("RPM: %.2f\n", contador_pulsos, rpm);

            // Reiniciar contador y tiempo
            contador_pulsos = 0;
            tiempo_anterior = ahora;
        }
    }

    return 0;
}
