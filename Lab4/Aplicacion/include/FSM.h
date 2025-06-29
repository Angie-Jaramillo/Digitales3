#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/platform.h"
#include "hardware/gpio.h"
#include "hardware/sync.h"


#ifndef _FSM_H_
#define _FSM_H_

// Definición de un tipo para la función de estado (puntero a una funcion)
typedef void (*state_func_t)(void);


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

void fsm_request_dump(void);


#endif // _FSM_H_