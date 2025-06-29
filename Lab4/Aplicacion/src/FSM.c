#include "FSM.h"
#include <stdio.h>
#include <string.h>
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

typedef struct {
    uint8_t posicion_geografica;
    uint8_t nivel_de_ruido;
} medicion_t;

medicion_t medicion_actual;

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
static volatile bool capture_cancelled = false;
static uint8_t Offset = 2;

void gpio_callback(uint gpio, uint32_t events) {

    printf("IRQ: gpio=%d, events=0x%x\n", gpio, events);
    
    if (current_state == state_capturing) {
        capture_cancelled = true;
    } else {
        button_pressed = true;
    }
}

/* void fsm_request_dump() {
    dump_requested = true; // Establece la bandera de solicitud de volcado
    printf("Dump requested!\n");
} */

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
    gpio_put(PIN_NARANJA, false);
    gpio_put(PIN_ROJO, false);
    gpio_put(PIN_AMARILLO, false);      
    gpio_put(PIN_VERDE, true);    // Encender el LED verde para indicar que está en estado idle

    char comando[32];
    int cmd_i = 0;

    // __wfi(); // Espera por una interrupción

    // Si se presiona el botón, cambiar al estado de captura

    while (1) {
        if (button_pressed) {
            button_pressed = false;
            printf("Button pressed! Transitioning to capturing state.\n");
            current_state = state_capturing;
            return;  // Sale del while(1)
        }
        if (stdio_usb_connected()) {
            int c = getchar_timeout_us(0);
            if (c != PICO_ERROR_TIMEOUT) {
                if (c == '\r' || c == '\n') {
                    comando[cmd_i] = '\0';
                    printf("Comando recibido: %s\n", comando);
                    if (strncmp(comando, "DUMP", 4) == 0) {
                        current_state = state_dump;
                        return;  // Sale del while(1)
                    }
                    cmd_i = 0;  // Reinicia buffer
                } else if (cmd_i < (int)(sizeof(comando) - 1)) {
                    comando[cmd_i++] = (char)c;
                }
            }
        }
        sleep_ms(10); // Evita consumir 100% de CPU
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

/*     const uint32_t capture_duration_ms = 10000; // Simulación de 10s
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
    } */

        // Aquí simulas valores
    medicion_actual.posicion_geografica = 42;  // Simula ID ubicación
    medicion_actual.nivel_de_ruido = 85;       // Simula nivel dB

    sleep_ms(2000); // Simula el tiempo de captura de datos
    printf("Data captured successfully. Transitioning to storing state.\n");
    current_state = state_storing; // Volver al estado de almacenamiento después de capturar
}

static void state_storing(void) {
    // Aquí se guardan los datos capturados en la memoria

/*     uint8_t datos_a_guardar[5][2] = {
        {101, 235},  // portería 1
        {28, 35},  // portería 2
        {22, 79},  // portería 3
        {93, 15},  // portería 4
        {1, 93}   // portería 5
    }; */

    uint8_t datos_a_guardar[2] = {
        medicion_actual.posicion_geografica,
        medicion_actual.nivel_de_ruido
    };

    printf("Escribiendo...\n");

    if (!eeprom_write_2bytes(i2c0, Offset, datos_a_guardar)) {
        printf("Error writing to EEPROM");
        current_state = state_error; // Cambiar al estado de error si falla la escritura
    }else{
        sleep_ms(10); // Tiempo extra por seguridad
        gpio_put(PIN_AMARILLO, false); // Apagar el LED amarillo
        gpio_put(PIN_NARANJA, true); // Encender el LED naranja para indicar que está almacenando
        printf("Data written successfully\n");
        Offset += 2;
        sleep_ms(3000);
        current_state = state_idle; // Cambiar al estado idle después de almacenar
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
    
    printf("Dumping data...\n");
    uint8_t buffer_lectura[10] = {0};

    if (eeprom_read_nbytes(i2c0, 0x00, buffer_lectura, 10)) {
        for (int i = 0; i < 5; i++) {
            printf("%d , %d\n", buffer_lectura[i * 2], buffer_lectura[i * 2 + 1]);
        }   
        printf("Data dumped successfully. Transitioning to idle state.\n");
        current_state = state_idle; // Volver al estado idle    
    } else {
        printf("Error en lectura\n");
        current_state = state_error; // Cambiar al estado de error si falla la lectura
    }

/*     printf("Data dumped successfully. Transitioning to idle state.\n");
    current_state = state_idle; // Volver al estado idle     */

}

