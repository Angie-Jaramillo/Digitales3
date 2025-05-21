/* 
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/timer.h"

#define PIN_PWM                     15 // Pin de salida de PWM
#define PIN_ENC                     14 // Pin del encoder
#define PULSOS_POR_VUELTA           20 // Pulsos por vuelta del encoder
#define FREQ_MUESTREO_HZ 250
#define INTERVALO_ESCALON_MS 2000
#define MAX_DATOS   20000

typedef struct {
    uint32_t tiempo_ms;
    uint8_t pwm;
    uint32_t rpm;
} muestra_t;

muestra_t buffer_datos[MAX_DATOS];
volatile uint32_t contador_pulsos = 0;
uint32_t idx_datos = 0;

enum Estado { WAIT, MANUAL, CAPTURA };
enum Estado estado = WAIT;

bool flanco_anterior = 0;

uint pwm_actual = 0;

int main() {
    stdio_init_all();
    gpio_init(PIN_ENC);
    gpio_set_dir(PIN_ENC, GPIO_IN);
    gpio_pull_up(PIN_ENC);

    gpio_set_function(PIN_PWM, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(PIN_PWM);
    pwm_set_wrap(slice, 100);
    pwm_set_enabled(slice, true);

    absolute_time_t t_muestreo = get_absolute_time();
    absolute_time_t t_envio = get_absolute_time();
    absolute_time_t t_escala = get_absolute_time();

    char comando[32];
    int cmd_i = 0;

    while (1) {
        // ---------------------------
        // Lectura del encoder (polling)
        // ---------------------------
/*         bool flanco_actual = gpio_get(PIN_ENC);
        if (flanco_actual && !flanco_anterior) {
            contador_pulsos++;
        }
        flanco_anterior = flanco_actual; */

        // ---------------------------
        // Lectura del comando serial (polling)
        // ---------------------------
/*         if (stdio_usb_connected()) {
            int c = getchar_timeout_us(0);
            if (c != PICO_ERROR_TIMEOUT) {
                if (c == '\r' || c == '\n') {
                    printf("Comando recibido: %s\n", comando);
/*                     comando[cmd_i] = '\0'; */

                    // ---------------------------
                    // Comando START
                    // ---------------------------
/*                     if (strncmp(comando, "START ", 6) == 0) {
                        int paso = atoi(&comando[6]);
                        if (paso > 0 && paso <= 100) {
                            // Iniciar captura
                        }
                    } */
                    // ---------------------------
                    // Comando PWM
                    // ---------------------------
/*                     else if (strncmp(comando, "PWM ", 4) == 0) {
                        int val = atoi(&comando[4]);
                        if (val >= 0 && val <= 100) {
                            pwm_actual = val;
                            pwm_set_chan_level(slice, PWM_CHAN_A, pwm_actual);
                        }
                    }

                    cmd_i = 0;
                } else if (cmd_i < (int)(sizeof(comando) - 1)) {
                    comando[cmd_i++] = (char)c;
                }
            }
        }
 */
        // ---------------------------
        // Enviar PWM y RPM cada 500 ms si no está capturando
        // ---------------------------
