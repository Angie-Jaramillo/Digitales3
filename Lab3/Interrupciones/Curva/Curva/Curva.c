/**
 * @file Curva.c
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

typedef struct {
    uint32_t    tiempo_ms;
    uint8_t     pwm;
    uint32_t    rpm;
} muestra_t;

muestra_t buffer[4500];
uint16_t bufferIndex = 0;

const uint8_t encoder_pin = 14;
const uint8_t Enable_motor_pin = 15;
const uint8_t IN1_pin = 16;
const uint8_t IN2_pin = 17;

volatile uint32_t counter = 0;
volatile uint16_t rpm = 0;
int ref = 0, value = 0;
int direc = 1;
int end = 1;

char comando[32];
int cmd_i = 0;

struct repeating_timer timer_rpm;
struct repeating_timer change_ref;
/**
 * @brief Función de callback para el temporizador que calcula los rpm.
 * 
 * @param t pointer al temporizador 
 * @return true sigue ejecutándose el temporizador
 * @return false deja de ejecutarse el temporizador
 */
bool sample_timer_callback(struct repeating_timer *t);
/**
 * @brief Función de callback para el temporizador que cambia la referencia.
 * 
 * @param t pointer al temporizador
 * @return true sigue ejecutándose el temporizador
 * @return false deja de ejecutarse el temporizador
 */
bool ref_timer_callback(struct repeating_timer *t);
/**
 * @brief Función de callback para el encoder.
 * 
 * @param gpio GPIO del encoder
 * @param events Eventos del GPIO
 */
void encoder_callback(uint gpio, uint32_t events);
/**
 * @brief Mueve el motor a una velocidad específica.
 * 
 * @param u Velocidad del motor (0-6250)
 */
void move(uint16_t u);
/**
 * @brief Mueve el motor hacia adelante a una velocidad específica.
 * 
 * @param u Velocidad del motor (0-6250)
 */
void forward(uint16_t u);
/**
 * @brief Mueve el motor hacia atrás a una velocidad específica.
 * 
 * @param u Velocidad del motor (0-6250)
 */
void backward(uint16_t u);
/**
 * @brief Inicia el proceso de captura de datos.
 * 
 * @param valor Valor de referencia para el motor (0-100)
 */
void Start(uint16_t valor);
/**
 * @brief Convierte un valor de referencia (0-100) a un valor PWM (0-6250).
 * 
 * @param ref Valor de referencia (0-100)
 * @return int Valor PWM correspondiente
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

    while (end);


    for (int i = 0; i < 4500; i++) {
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
    if (ref >= 100)
    {
        direc = 0;
    }
    if((direc == 0) && (ref == 0))
    {
        end = 0;
        bool cancel = cancel_repeating_timer(&timer_rpm);      // Detén el timer de muestreo
        cancel = cancel_repeating_timer(&change_ref);     // Detén el timer de referencia
        move(0);                                // Detén el motor
        return false;                           // Detén este timer
    }
    if (direc == 1)
    {
        ref = ref + value;
    }
    else if (direc == 0)
    {
        ref = ref - value;
    }
    move(reftoPWM(ref));
    return true;
}
