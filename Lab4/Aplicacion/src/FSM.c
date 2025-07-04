#include "FSM.h"
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "driver_GPS.h"
#include "driver_i2c.h"

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
static volatile bool pps_detected = false;
static uint8_t Offset_B0 = 0; // Offset de posición de escritura en la EEPROM con respecto a la última escritura
static uint8_t Offset_B1 = 0; // Offset de posición de escritura en la EEPROM con respecto a la última escritura

void gpio_callback(uint gpio, uint32_t events) {
    if (gpio == BUTTON_PIN) {
        if (current_state == state_capturing) {
            capture_cancelled = true;
        } else {
            button_pressed = true;
        }
    }
    else if (gpio == GPS_PPS_PIN) {
        if (events & GPIO_IRQ_EDGE_RISE) {
            pps_detected = true;  // O lo que uses para PPS
        }
    }
}

void fsm_init(void)
{
    // Inicializa el estado actual a la función de estado inicial

    // Configura la interrupción del botón
    gpio_set_irq_enabled_with_callback(BUTTON_PIN, GPIO_IRQ_EDGE_RISE, true, &gpio_callback);
    gpio_set_irq_enabled_with_callback(GPS_PPS_PIN, GPIO_IRQ_EDGE_RISE, true, &gpio_callback);

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

    // Inicializa LED para indicar que está corriendo
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 1); // Enciende el LED para indicar inicio

    // Inicializa el I2C para la EEPROM
    eeprom_init(i2c0, SDA_PIN, SCL_PIN);

    // Inicializa el UART para el GPS
    gps_init();

    current_state = init_state;
}

void fsm_run(void)
{
    if (current_state)
    {
        current_state(); // Llama a la función del estado actual
    }
}

static void init_state(void)
{
    while (!pps_detected)
    {
        printf("Esperando GPS...\n");
        sleep_ms(5000); // Espera antes de volver a verificar
    }

    printf("Transitioning to IDLE.\n");
    current_state = state_idle;
}

static void state_idle(void)
{
    gpio_put(PIN_NARANJA, false);
    gpio_put(PIN_ROJO, false);
    gpio_put(PIN_AMARILLO, false);
    gpio_put(PIN_VERDE, true); // Encender el LED verde para indicar que está en estado idle

    char comando[32];
    int cmd_i = 0;
    // __wfi(); // Espera por una interrupción

    // Si se presiona el botón, cambiar al estado de captura

    while (1)
    {
        if (button_pressed)
        {
            button_pressed = false;
            printf("Button pressed! Transitioning to capturing state.\n");
            current_state = state_capturing;
            return; // Sale del while(1)
        }
        if (stdio_usb_connected())
        {
            int c = getchar_timeout_us(0);
            if (c != PICO_ERROR_TIMEOUT)
            {
                if (c == '\r' || c == '\n')
                {
                    comando[cmd_i] = '\0';
                    printf("Comando recibido: %s\n", comando);
                    if (strncmp(comando, "DUMP", 4) == 0)
                    {
                        current_state = state_dump;
                        return; // Sale del while(1)
                    }
                    cmd_i = 0; // Reinicia buffer
                }
                else if (cmd_i < (int)(sizeof(comando) - 1))
                {
                    comando[cmd_i++] = (char)c;
                }
            }
        }
        sleep_ms(10); // Evita consumir 100% de CPU
    }
}

static void state_capturing(void)
{
    // reiniciar las bandaras de captura
    button_pressed = false;

    // leer el gps y el microfono y guardar los datos
    // si hay un error, cambiar al estado de error
    char line[128];
    double lat = 0.0, lon = 0.0;

    printf("Capturando datos del GPS...\n");

    bool fix_ok = false;

    while (!fix_ok)
    {
        if (!pps_detected)
        {
            printf("PPS perdido. GPS sin fix físico.\n");
            current_state = state_error;
            return;
        }
        if (gps_read_line(line, sizeof(line)))
        {
            if (strncmp(line, "$GNRMC", 6) == 0)
            {
                fix_ok = gps_parse_GNRMC(line, &lat, &lon);
                if (!fix_ok)
                {
                    printf("Fix lógico inválido (Status=V). Error.\n");
                    current_state = state_error;
                    return;
                }
            }
        }

        sleep_ms(20); // Pequeño respiro al CPU
    }

    // Si llegas aquí, PPS sigue bien y `$GNRMC` es válido
    printf("Fix OK. Lat: %.6f, Lon: %.6f\n", lat, lon);

    gpio_put(PIN_VERDE, false);   // Apagar el LED verde
    gpio_put(PIN_AMARILLO, true); // Encender el LED amarillo para indicar que está capturando

    medicion_actual.latitud = 0.0;
    medicion_actual.longitud = 0.0;
    medicion_actual.nivel_de_ruido = 0.0;

    sleep_ms(2000); // Simula el tiempo de captura de datos
    printf("Data captured successfully. Transitioning to storing state.\n");
    current_state = state_storing; // Volver al estado de almacenamiento después de capturar
}

