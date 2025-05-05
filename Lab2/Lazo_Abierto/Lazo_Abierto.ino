/**
 * @file lazo_abierto.ino
 * @brief Control de velocidad en lazo abierto mediante modulación por ancho de pulso (PWM).
 *
 * Este programa incrementa el ciclo de trabajo del PWM desde 0% hasta 100% en pasos de 10%,
 * manteniendo cada valor por 3 segundos. Luego, apaga el motor por 2 segundos. 
 * El objetivo es observar el comportamiento del sistema en lazo abierto.
 *
 * @authors Angie Paola Jaramillo Ortega,
 *          Juan Manuel Rivera Florez
 * 
 * 
 * @date 04-05-2025
 */



#define PWM_PIN           15      // Pin de salida PWM
#define PWM_FREQ          250     // Frecuencia del PWM en Hz
#define PWM_RESOLUTION    8     // Resolución del PWM (8 bits)
// #define PWM_CYCLE         30      // Ciclo de trabajo en % (0 a 100)


/**
 * @brief Función de interrupción para contar los pulsos del encoder
 * 
 * Esta función se ejecuta cada vez que se detecta un pulso en el pin del encoder.
 * Incrementa el contador de pulsos.
 */
void setup() {
  Serial.begin(115200);
  pinMode(PWM_PIN, OUTPUT);
  pinMode(14, OUTPUT);
  digitalWrite(14, HIGH);

  //Configurar la frecuencia y resolución del PWM
  analogWriteFreq(PWM_FREQ);
  analogWriteResolution(PWM_RESOLUTION); 
  // int valorPWM = map(PWM_CYCLE, 0, 100, 0, PWM_RESOLUTION); // Calcular el valor PWM correspondiente al porcentaje
  // analogWrite(PWM_PIN, valorPWM);
}


/**
 * @brief Función principal del programa
 * 
 * En esta función se incrementa el ciclo de trabajo del PWM desde 0% hasta 100% en pasos de 10%,
 * manteniendo cada valor por 3 segundos. Luego, apaga el motor por 2 segundos.
 */
void loop() {
  for(int ciclo = 0; ciclo <= 100; ciclo += 10) {  // Incrementos de 10%
    int valorPWM = map(ciclo, 0, 100, 0, 255);
    analogWrite(PWM_PIN, valorPWM);
    Serial.println(valorPWM);
    // Mantener este ciclo de trabajo por 3 segundos
    delay(3000);
  }
  // Apagar el motor completamente por 2 segundos
  analogWrite(PWM_PIN, 0);
  delay(2000);
}
