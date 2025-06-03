/*
Implementar el sistema de caracterización del motor DC de tal forma que permita medir en todo instante la 
velocidad en RPM del motor y el PWM aplicado al mismo, adicionalmente este sistema permite capturar la curva 
de reacción del motor. Para controlar el sistema es necesario desarrollar una interfaz de comandos a través una 
terminal. Este sistema debe implementarse usando las 3 estrategias clásicas de control de flujo: polling, 
interrupciones y polling+interrupciones. 
• Implemente una interfaz serial con dos comandos: 
1. START <valor>: Inicia automáticamente la captura de la curva experimental, almacenando temporalmente 
los datos (MARCA-TEMPORAL, PWM y RPM) en un buffer en memoria. Los datos se envían al PC al finalizar 
la secuencia de escalones ascendentes y descendentes. El parámetro valor indica la magnitud del cambio 
del PWM durante la secuencia de captura. Tenga en cuenta que valor no es un divisor exacto de 100, el 
valor máximo del PWM no puede superar 100.  
2. PWM <valor>: Cuando el sistema no este efectuando ninguna captura, es posible ajustar manualmente el 
ciclo de dureza del PWM. El parámetro valor indica el ciclo de dureza que debe generar el MCU para 
aplicar al motor. Mientras el sistema está en este modo de ejecución (NO CAPTURA), el MCU envía 
continuamente al PC los valores actuales de PWM y RPM con una frecuencia de 2Hz. 
• Documentar el código de los 3 programas en Doxygen. 
• Comparar el rendimiento de las 3 estrategias entre sí y con respecto a los resultados obtenidos con Arduino y 
microPython en la práctica 2.
*/

/*@file caracterizacion_sdk.c
 * @brief Programa para caracterizar un motor DC usando PWM y un encoder.
 * 
 * Este programa implementa un sistema de caracterización del motor DC que permite medir la velocidad
 * en RPM y el PWM aplicado al motor. Además, permite capturar la curva de reacción del motor.
 * Usa dos comandos a través de una interfaz serial: START <valor> para iniciar la captura y PWM <valor> para 
 * ajustar manualmente el ciclo de trabajo del PWM.
 * El sistema utiliza polling para la lectura del encoder y la recepción de comandos. Los datos se almacenan 
 * en un buffer y se envían al PC al finalizar la captura.
 * 
 * @author Angie Paola jaramillo Ortega y Juan Manuel River Flores
 * @year 2025
 *
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/timer.h"

#define PIN_PWM                     15 ///< Pin de salida de PWM
#define PIN_ENC                     14 ///< Pin del encoder
#define IN1                         16 ///< Pin de dirección 1 (L298N)
#define IN2                         17 ///< Pin de dirección 2 (L298N)
#define FREQ_PWM                    10000 ///< Frecuencia del PWM en Hz
#define WRAP                        100 ///< Valor de wrap del PWM (100 para 10% de resolución)
#define PULSOS_POR_VUELTA           20 ///< Pulsos por vuelta del encoder
#define TIEMPO_MUESTREO_MS          50 ///< Tiempo de muestreo en milisegundos
#define FREQ_MUESTREO_HZ            250 ///< Frecuencia de muestreo en Hz (4 ms)
#define INTERVALO_ESCALON_MS        2000 ///< Intervalo entre escalones en milisegundos
#define MAX_DATOS                   20000 ///< Máximo número de datos a capturar

#ifndef SYS_CLK_KHZ
    #define SYS_CLK_KHZ             125000  // 125 MHz
#endif

/**@brief Estructura para almacenar los datos de muestreo del motor. 
 * @param tiempo_ms Tiempo en milisegundos desde el inicio de la captura.
 * @param pwm Valor del PWM aplicado al motor (0-100).
 * @param rpm Velocidad del motor en revoluciones por minuto (RPM).
*/
typedef struct {
    uint32_t    tiempo_ms;
    uint8_t     pwm;
    float    rpm;
} muestra_t;

/**@brief Estados del sistema de caracterización.
 * WAIT: Esperando un comando.
 * MANUAL: En modo manual, ajustando el PWM.
 * CAPTURA: En modo captura, recolectando datos del motor.
 */
typedef enum { WAIT, MANUAL, CAPTURA } estado_t;

muestra_t buffer[MAX_DATOS];
uint32_t indice = 0;

/**@brief Función principal del programa.
 * Configura los pines, PWM y el encoder, y gestiona la captura de datos del motor.
 * Permite iniciar la captura con el comando START <valor> y ajustar el PWM con PWM <valor>.
 * Los datos se envían al PC al finalizar la captura o en modo manual.
 */
