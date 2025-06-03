/**
 * @file RPM.c
 * @author Juan Manuel Rivera Florez y Angie Paola Jaramillo
 * @brief Programa para medir la velocidad de un motor DC utilizando un encoder, se realiza mediante interrupciones.
 * @version 0.1
 * @date 2025-05-21
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "hardware/irq.h"
#include "hardware/gpio.h"

typedef struct {
    uint32_t    tiempo_ms;
    uint8_t     pwm;
    uint32_t    rpm;
} muestra_t;

muestra_t buffer[180];
uint16_t bufferIndex = 0;

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
struct repeating_timer timer_pwm;

/**
 * @brief Función de callback para el temporizador que calcula los rpm.
 * 
 * @param t pointer al temporizador 
 * @return true sigue ejecutándose el temporizador
 * @return false deja de ejecutarse el temporizador
 */
bool sample_timer_callback(struct repeating_timer *t);
/**
 * @brief Función de callback para el temporizador que cambia la referencia de PWM.
 * 
 * @param t pointer al temporizador 
 * @return true sigue ejecutándose el temporizador
 * @return false deja de ejecutarse el temporizador
 */
bool ref_timer_callback(struct repeating_timer *t);
/**
 * @brief Función de callback para el encoder que cuenta los pulsos.
 * 
 * @param gpio GPIO del encoder
 * @param events Eventos del GPIO
 */
void encoder_callback(uint gpio, uint32_t events);
/**
 * @brief Mueve el motor a una velocidad específica.
 * 
 * @param u Velocidad del motor en PWM
 */
void move(uint16_t u);
/**
 * @brief Mueve el motor hacia adelante a una velocidad específica.
 * 
 * @param u Velocidad del motor en PWM
 */
void forward(uint16_t u);
/**
 * @brief Mueve el motor hacia atrás a una velocidad específica.
 * 
 * @param u Velocidad del motor en PWM
 */
void backward(uint16_t u);
/**
 * @brief Inicia el proceso de captura de RPM y PWM.
 * 
 * @param valor Valor de referencia para el PWM
 */
void Start(uint16_t valor);
/**
 * @brief Configura el PWM del motor.
 * 
 * @param u Valor de referencia para el PWM
 */
void Pwm(uint16_t u);
/**
 * @brief Convierte un valor de referencia a un valor de PWM.
 * 
 * @param ref Valor de referencia (0-100)
 * @return int Valor de PWM correspondiente
 */
int reftoPWM(uint16_t ref);
/**
 * @brief Función de callback para el temporizador que imprime el valor de PWM y RPM.
 * 
 * @param t pointer al temporizador 
 * @return true sigue ejecutándose el temporizador
 * @return false deja de ejecutarse el temporizador
 */
bool pwm_timer_callback(struct repeating_timer *t);

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

    //Start(20);

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
                    else if (strncmp(comando, "PWM ", 4) == 0) { // Comando PWM
                        int val = atoi(&comando[4]);
                        // printf("The number is: %d\n", val);
                        if (val >= 0 && val <= 100) {
                            Pwm(val);
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

        buffer[bufferIndex].tiempo_ms = current_time;
        buffer[bufferIndex].pwm = ref;
        buffer[bufferIndex].rpm = rpm;
        bufferIndex++;
        
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
    
    for (int i = 0; i < 180; i++) {
        printf("%u %u %u\n", buffer[i].tiempo_ms, buffer[i].pwm, buffer[i].rpm);
    }
    memset(buffer, 0, sizeof(buffer));
    bufferIndex = 0;
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
        move(reftoPWM(100));
    }else{
        move(reftoPWM(ref));
    }
    //printf("Se cambio la ref a:%d\n", ref);
}

void Pwm(uint16_t u){
    // printf("Iniciando START:\n");
    move(reftoPWM(0));

    tiempo = 0;
    value = u;
    move(reftoPWM(value));

    add_repeating_timer_ms(-100, sample_timer_callback, NULL, &timer_rpm);

    add_repeating_timer_ms(-500, pwm_timer_callback, NULL, &timer_pwm);

    while (tiempo<=10);

    move(reftoPWM(0));
    bool cancelled = cancel_repeating_timer(&timer_rpm);
    cancelled = cancel_repeating_timer(&timer_pwm);
    tiempo = 0;
}

bool pwm_timer_callback(struct repeating_timer *t){
    if(t == &timer_pwm) {
        printf("%u %u %u\n", time_us_32()/1000, value, rpm);
        tiempo = tiempo + 0.5;
        return true;
    }
}