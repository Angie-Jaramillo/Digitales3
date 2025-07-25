#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"

#include "FSM.h"

int main()
{
    stdio_init_all();

    sleep_ms(3000); // Espera para que el sistema se estabilice
    
    fsm_init(); // Inicializa la máquina de estados

    sleep_ms(3000); // Espera para que inicie la maquina de estados

    while (true) {
        fsm_run(); // Ejecuta la máquina de estados
    }
}
