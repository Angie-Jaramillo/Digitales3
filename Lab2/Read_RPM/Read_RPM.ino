/**
 * @file Read_RPM.ino
 * 
 * @brief  Lectura de pulsos de un encoder para estimar la velocidad (RPM)
 * 
 * Este programa cuenta los pulsos de un encoder conectado a un pin digital y calcula la velocidad en revoluciones por minuto (RPM) cada segundo.
 * 
 * @authors Angie Paola Jaramillo Ortega,
 *         Juan Manuel Rivera Florez
 * 
 * @date 04-05-2025
 * 
 * @version 1.0
 * 
 */


#define PulsosPorVuelta (20)  // Pulsos por vuelta del encoder
#define ENCONDER_PIN (16)     // Pin del encoder

// struct DataPoint {
//   // unsigned long timepo_ms;
//   int pulsosPorSegundo;
// };

const int bufferSize = 5;     // Guardamos datos por 5 segundos
int dataBuffer[bufferSize];   // Buffer para almacenar los datos
int bufferIndex = 0;          // Índice del buffer

volatile int contadorPulsos = 0;    // Contador de pulsos del encoder
unsigned long tiempoAnterior  = 0;  // Variable para almacenar el tiempo anterior

/**
 * @brief Función de interrupción para contar los pulsos del encoder
 * 
 * Esta función se ejecuta cada vez que se detecta un pulso en el pin del encoder.
 * Incrementa el contador de pulsos.
 */
void contarPulsos() {
  contadorPulsos++;
}


/**
 * @brief Función de configuración inicial
 * 
 * Configura los pines, la frecuencia y resolución del PWM, y establece la comunicación serial.
 */
void setup() {
  Serial.begin(115200);
  pinMode(ENCONDER_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENCONDER_PIN), contarPulsos, RISING);
  tiempoAnterior = millis();
}

/**
 * @brief Función principal del programa
 * 
 * En esta función se manejan los estados del sistema, se leen los comandos por USB y se realizan las mediciones.
 */
void loop() {
  unsigned long tiempoActual = millis();

  // Cada 1 segundo
  if (tiempoActual - tiempoAnterior  >= 1000) {
    noInterrupts();
    int count = contadorPulsos;
    contadorPulsos = 0;
    interrupts();

    // Guardar en buffer
    dataBuffer[bufferIndex++] = count;

    tiempoAnterior  = tiempoActual;
  }

  // Después de 5 datos (5 segundos), enviamos por serial
  if (bufferIndex >= bufferSize) {
    int sum = 0;
    for (int i = 0; i < bufferSize; i++) {
      // Serial.print("Tiempo (ms): ");
      // Serial.print(dataBuffer[i].timepo_ms);
      // Serial.print(" | pulsosPorSegundo: ");
      // Serial.println(dataBuffer[i].pulsosPorSegundo);
      sum += dataBuffer[i];
    }
    float rpm = ((sum / (float)bufferSize) / (float)PulsosPorVuelta) * 60.0;
    // Serial.print("rpm: ");
    Serial.println(rpm);

    bufferIndex = 0; // Reiniciar buffer
  }
}

