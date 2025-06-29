#include "FSM.h"
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "driver_i2c.h"

#define BUTTON_PIN 11 // Definir el pin del botón

#define PIN_ROJO 12 // Definir el pin del LED rojo
#define PIN_AMARILLO 14 // Definir el pin del LED amarillo
#define PIN_NARANJA 13 // Definir el pin del LED naranja
#define PIN_VERDE 15 // Definir el pin del LED verde

#define SDA_PIN 16
#define SCL_PIN 17

#define LED_PIN 25


// Definición del tipo de función de estado 
typedef void (*state_func_t)(void);

// Prototipos de las funciones de estado
static void init_state(void);
static void state_idle(void);
static void state_capturing(void);
static void state_storing(void);
static void state_error(void);
static void state_dump(void);

static state_func_t current_state;

static volatile bool button_pressed = false;
static volatile bool dump_requested = false;

static volatile bool capture_cancelled = false;

void gpio_callback(uint gpio, uint32_t events) {

    printf("IRQ: gpio=%d, events=0x%x\n", gpio, events);
    
    if (current_state == state_capturing) {
        capture_cancelled = true;
    } else {
        button_pressed = true;
    }
}

void fsm_request_dump() {
    dump_requested = true; // Establece la bandera de solicitud de volcado
    printf("Dump requested!\n");
}

void fsm_init(void) {
    // Inicializa el estado actual a la función de estado inicial

    // Configura la interrupción del botón
    gpio_set_irq_enabled_with_callback(BUTTON_PIN, GPIO_IRQ_EDGE_RISE, true, &gpio_callback);

    // Inicializa el GPIO del botón
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_down(BUTTON_PIN);

    // Inicializa los pines de los LEDs
    gpio_init(PIN_ROJO);
    gpio_set_dir(PIN_ROJO, GPIO_OUT);
    gpio_put(PIN_ROJO, false); // Asegúrate de que el LED rojo esté apagado al inicio

    gpio_init(PIN_AMARILLO);
    gpio_set_dir(PIN_AMARILLO, GPIO_OUT);
    gpio_put(PIN_AMARILLO, false); // Asegúrate de que el LED amarillo esté apagado al inicio

    gpio_init(PIN_NARANJA);
    gpio_set_dir(PIN_NARANJA, GPIO_OUT);
    gpio_put(PIN_NARANJA, false); // Asegúrate de que el LED naranja esté apagado al inicio

    gpio_init(PIN_VERDE);
    gpio_set_dir(PIN_VERDE, GPIO_OUT);
    gpio_put(PIN_VERDE, false); // Asegúrate de que el LED verde esté apagado al inicio

    //Inicializa LED para indicar que está corriendo
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 1); // Enciende el LED para indicar inicio

    // Inicializa el I2C para la EEPROM
    eeprom_init(i2c0, SDA_PIN, SCL_PIN);

    current_state = init_state;

}

void fsm_run(void) {
    if (current_state) {
        current_state(); // Llama a la función del estado actual
    }
}

static void init_state(void) {

    // Aquí se puede realizar la inicialización necesaria
    
    // Cambia al estado idle después de la inicialización
    printf("FSM initialized. Transitioning to idle state.\n");
    // if(gps) {
    //     current_state = state_idle;
    //
    current_state = state_idle;

}

static void state_idle(void) {

    // Aquí se puede implementar la lógica del estado idle
    // tienes que empezar en modo de bajo consumo y
    // esperar a que ocurra un evento para cambiar


    gpio_put(PIN_NARANJA, false);
    gpio_put(PIN_ROJO, false);
    gpio_put(PIN_AMARILLO, false);      
    gpio_put(PIN_VERDE, true);    // Encender el LED verde para indicar que está en estado idle
    

    __wfi(); // Espera por una interrupción

    // Si se presiona el botón, cambiar al estado de captura

    if (button_pressed) {
        button_pressed = false; // Reiniciar la bandera
        printf("Button pressed! Transitioning to capturing state.\n");
        current_state = state_capturing;
    } else if (dump_requested) {
        dump_requested = false; // Reiniciar la bandera
        printf("Dump requested! Transitioning to dump state.\n");
        current_state = state_dump;
    }
    

}

