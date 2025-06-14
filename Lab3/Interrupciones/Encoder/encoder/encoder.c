/**
 * @file encoder.c
 * @author Juan Manuel Rivera y Angie Paola Jaramillo
 * @brief 
 * @version 0.1
 * @date 2025-06-02
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"

const uint8_t encoder_pin = 14;
const uint8_t Enable_motor_pin = 15;
const uint8_t IN1_pin = 16;
const uint8_t IN2_pin = 17;

volatile uint32_t counter = 0;
volatile uint16_t rpm = 0;
uint8_t ref = 0, value = 0;

struct repeating_timer timer_rpm;
/**
 * @brief Callback function for the encoder pin, se activa cuando hay un flanco de subida en el pin del encoder.
 * 
 * @param gpio pin del encoder
 * @param events flanco en el que se activa la interrupción
 */
void encoder_callback(uint gpio, uint32_t events);
/**
 * @brief Callback function for the timer, se activa cada 100 ms para calcular las RPM.
 * 
 * @param t puntero a la estructura del temporizador
 * @return true si el temporizador sigue activo
 * @return false si el temporizador se detiene
 */
bool sample_timer_callback(struct repeating_timer *t);
/**
 * @brief Mueve el motor a una velocidad determinada.
 * 
 * @param u velocidad del motor en PWM
 */
void move(uint16_t u);
/**
 * @brief Mueve el motor hacia adelante a una velocidad determinada.
 * 
 * @param u velocidad del motor en PWM
 */
void forward(uint16_t u);
/**
 * @brief Mueve el motor hacia atrás a una velocidad determinada.
 * 
 * @param u velocidad del motor en PWM
 */
void backward(uint16_t u);
/**
 * @brief Convierte una referencia de velocidad en porcentaje a un valor de PWM.
 * 
 * @param ref referencia de velocidad en porcentaje
 * @return int valor de PWM correspondiente
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

    value = 50;
    move(reftoPWM(value));

    add_repeating_timer_ms(-100, sample_timer_callback, NULL, &timer_rpm);
    while (true);
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

int reftoPWM(uint16_t ref)
{
    int x = (ref*6250)/100;
    return x;
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
