#define ENCODER 1         // Entrada del encoder
#define ENA 2         // PWM: velocidad del motor
#define IN1 4             // Dirección del motor
#define IN2 5             // Dirección del motor
#define PULSOS_POR_VUELTA 20  // Rueda encoder de 20 orificios

unsigned long tiempoAnterior = 0;
unsigned int conteoPulsos = 0;
bool estadoAnterior = LOW;
float rpm = 0; 

int pwmValor = 150;  //0-255

void setup() {
  Serial.begin(115200);
  pinMode(ENCODER, INPUT_PULLUP);
  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);

  // Sentido de giro: adelante
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);

  // Aplicar PWM
  analogWrite(ENA, pwmValor);

  Serial.print("PWM aplicado: ");
  Serial.println(pwmValor);
}

void loop() {
  // Contar pulsos del encoder (flanco ascendente)
  bool estadoActual = digitalRead(ENCODER);
  if (estadoActual == HIGH && estadoAnterior == LOW) {
    conteoPulsos++;
  }
  estadoAnterior = estadoActual;

  // Calcular RPM cada segundo
  unsigned long tiempoActual = millis();
  if (tiempoActual - tiempoAnterior >= 1000) {
    rpm = (conteoPulsos * 60.0) / PULSOS_POR_VUELTA;
    Serial.print("RPM: ");
    Serial.println(rpm);

    conteoPulsos = 0;
    tiempoAnterior = tiempoActual;
  }
}
