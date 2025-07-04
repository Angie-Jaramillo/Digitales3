#include "driver_GPS.h"
#include "hardware/uart.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* bool gps_has_fix(void) {
    return pps_detected;
} */

void gps_init(void) {
    uart_init(GPS_UART, GPS_BAUDRATE);
    gpio_set_function(GPS_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(GPS_RX_PIN, GPIO_FUNC_UART);
}

bool gps_read_line(char *line, size_t max_len) {
    size_t i = 0;
    while (i < max_len - 1) {
        if (uart_is_readable(GPS_UART)) {
            char c = uart_getc(GPS_UART);
            if (c == '\n' || c == '\r') {
                if (i > 0) break;
                else continue;
            }
            line[i++] = c;
        }
    }
    line[i] = '\0';
    return true;
}

bool gps_parse_GNRMC(const char *line, double *lat, double *lon) {
    char type[7];
    char status;
    char lat_s[15], ns, lon_s[15], ew;

    int scanned = sscanf(line, "$%6[^,],%*[^,],%c,%[^,],%c,%[^,],%c",
                         type, &status, lat_s, &ns, lon_s, &ew);

    if (scanned != 6) {
        printf("Parse failed: scanned %d fields\n", scanned);
        return false;
    }
    if (status != 'A') {
        printf("No fix (Status=%c)\n", status);
        return false;
    }
    if (strlen(lat_s) == 0 || strlen(lon_s) == 0) {
        printf("Empty lat/lon field\n");
        return false;
    }

    double raw_lat = atof(lat_s);
    double raw_lon = atof(lon_s);

    if (raw_lat == 0.0 || raw_lon == 0.0) {
        printf("Zero lat/lon → invalid\n");
        return false;
    }

    double deg_lat = (int)(raw_lat / 100);
    double min_lat = raw_lat - deg_lat * 100;
    *lat = deg_lat + min_lat / 60.0;
    if (ns == 'S') *lat *= -1;

    double deg_lon = (int)(raw_lon / 100);
    double min_lon = raw_lon - deg_lon * 100;
    *lon = deg_lon + min_lon / 60.0;
    if (ew == 'W') *lon *= -1;

    return true;
}
