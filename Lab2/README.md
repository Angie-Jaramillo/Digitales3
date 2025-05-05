# PRÁCTICA 2 – CARACTERIZACIÓN DE UN MOTOR DC (Arduino)

## Descripción

Este laboratorio implementa un sistema completo de control y medición de velocidad (RPM) en motores DC usando Arduino. A través de modulación por ancho de pulso (PWM) y lectura con encoder, se analizan dos configuraciones de control: **lazo abierto** y **lazo cerrado con captura de datos**. El propósito es observar el comportamiento del motor ante distintos ciclos de trabajo y medir su respuesta dinámica.

---

## Archivos del Proyecto

| Archivo                | Descripción                                                                 |
|------------------------|-----------------------------------------------------------------------------|
| `lazo_abierto.ino`     | Control de velocidad en lazo abierto con PWM escalonado de 0% a 100%.       |
| `Read_RPM.ino`         | Lectura de pulsos de encoder y estimación de RPM en tiempo real.            |
| `implementacion.ino`   | Medición en lazo cerrado con control por comandos y captura de datos.       |

---
Por otro lado se tiene la misma implementación de funcionalidades en micropython.

## Detalles de cada módulo

### 🔹 `lazo_abierto.ino`

**Objetivo:**  
Evaluar el comportamiento del motor sin retroalimentación, variando el ciclo PWM desde 0% hasta 100% en pasos de 10% cada 3 segundos, con un apagado final de 2 segundos.

**Características:**
- Control sin medición (lazo abierto).
- PWM generado mediante `analogWrite()`.
- No se realiza lectura de encoder.

---

### 🔹 `Read_RPM.ino`

**Objetivo:**  
Estimar la velocidad del motor leyendo los pulsos del encoder cada segundo y calculando las RPM.

**Características:**
- Interrupciones para contar pulsos.
- Buffer de 5 segundos para promediar.
- Cálculo de RPM basado en pulsos y `PulsosPorVuelta`.

---

### 🔹 `implementacion.ino` *(captura de datos en lazo cerrado)*

**Objetivo:**  
Ejecutar una secuencia automática de PWM escalonado (subida y bajada), mientras se capturan las RPM con alta resolución temporal (cada 4 ms). También admite control manual.

**Comandos Serial:**
- `START <valor>`: Ejecuta secuencia automática con incrementos de PWM según el valor dado (por ejemplo `START 20`).
- `PWM <valor>`: Control manual del ciclo PWM en porcentaje.

**Características:**
- PWM con resolución de 8 bits.
- 11 escalones de PWM con duración de 2 segundos cada uno.
- Medición de RPM con encoder mediante interrupción.
- Registro de tiempo, PWM y RPM en buffer estructurado.
- Exportación de datos por serial para análisis externo.

---

## Documentación.
Para generar la documentación, dirijase a cada carpeta de cada código implementado y ejecute el comando: 

```bash
doxygen Doxyfile

```
