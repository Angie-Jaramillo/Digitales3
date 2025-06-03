/**
 * @file lazo_abierto.c
 * @author Juan Manuel Rivera Florez y Angie Paola Jaramillo
 * @brief 
 * @version 0.1
 * @date 2025-06-02
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"

const uint8_t encoder_pin = 14;
const uint8_t Enable_motor_pin = 15;
const uint8_t IN1_pin = 16;
const uint8_t IN2_pin = 17;

volatile uint32_t counter = 0;
volatile uint16_t rpm = 0;
uint8_t ref = 0, value = 0;
int tiempo = 0;

char comando[32];
int cmd_i = 0;

struct repeating_timer timer_rpm;
struct repeating_timer change_ref;

/**
 * @brief callback function for the sample timer. este timer se usa para calcular la RPM del motor.
 * 
 * @param t timer structure pointer
 * @note This function is called periodically to calculate the RPM based on the encoder pulses.
 * @return true sigue ejecutando el timer
 * @return false deja de ejecutar el timer
 */
bool sample_timer_callback(struct repeating_timer *t);
/**
 * @brief callback function for the reference timer. este timer se usa para cambiar la referencia del motor.
 * 
 * @param t timer structure pointer
 * @note This function is called periodically to update the reference value for the motor.
 * @return true sigue ejecutando el timer
 * @return false deja de ejecutar el timer
 */
bool ref_timer_callback(struct repeating_timer *t);
/**
 * @brief Callback function for the encoder pin interrupt.
 * 
 * @param gpio GPIO pin number
 * @param events Events that triggered the interrupt
 * @note This function increments the counter each time an edge is detected on the encoder pin.
 */
void encoder_callback(uint gpio, uint32_t events);
/**
 * @brief Moves the motor based on the given value.
 * 
 * @param u Value to set the motor speed (0-6250 for forward, -6250 to 0 for backward)
 * @note This function sets the PWM levels for the motor control pins.
 */
void move(uint16_t u);
/**
 * @brief Moves the motor forward with the specified speed.
 * 
 * @param u Speed value (0-6250)
 * @note This function sets the PWM level for the IN1 pin to move the motor forward.
 */
void forward(uint16_t u);
/**
 * @brief Moves the motor backward with the specified speed.
 * 
 * @param u Speed value (0-6250)
 * @note This function sets the PWM level for the IN2 pin to move the motor backward.
 */
void backward(uint16_t u);
/**
 * @brief Starts the motor control process with the specified value.
 * 
 * @param valor Value to set the reference for the motor (0-100)
 * @note This function initializes the motor control, sets the reference, and starts the timers.
 */
void Start(uint16_t valor);
/**
 * @brief Converts a reference value (0-100) to a PWM value (0-6250).
 * 
 * @param ref Reference value (0-100)
 * @return int PWM value (0-6250)
 * @note This function scales the reference value to the PWM range.
 */
int reftoPWM(uint16_t ref);

int main()
{
    stdio_init_all();

    gpio_init(encoder_pin);
    gpio_set_irq_enabled_with_callback(encoder_pin, GPIO_IRQ_EDGE_RISE, true, &encoder_callback);

    
    gpio_init(Enable_motor_pin);
    gpio_set_dir(Enable_motor_pin, GPIO_OUT);
    gpio_put(Enable_motor_pin, 1);

    gpio_set_function(IN1_pin, GPIO_FUNC_PWM);
    gpio_set_function(IN2_pin, GPIO_FUNC_PWM);

    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 1.0f);
    pwm_config_set_wrap(&config, 6250);

    uint slice_num = pwm_gpio_to_slice_num(IN1_pin);

    pwm_init(slice_num, &config, true);
    pwm_set_gpio_level(IN1_pin, 0);
    pwm_set_gpio_level(IN2_pin, 0);

    char input[32];

    while (true) {
        if (stdio_usb_connected()) {
            int c = getchar_timeout_us(0); // Lectura del comando serial
            if (c != PICO_ERROR_TIMEOUT) {
                if (c == '\r' || c == '\n') {
                    // printf("Comando recibido: %s\n", comando);
                     comando[cmd_i] = '\0';

                     if (strncmp(comando, "START ", 6) == 0) { // Comando START
                        int paso = atoi(&comando[6]);
                        // printf("The number is: %d\n", paso);
                         if (paso > 0 && paso <= 100) {
                            // Iniciar captura
                            Start(paso);
                        } 
                    }
                cmd_i = 0;
                } else if (cmd_i < (int)(sizeof(comando) - 1)) {
                    comando[cmd_i++] = (char)c;
                }
            }
        }
    }
}

bool sample_timer_callback(struct repeating_timer *t) {

    if(t == &timer_rpm) {
        // Calcular RPM
        uint32_t current_time = time_us_32() / 1000; // Convertir a milisegundos
        rpm = (counter * 60) / (20*0.1); // RPM = (pulsos por minuto) / 20 
        //printf("%u\n", counter);
        counter = 0; // Reiniciar contador

        printf("%u %u %u\n", time_us_32()/1000, value, rpm);
        
    }
    
    return true; // Keep the timer running
}

void encoder_callback(uint gpio, uint32_t events) {
    if(events == GPIO_IRQ_EDGE_RISE) {
        counter++;
    }
}

void move(uint16_t u)
{
    if (u> 6250) {
        u = 6250;
    } else if (u < -6250) {
        u = -6250;
    }

    if (u > 0) {
        forward(u);
    } else if (u < 0) {
        backward(-u);
    } else {
        pwm_set_gpio_level(IN1_pin, 0);
        pwm_set_gpio_level(IN2_pin, 0);
    }
}

void forward(uint16_t u)
{
    pwm_set_gpio_level(IN2_pin, 0);
    pwm_set_gpio_level(IN1_pin, u);
}

void backward(uint16_t u)
{
    pwm_set_gpio_level(IN1_pin, 0);
    pwm_set_gpio_level(IN2_pin, u);
}

void Start(uint16_t valor)
{
    // printf("Iniciando START:\n");
    move(reftoPWM(0));

    value = valor;

    add_repeating_timer_ms(-100, sample_timer_callback, NULL, &timer_rpm);
 
    add_repeating_timer_ms(-3000, ref_timer_callback, NULL, &change_ref);

    while (ref<=100);

    bool cancelled = cancel_repeating_timer(&timer_rpm);
    cancelled = cancel_repeating_timer(&change_ref);
    move(0);
    
    ref = 0;
}

int reftoPWM(uint16_t ref)
{
    int x = (ref*6250)/100;
    return x;
}

bool ref_timer_callback(struct repeating_timer *t){
    ref = ref + value;
    if (ref >= 100)
    {
        ref = 100;
        move(reftoPWM(100));
    }else if (ref <= 0)
    {
        ref = 0;
        move(reftoPWM(0));
    }
    else{
        move(reftoPWM(ref));
    }
    //printf("Se cambio la ref a:%d\n", ref);
}