int main() {
    stdio_init_all();

    //Configurar pin del encoder
    gpio_init(PIN_ENC);
    gpio_set_dir(PIN_ENC, GPIO_IN);
    gpio_pull_up(PIN_ENC);

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

    //Variables del sistema
    absolute_time_t t_inicio;
    absolute_time_t t_muestreo = get_absolute_time();
    absolute_time_t t_reporte = get_absolute_time();
    absolute_time_t t_escalon = get_absolute_time();
    
    estado_t estado = WAIT;
    bool flanco_anterior = 0;
    uint escalon_actual = 0;
    uint8_t pwm_actual = 0;
    uint8_t paso = 0;
    uint total_escalones = 0;
    bool bajando = false;

    volatile uint32_t contador_pulsos = 0;

    char comando[32];
    int cmd_i = 0;

    
    while (1) {

        bool flanco_actual = gpio_get(PIN_ENC);
        if (flanco_actual && !flanco_anterior) { //Lectura del encoder
            contador_pulsos++;
        }
        flanco_anterior = flanco_actual;
        // Lectura del comando serial
        if (stdio_usb_connected()) {
            int c = getchar_timeout_us(0); 
            if (c != PICO_ERROR_TIMEOUT) {
                if (c == '\r' || c == '\n') {
                    printf("Comando recibido: %s\n", comando);
                     comando[cmd_i] = '\0';

                    if (strncmp(comando, "START ", 6) == 0) { // Comando START
                        paso = atoi(&comando[6]);
                        printf("The number is: %d\n", paso);
                        if (paso > 0 && paso <= 100) {
                            // Iniciar captura
                            int subida = (100 / paso) + 1;
                            int bajada = subida - 1;
                            total_escalones = subida + bajada;
                            escalon_actual = 0;
                            pwm_actual = 0;
                            bajando = false;
                            indice = 0;
                            contador_pulsos = 0;
                            t_inicio = get_absolute_time();
                            t_escalon = get_absolute_time();
                            t_muestreo = get_absolute_time();
                            pwm_set_chan_level(slice, chan, 0);
                            estado = CAPTURA;
                            printf("tiempo_ms,pwm,rpm\n");
                        }
                    }
                     else if (strncmp(comando, "PWM ", 4) == 0) { // Comando PWM
                        int val = atoi(&comando[4]);
                        printf("The number is: %d\n", val);
                         if (val >= 0 && val <= 100) {
                            pwm_actual = val;
                            pwm_set_chan_level(slice, chan, pwm_actual);
                            t_reporte = get_absolute_time();
                            estado = MANUAL;
                        } 
                    }
                cmd_i = 0;
                } else if (cmd_i < (int)(sizeof(comando) - 1)) {
                    comando[cmd_i++] = (char)c;
                }
            }
        }
        absolute_time_t t_actual = get_absolute_time();

        //Muestreo RPM cada 50 ms
        int64_t delta_us = absolute_time_diff_us(t_muestreo, t_actual);
        if (delta_us >= TIEMPO_MUESTREO_MS * 1000) {
            float delta_ms = delta_us / 1000.0f;
            float rpm = ((float)contador_pulsos / PULSOS_POR_VUELTA) * (60000.0f / delta_ms);
            contador_pulsos = 0;

            if (estado == CAPTURA) {
                buffer[indice].tiempo_ms = absolute_time_diff_us(t_inicio, t_actual) / 1000;
                buffer[indice].pwm = pwm_actual;
                buffer[indice].rpm = rpm;
                indice++;
            }
            else if (estado == MANUAL && absolute_time_diff_us(t_reporte, t_actual) >= 500000) {
                printf("PWM = %d, RPM = %.2f\n", pwm_actual, rpm);
                t_reporte = t_actual;
            }
            t_muestreo = t_actual;
        }
        //Cambio de escalón cada 2 s
        if (estado == CAPTURA && absolute_time_diff_us(t_escalon, t_actual) >= INTERVALO_ESCALON_MS * 1000) {
            escalon_actual++;
            if (escalon_actual < total_escalones) {
                if (escalon_actual <= 100 / paso) {
                    pwm_actual = escalon_actual * paso;
                } else {
                    pwm_actual = (total_escalones - escalon_actual) * paso;
                }
                if (pwm_actual > 100) pwm_actual = 100;
                if (pwm_actual < 0) pwm_actual = 0;
                pwm_set_chan_level(slice, chan, pwm_actual);
                t_escalon = t_actual;
            }
            else {
                pwm_set_chan_level(slice, chan, 0);
                estado = WAIT;
                for (uint32_t i = 0; i < indice; i++) {
                    printf("%u,%u,%.2f\n", buffer[i].tiempo_ms, buffer[i].pwm, buffer[i].rpm);
                }
            }
        }
    }
    return 0;
}