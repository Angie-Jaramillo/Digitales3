#define PulsosPorVuelta (20)
#define ENCONDER_PIN (16)

// struct DataPoint {
//   // unsigned long timepo_ms;
//   int pulsosPorSegundo;
// };

const int bufferSize = 5;  // Guardamos datos por 5 segundos
int dataBuffer[bufferSize];
int bufferIndex = 0;

volatile int contadorPulsos = 0;
unsigned long tiempoAnterior  = 0;

void contarPulsos() {
  contadorPulsos++;
}

void setup() {
  Serial.begin(115200);
  pinMode(ENCONDER_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENCONDER_PIN), contarPulsos, RISING);
  tiempoAnterior = millis();
}

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

