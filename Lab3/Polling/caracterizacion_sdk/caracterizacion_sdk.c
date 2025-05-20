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
        if (stdio_usb_connected()) {
            int c = getchar_timeout_us(0);
            if (c != PICO_ERROR_TIMEOUT) {
                if (c == '\r' || c == '\n') {
                    printf("Comando recibido: %s\n", comando);
/*                     comando[cmd_i] = '\0'; */

                    // ---------------------------
                    // Comando START
                    // ---------------------------
                    if (strncmp(comando, "START ", 6) == 0) {
                        int paso = atoi(&comando[6]);
                        if (paso > 0 && paso <= 100) {
                            // Iniciar captura
                        }
                    }
                    // ---------------------------
                    // Comando PWM
                    // ---------------------------
                    else if (strncmp(comando, "PWM ", 4) == 0) {
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

        // ---------------------------
        // Enviar PWM y RPM cada 500 ms si no está capturando
        // ---------------------------
/*         if (!capturando && absolute_time_diff_us(t_envio, get_absolute_time()) >= 500000) {
            uint32_t pulsos = contador_pulsos;
            contador_pulsos = 0;
            uint32_t rpm = (pulsos * 1000 * 60) / (PULSOS_POR_VUELTA * 500);
            printf("PWM=%u  RPM=%u\n", pwm_actual, rpm);
            t_envio = get_absolute_time();
        } */
    }

    return 0;
}
