#include "FSM.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "pico/stdlib.h"
#include "driver_GPS.h"
#include "driver_i2c.h"
#include "driver_adc.h"

#define N_SAMPLES 10000

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
static struct repeating_timer pps_check;
static struct repeating_timer adc_sample;

static volatile bool button_pressed = false;
static volatile bool capture_cancelled = false;
static volatile bool pps_detected = false;
static int motivo_error = 0;
static uint8_t Offset_B0 = 0; // Offset para el bloque 0 de la EEPROM
static uint8_t Offset_B1 = 0; // Offset para el bloque 1 de la EEPROM

volatile uint16_t adc_buffer[N_SAMPLES];
volatile uint32_t adc_index = 0;
volatile bool buffer_full = false;

bool check_pps_callback(struct repeating_timer *t) {
    if (!pps_detected) {
        current_state = state_error;
        return false; // parar el timer
    }else {
        pps_detected = false; // Reiniciar la bandera de PPS detectado
        return true; // sigue corriendo
    }

}

bool adc_sampling_callback(struct repeating_timer *t) {
    if (current_state == state_error) {
        return false;
    }

    if (adc_index < N_SAMPLES) {
        uint16_t raw = adc_read_sample();
        adc_buffer[adc_index++] = raw;
    }

    if (adc_index >= N_SAMPLES) {
        buffer_full = true;
        return false;  // Para detener el timer
    }

    return true;
}

void gpio_callback(uint gpio, uint32_t events) {
    if (gpio == BUTTON_PIN) {
        if (current_state == state_capturing) {
            capture_cancelled = true;
        } else if (current_state == state_idle) {
            button_pressed = true;
        }else {
            button_pressed = false;
        }
    }
    else if (gpio == GPS_PPS_PIN) {
        if (events & GPIO_IRQ_EDGE_RISE) {
            pps_detected = true;
        }
    }
}

void fsm_init(void)
{
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

    // Inicializa UART del GPS
    gps_init();

    adc_driver_init(ADC_GPIO, 0);

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
        sleep_ms(5000);
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
    __wfi(); // Espera por una interrupción

    // Si se presiona el botón, cambiar al estado de captura

    while (1)
    {
        if (button_pressed)
        {
            button_pressed = false;
            printf("Button pressed! Transitioning to capturing state.\n");
            current_state = state_capturing;
            return;
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
        sleep_ms(10);
    }
}

static void state_capturing(void)
{
    printf("inicio captura");
    // reiniciar las bandaras de captura
    button_pressed = false;
    capture_cancelled = false;

    gpio_put(PIN_VERDE, false);   // Apagar el LED verde
    gpio_put(PIN_AMARILLO, true); // Encender el LED amarillo para indicar que está capturando

    add_repeating_timer_ms(2000, check_pps_callback, NULL, &pps_check); // Iniciar el temporizador para verificar PPS

    char datos[128];
    double lat = 0.0, lon = 0.0;
    uint8_t nivel_ruido;

    printf("Capturando datos del GPS...\n");

    bool fix_ok = false;

    while (!fix_ok)
    {
        if (current_state == state_error)
        {
            motivo_error = 1; // Error por falta de PPS
            return;
        }
        if (gps_read_line(datos, sizeof(datos)))
        {
            if (strncmp(datos, "$GNRMC", 6) == 0)
            {
                fix_ok = gps_parse_GNRMC(datos, &lat, &lon);
                if (!fix_ok)
                {
                    current_state = state_error;
                    return;
                }
                else
                {
                    break;
                }
            }
        }
    }
    
    printf("Lat, Lon: %.6f, %.6f\n", lat, lon);

    adc_index = 0;
    buffer_full = false;

    add_repeating_timer_ms(1, adc_sampling_callback, NULL, &adc_sample); // Iniciar el temporizador para verificar PPS

    while (!buffer_full) {
        if (!pps_detected) {
            printf("PPS not detected, transitioning to error state.\n");
            motivo_error = 1; // Error por falta de PPS
            cancel_repeating_timer(&adc_sample);
            cancel_repeating_timer(&pps_check);
            current_state = state_error;
            return;
        }
        if (capture_cancelled) {
            printf("Capture cancelled by button press.\n");
            cancel_repeating_timer(&adc_sample);
            cancel_repeating_timer(&pps_check);
            current_state = state_error;
            return;
        }
    }

    cancel_repeating_timer(&adc_sample);
    cancel_repeating_timer(&pps_check);

    uint32_t suma = 0;
    for (uint32_t i = 0; i < N_SAMPLES; i++) {
        int16_t centered = adc_buffer[i] - 2048;
        suma += centered * centered;
    }

    float rms = sqrtf((float)suma / N_SAMPLES);
    float vin_rms = (rms / 4095.0f) * 3.3f;
    float db_spl = 20.0f * log10f(vin_rms / 0.00005f);
    nivel_ruido = (uint8_t)(db_spl + 0.5);

    printf("Nivel de Ruido (uint8_t): %u\n", nivel_ruido);

    //PPS sigue bien y los datos son validos

    medicion_actual.latitud = lat;
    medicion_actual.longitud = lon;
    medicion_actual.nivel_de_ruido = nivel_ruido;

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

    gpio_put(PIN_AMARILLO, false); // Apagar el LED amarillo
    printf("Data written successfully\n");
    for (int i = 0; i < 6; i++) {  // 6 ciclos de 500 ms = 3 s
        gpio_put(PIN_NARANJA, true);// Encender el LED naranja para indicar que ya almacenó
        sleep_ms(250);
        gpio_put(PIN_NARANJA, false);
        sleep_ms(250);
    }

    current_state = state_idle; // Cambiar al estado idle después de almacenar
}

static void state_error(void)
{   
    gpio_put(PIN_AMARILLO, false); // Apagar el LED amarillo
    gpio_put(PIN_NARANJA, false);

    if(capture_cancelled) {
        gpio_put(PIN_ROJO, true);
        sleep_ms(3000);
        gpio_put(PIN_ROJO, false);
        capture_cancelled = false; // Reiniciar la bandera de captura cancelada
    }else if (motivo_error == 1) {
        printf("Error: No se detectó PPS.\n");
        for (int i = 0; i < 3; i++) {
            gpio_put(PIN_ROJO, true);
            sleep_ms(500);
            gpio_put(PIN_ROJO, false);
            sleep_ms(500);
        }
        motivo_error = 0; // Reiniciar el motivo del error
    } else {
        gpio_put(PIN_ROJO, true);
        sleep_ms(2000);
        gpio_put(PIN_ROJO, false);
    }
    
    gpio_put(PIN_ROJO, true); // Encender el LED rojo para indicar un error

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

    for (uint8_t i = 0; i < 5; i++)
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