/*         if (!capturando && absolute_time_diff_us(t_envio, get_absolute_time()) >= 500000) {
            uint32_t pulsos = contador_pulsos;
            contador_pulsos = 0;
            uint32_t rpm = (pulsos * 1000 * 60) / (PULSOS_POR_VUELTA * 500);
            printf("PWM=%u  RPM=%u\n", pwm_actual, rpm);
            t_envio = get_absolute_time();
        } 
    }

    return 0;
}
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"

#define PIN_PWM     15
#define PIN_ENCODER 14
#define IN1         16
#define IN2         17

#define CPR                 20
#define FREQ_PWM            10000
#define WRAP                100
#define TIEMPO_ESCALON_MS   2000
#define TIEMPO_MUESTREO_MS  50
#define MAX_MUESTRAS        5000
#define MAX_COMANDO         32

#ifndef SYS_CLK_KHZ
    #define SYS_CLK_KHZ     125000
#endif

typedef struct {
    uint32_t tiempo_ms;
    uint8_t pwm;
    float rpm;
} muestra_t;

typedef enum { WAIT, MANUAL, CAPTURA } estado_t;

muestra_t buffer[MAX_MUESTRAS];
uint32_t indice = 0;

int main() {
    stdio_init_all();

    // Pines L298N
    gpio_init(IN1); gpio_set_dir(IN1, GPIO_OUT); gpio_put(IN1, 1);
    gpio_init(IN2); gpio_set_dir(IN2, GPIO_OUT); gpio_put(IN2, 0);

    // PWM
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

    // Encoder
    gpio_init(PIN_ENCODER); gpio_set_dir(PIN_ENCODER, GPIO_IN); gpio_pull_up(PIN_ENCODER);
    bool flanco_anterior = gpio_get(PIN_ENCODER);
    volatile uint32_t pulsos = 0;

    // Variables del sistema
    estado_t estado = WAIT;
    int pwm_actual = 0;
    int paso = 20;
    int escalon_actual = 0;
    int total_escalones = 0;
    bool bajando = false;

    absolute_time_t t_inicio, t_muestreo, t_escalon, t_reporte;

    char comando[MAX_COMANDO];
    int cmd_i = 0;

    pwm_set_chan_level(slice, chan, 0);

    while (1) {
        // Polling del encoder
        bool flanco_actual = gpio_get(PIN_ENCODER);
        if (flanco_actual && !flanco_anterior) pulsos++;
        flanco_anterior = flanco_actual;

        // Leer comandos por USB
        if (stdio_usb_connected()) {
            int c = getchar_timeout_us(0);
            if (c != PICO_ERROR_TIMEOUT) {
                if (c == '\r' || c == '\n') {
                    comando[cmd_i] = '\0';
                    cmd_i = 0;

                    if (strncmp(comando, "START ", 6) == 0) {
                        paso = atoi(comando + 6);
                        if (paso < 1) paso = 1;
                        if (paso > 100) paso = 100;
                        int subida = (100 / paso) + 1;
                        int bajada = subida - 1;
                        total_escalones = subida + bajada;
                        pwm_actual = 0;
                        escalon_actual = 0;
                        bajando = false;
                        indice = 0;
                        pwm_set_chan_level(slice, chan, 0);
                        t_inicio = get_absolute_time();
                        t_muestreo = t_inicio;
                        t_escalon = t_inicio;
                        estado = CAPTURA;
                        printf("tiempo_ms,pwm,rpm\n");
                    }
                    else if (strncmp(comando, "PWM ", 4) == 0) {
                        pwm_actual = atoi(comando + 4);
                        if (pwm_actual < 0) pwm_actual = 0;
                        if (pwm_actual > 100) pwm_actual = 100;
                        pwm_set_chan_level(slice, chan, pwm_actual);
                        t_muestreo = get_absolute_time();
                        t_reporte = t_muestreo;
                        estado = MANUAL;
                    }
                } else if (cmd_i < MAX_COMANDO - 1) {
                    comando[cmd_i++] = (char)c;
                }
            }
        }

        absolute_time_t t_ahora = get_absolute_time();

        // Muestreo cada 50 ms
        if (absolute_time_diff_us(t_muestreo, t_ahora) >= TIEMPO_MUESTREO_MS * 1000) {
            float delta_ms = absolute_time_diff_us(t_muestreo, t_ahora) / 1000.0f;
            float rpm = ((float)pulsos / CPR) * (60000.0f / delta_ms);
            pulsos = 0;

            if (estado == CAPTURA && indice < MAX_MUESTRAS) {
                buffer[indice].tiempo_ms = to_ms_since_boot(t_ahora) - to_ms_since_boot(t_inicio);
                buffer[indice].pwm = pwm_actual;
                buffer[indice].rpm = rpm;
                indice++;
            }
            else if (estado == MANUAL && absolute_time_diff_us(t_reporte, t_ahora) >= 500000) {
                printf("PWM=%d, RPM=%.2f\n", pwm_actual, rpm);
                t_reporte = t_ahora;
            }

            t_muestreo = t_ahora;
        }

        // Escalón cada 2 s en modo CAPTURA
        if (estado == CAPTURA && absolute_time_diff_us(t_escalon, t_ahora) >= TIEMPO_ESCALON_MS * 1000) {
            escalon_actual++;
            if (!bajando)
                pwm_actual = escalon_actual * paso;
            else
                pwm_actual = (total_escalones - escalon_actual) * paso;

            if (pwm_actual >= 100) {
                pwm_actual = 100;
                bajando = true;
            }
            if (pwm_actual <= 0 && bajando) {
                pwm_actual = 0;
                pwm_set_chan_level(slice, chan, 0);
                for (uint32_t i = 0; i < indice; i++) {
                    printf("%u,%u,%.2f\n", buffer[i].tiempo_ms, buffer[i].pwm, buffer[i].rpm);
                }
                estado = WAIT;
                continue;
            }

            pwm_set_chan_level(slice, chan, pwm_actual);
            t_escalon = t_ahora;
        }

        tight_loop_contents();
    }

    return 0;
}
