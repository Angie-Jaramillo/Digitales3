#include "driver_GPS.h"
#include "hardware/uart.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* bool gps_has_fix(void) {
    return pps_detected;
} */

void gps_init(void)
{
    uart_init(GPS_UART, GPS_BAUDRATE);
    gpio_set_function(GPS_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(GPS_RX_PIN, GPIO_FUNC_UART);
}

bool gps_read_line(char *line, size_t max_len)
{
    size_t i = 0;
    while (i < max_len - 1)
    {
        if (uart_is_readable(GPS_UART))
        {
            char c = uart_getc(GPS_UART);
            if (c == '\n' || c == '\r')
            {
                if (i == 0)
                    continue; // ignora saltos iniciales
                else
                    break; // fin de línea
            }
            line[i++] = c;
        }
    }
    line[i] = '\0';
    return i > 0;
}

bool gps_parse_GNRMC(const char *line, double *lat, double *lon)
{
    // Copia segura
    char copy[128];
    strncpy(copy, line, sizeof(copy));
    copy[sizeof(copy) - 1] = '\0';

    char *token;
    char *fields[12] = {0};
    int i = 0;

    // Divide por coma
    token = strtok(copy, ",");
    while (token && i < 12)
    {
        fields[i++] = token;
        token = strtok(NULL, ",");
    }

    if (i < 7)
    {
        printf("RMC parse: campos insuficientes (%d)\n", i);
        return false;
    }

    if (fields[2][0] != 'A')
    {
        printf("RMC: Status=%c (no fix)\n", fields[2][0]);
        return false;
    }

    if (strlen(fields[3]) == 0 || strlen(fields[5]) == 0)
    {
        printf("RMC: Lat o Lon vacías\n");
        return false;
    }

    double raw_lat = atof(fields[3]);
    double raw_lon = atof(fields[5]);

    if (raw_lat == 0.0 || raw_lon == 0.0)
    {
        printf("RMC: lat/lon cruda = 0 → invalida\n");
        return false;
    }

    double deg_lat = (int)(raw_lat / 100);
    double min_lat = raw_lat - deg_lat * 100;
    *lat = deg_lat + min_lat / 60.0;
    if (fields[4][0] == 'S')
        *lat *= -1;

    double deg_lon = (int)(raw_lon / 100);
    double min_lon = raw_lon - deg_lon * 100;
    *lon = deg_lon + min_lon / 60.0;
    if (fields[6][0] == 'W')
        *lon *= -1;

    return true;
}