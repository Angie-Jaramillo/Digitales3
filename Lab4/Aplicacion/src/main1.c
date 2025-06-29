int main() {
    stdio_init_all();
    sleep_ms(5000); // Espera para estabilizar la conexión

    printf("Leyendo desde dirección 0x0000...\n");
    uint8_t buffer_lectura[10] = {0};
    if (eeprom_read_nbytes(i2c0, 0x00, buffer_lectura, 10)) {
        for (int i = 0; i < 5; i++) {
            printf(buffer_lectura[i * 2], buffer_lectura[i * 2 + 1]);
        }
    } else {
        printf("Error en lectura\n");
    }

    return 0;
}
