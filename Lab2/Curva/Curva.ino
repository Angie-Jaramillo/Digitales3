#define PIN_ENCODER           16

#define PIN_PWM               15

#define PULSOS_POR_VUELTA     20

#define FRECUENCIA_PWM        250

#define RESOLUCION_PWM        8

#define ESCALON_PORCENTAJE    20      // % de incremento por escalón

// Cálculo de número de escalones
#define NUM_ESCALONES_SUBIDA  6       
#define NUM_ESCALONES_BAJADA  5
#define TOTAL_ESCALONES       11      // 0, 20, 40, 60, 80, 100, 80, 60, 40, 20, 0

#define MUESTRAS_POR_ESCALON  500

const int MAX_MUESTRAS = TOTAL_ESCALONES * MUESTRAS_POR_ESCALON;//Tamaño máximo del buffer


struct Buffer {
  unsigned long tiempo;
  uint8_t valor_pwm;
  float rpm;
};

Buffer datos[MAX_MUESTRAS];
int buffIndex = 0;

volatile int conteo_pulsos = 0;

void contarPulsos() {
  conteo_pulsos++;
}

void setup() {
  Serial.begin(115200);

  pinMode(PIN_ENCODER, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_ENCODER), contarPulsos, RISING);

  pinMode(PIN_PWM, OUTPUT);
  analogWriteFreq(FRECUENCIA_PWM);
  analogWriteResolution(RESOLUCION_PWM);
  analogWrite(PIN_PWM, 0);

  pinMode(14, OUTPUT);
  pinMode(13, OUTPUT);
  digitalWrite(14, HIGH);
  digitalWrite(13, LOW);

  Serial.println("tiempo(ms),valor_pwm(%),RPM");
}

void loop() {

  static unsigned long tiempo_inicio = millis();
  static unsigned long tiempo_ultima_muestra = tiempo_inicio;
  static unsigned long tiempo_inicio_escalon = tiempo_inicio;
  static int escalon_actual = 0;

  unsigned long tiempo_actual = millis();

  // Muestreo cada 4ms
  if (tiempo_actual - tiempo_ultima_muestra >= 4) {// && buffIndex < MAX_MUESTRAS

    noInterrupts();
    int pulsos = conteo_pulsos;
    conteo_pulsos = 0;
    interrupts();

    float pulsos_por_segundo = pulsos * (1000.0 / 4.0);
    float rpm = (pulsos_por_segundo / PULSOS_POR_VUELTA) * 60.0;
    unsigned long tiempo_rel = tiempo_actual - tiempo_inicio;

    //Determinar valor PWM actual según si es ascendente o descendente
    uint8_t porcentaje_pwm;
    if (escalon_actual < NUM_ESCALONES_SUBIDA) {

      porcentaje_pwm = escalon_actual * ESCALON_PORCENTAJE;

    } else {

      porcentaje_pwm = (TOTAL_ESCALONES - escalon_actual) * ESCALON_PORCENTAJE;

    }

    uint8_t valor_pwm = map(porcentaje_pwm, 0, 100, 0, 255);
    analogWrite(PIN_PWM, valor_pwm);

    datos[buffIndex++] = {tiempo_rel, valor_pwm, rpm }; //Almacenar datos
    tiempo_ultima_muestra = tiempo_actual;
  }

  // Cambiar escalón cada 2s
  if (tiempo_actual - tiempo_inicio_escalon >= 2000) { //verifica si han pasado 2s desde que comenzó el escalón actual de PWM

    escalon_actual++;
    tiempo_inicio_escalon = tiempo_actual;

    if (escalon_actual >= TOTAL_ESCALONES) {//Fin de la secuencia

      analogWrite(PIN_PWM, 0);  // Apagar motor

      // Enviar datos por serial
      for (int i = 0; i < buffIndex; i++) {
        Serial.print(datos[i].tiempo);
        Serial.print(',');
        Serial.print(datos[i].valor_pwm);
        Serial.print(',');
        Serial.println(datos[i].rpm);
      }
      while (true);  // Fin del programa
    }
    else {
      // Configurar PWM para el nuevo escalón
      uint8_t porcentaje_pwm;
      if (escalon_actual < NUM_ESCALONES_SUBIDA) {

        porcentaje_pwm = escalon_actual * ESCALON_PORCENTAJE;

      } else {

        porcentaje_pwm = (TOTAL_ESCALONES - escalon_actual) * ESCALON_PORCENTAJE;

      }
      int valor_pwm = map(porcentaje_pwm, 0, 100, 0, 255);
      analogWrite(PIN_PWM, valor_pwm);
    }
  }
}