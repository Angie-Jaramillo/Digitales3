#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"

#define PIN_PWM 15
#define IN1 16
#define IN2 17

#define PWM_WRAP 100
#define TIEMPO_ENTRE_ESCALONES_MS 5000

int main() {
    stdio_init_all();

    // Configurar pines de dirección del L298
    gpio_init(IN1);
    gpio_set_dir(IN1, GPIO_OUT);
    gpio_put(IN1, 1);  // HIGH

    gpio_init(IN2);
    gpio_set_dir(IN2, GPIO_OUT);
    gpio_put(IN2, 0);  // LOW

    // Configurar PWM
    gpio_set_function(PIN_PWM, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(PIN_PWM);

    pwm_set_wrap(slice, PWM_WRAP);
    pwm_set_clkdiv(slice, 1.0f);
    pwm_set_enabled(slice, true);

    printf("Iniciando prueba de lazo abierto (PWM de 0%% a 100%%)...\n");

    for (int duty = 100; duty >= 0; duty-=10) {
        pwm_set_chan_level(slice, PWM_CHAN_A, duty);
        printf("DUTY = %d%%\n", duty);
        sleep_ms(TIEMPO_ENTRE_ESCALONES_MS);
    }

    printf("Fin de la prueba.");

    while (1) {
        tight_loop_contents();
    }

    return 0;
}
