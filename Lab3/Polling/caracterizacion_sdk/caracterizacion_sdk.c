
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/timer.h"

#define PIN_PWM                     15 // Pin de salida de PWM
#define PIN_ENC                     14 // Pin del encoder
#define IN1                         16
#define IN2                         17
#define FREQ_PWM                    10000
#define WRAP                        100
#define PULSOS_POR_VUELTA           20 // Pulsos por vuelta del encoder
#define FREQ_MUESTREO_HZ            250
#define INTERVALO_ESCALON_MS        2000
#define MAX_DATOS                   20000

#ifndef SYS_CLK_KHZ
    #define SYS_CLK_KHZ             125000  // 125 MHz
#endif

typedef struct {
    uint32_t    tiempo_ms;
    uint8_t     pwm;
    uint32_t    rpm;
} muestra_t;

typedef enum { WAIT, MANUAL, CAPTURA } estado_t;

muestra_t buffer[MAX_DATOS];
uint32_t indice = 0;

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
    absolute_time_t t_muestreo = get_absolute_time();
    //absolute_time_t t_envio = get_absolute_time();
    absolute_time_t t_escalon = get_absolute_time();
    
    estado_t estado = WAIT;
    bool flanco_anterior = 0;
    uint escalon_actual = 0;
    uint8_t pwm_actual = 0;
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

        if (stdio_usb_connected()) {
            int c = getchar_timeout_us(0); // Lectura del comando serial
            if (c != PICO_ERROR_TIMEOUT) {
                if (c == '\r' || c == '\n') {
                    printf("Comando recibido: %s\n", comando);
                     comando[cmd_i] = '\0';

                     if (strncmp(comando, "START ", 6) == 0) { // Comando START
                        int paso = atoi(&comando[6]);
                        printf("The number is: %d\n", paso);
                         if (paso > 0 && paso <= 100) {
                            // Iniciar captura
                        } 
                    }
                     else if (strncmp(comando, "PWM ", 4) == 0) { // Comando PWM
                        int val = atoi(&comando[4]);
                        printf("The number is: %d\n", val);
                         if (val >= 0 && val <= 100) {
                            printf("entró");
                            pwm_actual = val;
                            pwm_set_chan_level(slice, chan, pwm_actual);
                        } 
                    }

                cmd_i = 0;
                } else if (cmd_i < (int)(sizeof(comando) - 1)) {
                    comando[cmd_i++] = (char)c;
                }
            }
        }
        absolute_time_t t_actual = get_absolute_time();
        // Enviar PWM y RPM cada 500 ms si no está capturando
        if (estado == CAPTURA && absolute_time_diff_us(t_escalon, t_actual) >= INTERVALO_ESCALON_MS * 1000) {
            uint32_t pulsos = contador_pulsos;
            contador_pulsos = 0;
            uint32_t rpm = (pulsos * 1000 * 60) / (PULSOS_POR_VUELTA * 500);
            printf("PWM=%u  RPM=%u\n", pwm_actual, rpm);
            t_muestreo = get_absolute_time();
        }  
    }
    return 0;
}