static void state_storing(void)
{
    double lat = medicion_actual.latitud;
    double lon = medicion_actual.longitud;
    uint8_t nivel = medicion_actual.nivel_de_ruido;

    uint8_t latitude_bytes[8];
    uint8_t longitud_bytes[8];

    memcpy(latitude_bytes, &lat, sizeof(lat));
    memcpy(longitud_bytes, &lon, sizeof(lon));

    uint8_t buffer[16];
    memcpy(buffer, latitude_bytes, 8);
    memcpy(buffer + 8, longitud_bytes, 8);

    uint8_t page_size = 16;

    /*     if ((Offset_B0 % page_size) + 16 > page_size) {
            Offset_B0 += page_size - (Offset_B0 % page_size);
        } */

    if (!eeprom_write_nbytes(i2c0, EEPROM_BLOCK0, Offset_B0, buffer, 16))
    {
        printf("Error escribiendo EEPROM\n");
        current_state = state_error;
        return;
    }

    Offset_B0 += 16;

    if (!eeprom_write_nbytes(i2c0, EEPROM_BLOCK1, Offset_B1, &nivel, 1))
    {
        printf("Error escribiendo EEPROM\n");
        current_state = state_error;
        return;
    }

    Offset_B1 += 1;

    sleep_ms(300);                 // Tiempo extra por seguridad
    gpio_put(PIN_AMARILLO, false); // Apagar el LED amarillo
    gpio_put(PIN_NARANJA, true);   // Encender el LED naranja para indicar que está almacenando
    printf("Data written successfully\n");
    current_state = state_idle; // Cambiar al estado idle después de almacenar
}

static void state_error(void)
{
    // Aquí se puede implementar la lógica del estado de error
    // por ejemplo, encender un LED rojo o reiniciar el sistema

    gpio_put(PIN_AMARILLO, false); // Apagar el LED amarillo
    gpio_put(PIN_NARANJA, false);
    gpio_put(PIN_ROJO, true); // Encender el LED rojo para indicar un error

    sleep_ms(2000); // Simula el tiempo de manejo del error

    // Después de manejar el error, se puede volver al estado idle
    current_state = init_state;
}

static void state_dump(void)
{
    printf("Dumping data...\n");

    uint8_t buffer_lectura[16];
    uint8_t nivel_ruido;
    uint8_t pos_B0 = 0;
    uint8_t pos_B1 = 0;

    for (uint8_t i = 0; i < 4; i++)
    {

        if (!eeprom_read_nbytes(i2c0, EEPROM_BLOCK0, pos_B0, buffer_lectura, 16))
        {
            printf("Error leyendo EEPROM en pos %d\n", pos_B0);
            current_state = state_error;
            return;
        }

        if (!eeprom_read_nbytes(i2c0, EEPROM_BLOCK1, pos_B1, &nivel_ruido, 1))
        {
            printf("Error leyendo EEPROM en pos %d\n", pos_B1);
            current_state = state_error;
            return;
        }

        double lat, lon;
        memcpy(&lat, buffer_lectura + 0, 8);
        memcpy(&lon, buffer_lectura + 8, 8);

        printf("Coordenadas: %.6f, %.6f, Nivel de ruido: %d dB\n",
               lat, lon, nivel_ruido);

        pos_B0 += 16;
        pos_B1 += 1;
    }

    printf("Dump completado. Regresando a estado IDLE.\n");
    current_state = state_idle;
}