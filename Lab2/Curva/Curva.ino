//----- CONFIGURACION DE LOS PINES ---------
#define ENA (15)
#define EN1 (14)
#define EN2 (13)
#define ENCODER_PIN (16)

#define PulsosPorVuelta (20)

//---- CONFIGURACION DEL PWM ------------
#define PWM_FREQ (500)          // Frecuencia en Hz
#define PWM_RESOLUTION (8)      // Resolución del PWM
#define PWM_CYCLE (150)         // Valor del ciclo de trabajo 0-255 --> 0%-100%


struct Data {
  unsigned long time;   // Tiempo en milisengundos
  float rpm;            // Velocidad del motor en ese punto
};

const int bufferSize = 5;       // Cuantas muestras de rpm se guarda para el filtro de media movil
Data dataBuffer[bufferSize];         // Inicialización del buffer
int bufferIndex = 0;            // Contador para iterar sobre los elementos del buffer

volatile int contadorPulsos = 0;    ///< Contador de pulsos del encoder
unsigned long tiempoAnterior = 0;   ///< Ultimo tiempo registrado


void contarPulsos() {
  contadorPulsos++;
}


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  // Inicialización de los pines del puente H
  pinMode(EN1, OUTPUT);
  digitalWrite(EN1, HIGH);
  pinMode(EN2, OUTPUT);
  digitalWrite(EN2, LOW);

  // Se escribe el pwm 
  analogWriteFreq(PWM_FREQ);
  analogWriteResolution(PWM_RESOLUTION);
  analogWrite(ENA, PWM_CYCLE);

  // Se inicializa el pin del encoder y la interrupción
  pinMode(ENCODER_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN), contarPulsos, RISING);

}

void loop() {
  unsigned long tiempoActual = millis();

  // Cada 1 segundo
  if (tiempoActual - tiempoAnterior >= 1000) {
    noInterrupts();
    float count = contadorPulsos;
    contadorPulsos = 0;
    interrupts();

    float rpm = (count / (float)PulsosPorVuelta) * 60.0;

    // Guardar en buffer
    if (bufferIndex < bufferSize) {
      dataBuffer[bufferIndex++] = {tiempoActual, rpm};
    }

    tiempoAnterior = tiempoActual;
  }

  // Después de 5 datos (5 segundos), enviamos por serial
  if (bufferIndex >= bufferSize) {
    float sum = 0;
    for (int i = 0; i < bufferSize; i++) {
      // Serial.print("Tiempo (ms): ");
      // Serial.print(dataBuffer[i].time);
      // Serial.print(" | RPM: ");
      // Serial.println(dataBuffer[i].rpm);
      sum += dataBuffer[i].rpm;
    }

    Serial.print("RPM: ");
    Serial.println(sum/bufferSize);


    bufferIndex = 0; // Reiniciar buffer
  }
}


