#ifndef DRIVER_GPS_H_
#define DRIVER_GPS_H_

#include "pico/stdlib.h"

// UART config
#define GPS_UART uart1
#define GPS_BAUDRATE 9600
#define GPS_TX_PIN 4
#define GPS_RX_PIN 5

void gps_init(void);
bool gps_read_line(char *line, size_t max_len);
bool gps_parse_GNRMC(const char *line, double *lat, double *lon);

#endif