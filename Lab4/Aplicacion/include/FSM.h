#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/platform.h"
#include "hardware/gpio.h"
#include "hardware/sync.h"

#ifndef _FSM_H_
#define _FSM_H_

#define BUTTON_PIN 11 // Definir el pin del botón
#define GPS_PPS_PIN 3 // Definir el pin del PPS del GPS
#define ADC_GPIO 26   // Definir el pin del ADC

#define PIN_ROJO 12     // Definir el pin del LED rojo
#define PIN_AMARILLO 14 // Definir el pin del LED amarillo
#define PIN_NARANJA 13  // Definir el pin del LED naranja
#define PIN_VERDE 15    // Definir el pin del LED verde

#define EEPROM_BLOCK0 0x50
#define EEPROM_BLOCK1 0x51

#define SDA_PIN 16
#define SCL_PIN 17

#define LED_PIN 25


// Definición de un tipo para la función de estado (puntero a una funcion)
typedef void (*state_func_t)(void);

/**
 * @brief Estructura para almacenar las mediciones.
 * 
 * Esta estructura contiene la información de una medición, incluyendo
 * la longitud, latitud y el nivel de ruido.
 */
typedef struct {
    double longitud;
    double latitud;
    uint8_t nivel_de_ruido;
} medicion_t;

/**
 * @brief Enumeración de los estados de la máquina de estados.
 * 
 * Esta enumeración define los diferentes estados que puede tener la máquina de estados.
 * Cada estado representa una etapa en el proceso de captura y almacenamiento de datos.
 */
typedef enum {
    STATE_INIT,
    STATE_IDLE,
    STATE_CAPTURING,
    STATE_STORING,
    STATE_ERROR,
    STATE_DUMP
} fsm_state_enum_t;

/**
 * @brief Inicializa la máquina de estados.
 * 
 * Esta función establece el estado inicial de la máquina de estados 
 * y reinicia los índices de entrada de ID y contraseña.
 */
void fsm_init(void);

/**
 * @brief Ejecuta la máquina de estados.
 * 
 * Llama a la función correspondiente al estado actual de la máquina 
 * de estados.
 */
void fsm_run(void);

/**
 * @brief funcion para obtener el estado actual.
 *
 * Esta función retorna un puntero a la función que representa el estado actual de la máquina de estados
 * y permite que el sistema sepa qué acción tomar en función del estado actual.
 *
 * @return fsm_state_enum_t: estado actual de la máquina de estados.
 */
fsm_state_enum_t fsm_get_current_state(void);

/**
 * @brief Solicita mostrar los de datos.
 * 
 * Esta función establece una bandera para indicar que se ha solicitado mostrar los datos guardados.
 * El volcado se puede utilizar para depurar o almacenar información del estado actual.
 */
void fsm_request_dump(void);


#endif // _FSM_H_