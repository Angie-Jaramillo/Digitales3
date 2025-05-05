/**
 * @file Implementacion.ino
 * 
 * @brief Medición de RPM con control PWM escalonado en Arduino
 * 
 *  Este archivo contiene la implementación de un sistema para medir las revoluciones por minuto (RPM) de un motor DC utilizando un encoder y una señal PWM que se incrementa en escalones. Los datos se registran y se envían por el puerto serial para análisis posterio
 * 
 * 
 * 
 * 
 */




#define PIN_ENCODER           16    // Pin del encoder
#define PIN_PWM               15    // Pin del PWM  
#define PULSOS_POR_VUELTA     20    // Pulsos por vuelta del encoder
#define FRECUENCIA_PWM        250   // Frecuencia del PWM en Hz
#define RESOLUCION_PWM        8     // Resolución del PWM en bits

#define MAX_MUESTRAS          20000  // máximo permitido

/**
 * @brief Estructura para almacenar los datos de cada muestra
 * 
 */
struct Buffer {
  unsigned long tiempo;
  uint8_t valor_pwm;
  float rpm;
};

Buffer datos[MAX_MUESTRAS];   // Buffer para almacenar los datos de las muestras

volatile int conteo_pulsos = 0;   // Contador de pulsos del encoder


/**
 * @brief Enumeración para los estados del sistema
 */
enum Estado { WAIT, MANUAL, CAPTURA };    

Estado estado = WAIT;   // Estado inicial del sistema 
    
uint8_t PWM_CYCLE = 0;
unsigned long tiempo_ultima_muestra = 0;
unsigned long tiempo_ultimo_reporte = 0;
unsigned long tiempo_inicio_captura = 0;
unsigned long tiempo_inicio_escalon = 0;

int buffIndex = 0;
int escalon_actual = 0;
int TOTAL_ESCALONES = 0;
int ESCALON_PORCENTAJE = 20;


/**
 * @brief Función de interrupción para contar los pulsos del encoder
 * 
 */
void contarPulsos() {
  conteo_pulsos++;
}

/**
 * @brief Función de configuración inicial
 * 
 * Configura los pines, la frecuencia y resolución del PWM, y establece la comunicación serial.
 */
void setup() {
  Serial.begin(115200);
  pinMode(PIN_ENCODER, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_ENCODER), contarPulsos, RISING);

  pinMode(PIN_PWM, OUTPUT);
  analogWriteFreq(FRECUENCIA_PWM);
  analogWriteResolution(RESOLUCION_PWM);
  analogWrite(PIN_PWM, 0);

  Serial.println("Listo. Use START <paso> o PWM <valor>");
}

/**
 * @brief Función principal del programa
 * 
 * En esta función se manejan los estados del sistema, se leen los comandos por USB y se realizan las mediciones.
 */
void loop() {
  unsigned long tiempo_actual = millis();

  // Lectura de comandos por USB
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    if (cmd.startsWith("START ")) {
      ESCALON_PORCENTAJE = constrain(cmd.substring(6).toInt(), 1, 100);
      int NUM_ESCALONES_SUBIDA = (100 / ESCALON_PORCENTAJE) + 1;
      int NUM_ESCALONES_BAJADA = NUM_ESCALONES_SUBIDA - 1;
      TOTAL_ESCALONES = NUM_ESCALONES_SUBIDA + NUM_ESCALONES_BAJADA;
      buffIndex = 0;
      escalon_actual = 0;
      tiempo_inicio_captura = tiempo_actual;
      tiempo_inicio_escalon = tiempo_actual;
      tiempo_ultima_muestra = tiempo_actual;
      analogWrite(PIN_PWM, 0);
      estado = CAPTURA;
      Serial.println("tiempo(ms),valor_pwm(%),RPM");
    }
    else if (cmd.startsWith("PWM ")) {
      PWM_CYCLE = constrain(cmd.substring(4).toInt(), 0, 100);
      analogWrite(PIN_PWM, map(PWM_CYCLE, 0, 100, 0, 255));
      tiempo_ultimo_reporte = tiempo_actual;
      estado = MANUAL;
    }
  }

  // Modo MANUAL: Reporta PWM y RPM cada 500ms
  if (estado == MANUAL && (tiempo_actual - tiempo_ultimo_reporte >= 500)) {
    noInterrupts();
    int pulsos = conteo_pulsos;
    conteo_pulsos = 0;
    interrupts();
    float rpm = (pulsos * 1000.0 / 500.0) / PULSOS_POR_VUELTA * 60.0;
    Serial.print(PWM_CYCLE); Serial.print(","); Serial.println(rpm);
    tiempo_ultimo_reporte = tiempo_actual;
  }

  // Modo CAPTURA: Muestrea cada 4ms
  if (estado == CAPTURA && (tiempo_actual - tiempo_ultima_muestra >= 4)) {
    noInterrupts();
    int pulsos = conteo_pulsos;
    conteo_pulsos = 0;
    interrupts();
    float rpm = (pulsos * 1000.0 / 4.0) / PULSOS_POR_VUELTA * 60.0;
    unsigned long tiempo_rel = tiempo_actual - tiempo_inicio_captura;

    uint8_t porcentaje_pwm;
    if (escalon_actual < (100 / ESCALON_PORCENTAJE) + 1) {
      porcentaje_pwm = escalon_actual * ESCALON_PORCENTAJE;
    } else {
      porcentaje_pwm = (TOTAL_ESCALONES - escalon_actual) * ESCALON_PORCENTAJE;
    }
    porcentaje_pwm = constrain(porcentaje_pwm, 0, 100);
    uint8_t valor_pwm = map(porcentaje_pwm, 0, 100, 0, 255);
    analogWrite(PIN_PWM, valor_pwm);

    if (buffIndex < MAX_MUESTRAS) {
      datos[buffIndex++] = {tiempo_rel, porcentaje_pwm, rpm};
    }

    tiempo_ultima_muestra = tiempo_actual;
  }

  // Modo CAPTURA: Cambio de escalón cada 2s
  if (estado == CAPTURA && (tiempo_actual - tiempo_inicio_escalon >= 2000)) {
    escalon_actual++;
    tiempo_inicio_escalon = tiempo_actual;

    if (escalon_actual >= TOTAL_ESCALONES) {
      analogWrite(PIN_PWM, 0);
      for (int i = 0; i < buffIndex; i++) {
        Serial.print(datos[i].tiempo);
        Serial.print(',');
        Serial.print(datos[i].valor_pwm);
        Serial.print(',');
        Serial.println(datos[i].rpm);
      }
      estado = WAIT;
    }
  }
}
