# Lazo Abierto – Control de Velocidad con PWM

Este programa controla un motor usando una señal PWM de lazo abierto, sin retroalimentación. Aumenta gradualmente el ciclo de trabajo cada 3 segundos, desde 0% hasta 100%, y luego detiene el motor durante 2 segundos antes de repetir.

## Descripción

- Salida PWM en el pin 15.
- Frecuencia: 250 Hz.
- Resolución: 8 bits (0–255).
- Incrementa el ciclo de trabajo del 0% al 100% en pasos de 10%.
- Cada nivel se mantiene por 3 segundos.
- Detiene la salida durante 2 segundos antes de reiniciar.

## Configuración de los pines

| Señal       | Pin |
|-------------|-----|
| PWM Output  | 15  |
| LED/Vcc     | 14  |

## Documentación

Para generar la documentación en Doxygen ejecute el siguiente comando en el terminal: 
``bash
doxygen Doxyfile
```