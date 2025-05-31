#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/irq.h"
#include "hardware/timer.h"

#define PIN_PWM             15   // Salida PWM al L298
#define PIN_ENC             14   // Entrada encoder óptico
#define IN1                 16   // L298 IN1
#define IN2                 17   // L298 IN2

#define CPR                 20   // Pulsos por vuelta
#define FREQ_PWM            10000
#define WRAP                100
#define TIEMPO_MUESTREO_MS  50   // Ventana de muestreo exacta
#define TIEMPO_ESCALON_MS   2000 // Tiempo entre escalones
#define MAX_DATOS           20000
#define MAX_CMD_LEN         32

#ifndef SYS_CLK_KHZ
  #define SYS_CLK_KHZ       125000
#endif

typedef struct {
    uint32_t tiempo_ms;
    uint8_t  pwm;
    float    rpm;
} muestra_t;

// Buffer de muestras
static muestra_t buffer[MAX_DATOS];
static uint32_t  indice = 0;

// Flags y variables compartidas
static volatile uint32_t contador_pulsos = 0;
static volatile bool     flag_muestreo   = false;

// Estados
typedef enum { WAIT, MANUAL, CAPTURA } estado_t;
static estado_t estado = WAIT;

// Parámetros de curva
static uint8_t  escalon_porc   = 20;
static uint32_t total_escalones, escalon_actual;
static uint8_t  pwm_actual;
static absolute_time_t t_inicio, t_escalon, t_reporte;
bool imprimir_buffer = false;

// === ISR del encoder: cuenta flancos de subida ===
void encoder_isr(uint gpio, uint32_t events) {
    contador_pulsos++;
}

// === Callback de timer: dispara cada TIEMPO_MUESTREO_MS ===
bool muestreo_cb(alarm_id_t id, void *user_data) {
    flag_muestreo = true;

    add_alarm_in_ms(TIEMPO_MUESTREO_MS, muestreo_cb, NULL, true);
    return true;
}

int main() {
    stdio_init_all();

    gpio_init(PIN_ENC);
    gpio_set_dir(PIN_ENC, GPIO_IN);
    gpio_pull_up(PIN_ENC);
    gpio_set_irq_enabled_with_callback(PIN_ENC, GPIO_IRQ_EDGE_RISE, true, &encoder_isr);

    gpio_init(IN1); gpio_set_dir(IN1, GPIO_OUT); gpio_put(IN1, 1);
    gpio_init(IN2); gpio_set_dir(IN2, GPIO_OUT); gpio_put(IN2, 0);

    gpio_set_function(PIN_PWM, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(PIN_PWM);
    uint chan  = pwm_gpio_to_channel(PIN_PWM);
    float divider = (SYS_CLK_KHZ * 1000.0f) / (FREQ_PWM * WRAP);
    divider = fmaxf(1.0f, fminf(divider, 255.0f));
    pwm_config cfg = pwm_get_default_config();
    pwm_config_set_clkdiv(&cfg, divider);
    pwm_config_set_wrap(&cfg, WRAP);
    pwm_init(slice, &cfg, true);
    pwm_set_chan_level(slice, chan, 0);

    add_alarm_in_ms(TIEMPO_MUESTREO_MS, muestreo_cb, NULL, true);

    char comando[MAX_CMD_LEN];
    int cmd_i = 0;

    printf("Listo. Use START <paso> o PWM <valor>\n");

    while (1) {
        if (stdio_usb_connected()) {
            int c = getchar_timeout_us(0);
            if (c != PICO_ERROR_TIMEOUT) {
                if (c == '\r' || c == '\n') {
                    comando[cmd_i] = '\0';
                    // START <paso>
                    if (strncmp(comando, "START ", 6) == 0) {
                        escalon_porc   = atoi(comando + 6);
                        if (escalon_porc < 1) escalon_porc = 1;
                        if (escalon_porc > 100) escalon_porc = 100;
                        int subidas = (100 / escalon_porc) + 1;
                        total_escalones = subidas + (subidas - 1);
                        escalon_actual  = 0;
                        pwm_actual      = 0;
                        indice          = 0;
                        contador_pulsos = 0;
                        estado          = CAPTURA;
                        t_inicio        = get_absolute_time();
                        t_escalon       = get_absolute_time();
                        t_reporte       = get_absolute_time();
                        pwm_set_chan_level(slice, chan, 0);
                        printf("tiempo_ms,pwm,rpm\n");
                    }
                    // PWM <valor>
                    else if (strncmp(comando, "PWM ", 4) == 0) {
                        int v = atoi(comando + 4);
                        pwm_actual = (v < 0 ? 0 : (v > 100 ? 100 : v));
                        pwm_set_chan_level(slice, chan, pwm_actual);
                        estado    = MANUAL;
                        t_reporte = get_absolute_time();
                    }
                    cmd_i = 0;
                }
                else if (cmd_i < MAX_CMD_LEN - 1) {
                    comando[cmd_i++] = (char)c;
                }
            }
        }
        // Esperar comando START o PWM
        if (flag_muestreo) {
            flag_muestreo = false;
            // Leer y resetear el contador de ISR
            uint32_t p = contador_pulsos;
            contador_pulsos = 0;
            // Calcular RPM
            float rpm = ((float)p / CPR) * (60000.0f / TIEMPO_MUESTREO_MS);

            if (estado == CAPTURA && indice < MAX_DATOS) {
                uint32_t t_rel = absolute_time_diff_us(t_inicio, get_absolute_time()) / 1000;
                buffer[indice++] = (muestra_t){ t_rel, pwm_actual, rpm };
            }
            else if (estado == MANUAL) {
                // Reportar cada 500 ms (polling de tiempo en bucle)
                if (absolute_time_diff_us(t_reporte, get_absolute_time()) >= 500000) {
                    printf("PWM=%u, RPM=%.2f\n", pwm_actual, rpm);
                    t_reporte = get_absolute_time();
                }
            }
        }

        //Cambio de escalón cada TIEMPO_ESCALON_MS
        if (estado == CAPTURA && 
            absolute_time_diff_us(t_escalon, get_absolute_time()) >= TIEMPO_ESCALON_MS * 1000) {

            escalon_actual++;
            if (escalon_actual < total_escalones) {
                if (escalon_actual <= 100 / escalon_porc)
                    pwm_actual = escalon_actual * escalon_porc;
                else
                    pwm_actual = (total_escalones - escalon_actual) * escalon_porc;

                pwm_actual = (pwm_actual > 100 ? 100 : pwm_actual);
                pwm_set_chan_level(slice, chan, pwm_actual);
                t_escalon = get_absolute_time();
            }
            else {
                // Fin de curva: imprimir CSV
                printf("Fin de captura. Total muestras: %u\n", indice);
                pwm_set_chan_level(slice, chan, 0);
                for (uint32_t i = 0; i < indice; i++) {
                    printf("%u,%u,%.2f\n",
                        buffer[i].tiempo_ms,
                        buffer[i].pwm,
                        buffer[i].rpm
                    );
                }
                estado = WAIT;
            }
        }
    }

    return 0;
}