static void state_capturing(void) {
    // leer el gps y el microfono y guardar los datos
    // si hay un error, cambiar al estado de error
    // si se ha capturado correctamente, cambiar al estado de idle

    // if (condition_for_error) {
    //     current_state = state_error;
    // } else if (condition_for_success) {
    //     current_state = state_idle;
    // }

    gpio_put(PIN_VERDE, false); // Apagar el LED verde
    gpio_put(PIN_AMARILLO, true); // Encender el LED amarillo para indicar que está capturando
    
    //reiniciar las bandaras de captura
    button_pressed = false;
    // dump_requested = false;

    const uint32_t capture_duration_ms = 10000; // Simulación de 10s
    const uint32_t step_ms = 100; // Chequear cada 100ms
    uint32_t elapsed = 0;

    while (elapsed < capture_duration_ms) {
        // Simula leer el ADC o hacer algún trabajo simple
        printf("Capturing sample at t=%lu ms\n", elapsed);

        sleep_ms(step_ms); // Esto te da tiempo para presionar el botón

        if (capture_cancelled) {
            printf("Capture cancelled by button. Transitioning to error state.\n");
            capture_cancelled = false; // limpia la bandera
            current_state = state_error;
            return;
        }

        elapsed += step_ms;
    }

    sleep_ms(2000); // Simula el tiempo de captura de datos
    printf("Data captured successfully. Transitioning to storing state.\n");
    current_state = state_storing; // Volver al estado de almacenamiento después de capturar
}

static void state_storing(void) {
    // Aquí se guardan los datos capturados en la memoria

    uint8_t datos_a_guardar[5][2] = {
        {100, 255},  // portería 1
        {28, 35},  // portería 2
        {20, 79},  // portería 3
        {93, 14},  // portería 4
        {1, 93}   // portería 5
    };

    printf("Escribiendo...\n");
    for (uint8_t i = 0; i < 5; i++) {
        uint8_t offset = i * 2;
        if (!eeprom_write_2bytes(i2c0, offset, datos_a_guardar[i])) {
            printf("Error writing to EEPROM at offset %d\n", offset);
            current_state = state_error; // Cambiar al estado de error si falla la escritura
            return;
        } else {
            sleep_ms(10); // Tiempo extra por seguridad
            gpio_put(PIN_AMARILLO, false); // Apagar el LED amarillo
            gpio_put(PIN_NARANJA, true); // Encender el LED naranja para indicar que está almacenando
            printf("Data written successfully\n");
            current_state = state_idle; // Volver al estado idle después de almacenar
        }
    }
}

static void state_error(void){
    // Aquí se puede implementar la lógica del estado de error
    // por ejemplo, encender un LED rojo o reiniciar el sistema

    gpio_put(PIN_AMARILLO, false); // Apagar el LED amarillo
    gpio_put(PIN_NARANJA, false);
    gpio_put(PIN_ROJO, true); // Encender el LED rojo para indicar un error

    sleep_ms(2000); // Simula el tiempo de manejo del error

    // Después de manejar el error, se puede volver al estado idle
    current_state = state_idle;
}

static void state_dump(void){
    // este estado solo se entra por el comando de dump en serial
    
    // Aquí se puede implementar la lógica del estado de volcado
    // por ejemplo, enviar los datos capturados a través de UART o guardarlos en la

    printf("Dumping data...\n");
    uint8_t buffer_lectura[10] = {0};
    if (eeprom_read_nbytes(i2c0, 0x00, buffer_lectura, 10)) {
        for (int i = 0; i < 5; i++) {
            printf(buffer_lectura[i * 2], buffer_lectura[i * 2 + 1]);
        }   
        printf("Data dumped successfully. Transitioning to idle state.\n");
        current_state = state_idle; // Volver al estado idle    
    } else {
        printf("Error en lectura\n");
        current_state = state_error; // Cambiar al estado de error si falla la lectura
    }

    printf("Data dumped successfully. Transitioning to idle state.\n");
    current_state = state_idle; // Volver al estado idle    

}

